#include "interpreter.hpp"

#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/misc/object_file.hpp"
#include "pl/misc/show_location.hpp"
#include "pl/parse/parse_error.hpp"
#include "pl/parse/prolog_parser.hpp"

#include <fstream>
#include <iostream>



#define ERROR(fmt, ...)                                                        \
  throw std::runtime_error { std::format(fmt, ##__VA_ARGS__) }

void
interpreter::load_file(std::string_view path)
{
  std::ifstream file {path.data(), std::ios_base::binary};
  if (not file)
    ERROR("failed to open file for reading ({})", path);

  try { load(file); }
  catch (const parse_error &exn)
  {
    show_location(std::cerr, path, exn.where.first, exn.where.second, 2);
    throw;
  }
  catch (const std::exception &exn)
  {
    ERROR("failed to load file {} ({})", path, exn.what());
  }
}


void
interpreter::load_objfile(std::string_view path)
{
  object_file objfile;

  { // fill in `objfile`
    prolog_parser p;
    std::ifstream file {path.data()};
    objfile.read(file, *this);
  }

  for (object &obj : objfile.objects)
  {
    transfer_symbols(objfile.symbols, m_symdict, obj);
    interpret(obj);
  }
}


void
interpreter::load(std::istream &in)
{
  prolog_parser p;
  std::string text {std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>()};
  tokens toks = p.tokenize(text);

  while (toks.list.size() > 1)
  {
    const object obj = p.parse_one_stmt(m_symdict, toks);
    interpret(obj);
  }
}


void
interpreter::eval(object_view obj, const dictionary &vardict)
{
  std::cout << "[eval] " << dump_object(m_symdict, obj) << std::endl;
  varnamespace ns;
  const object_view expr = adopt(ns, obj);
  make_true(expr, [this, ns, vardict](runtime &rt) {
    std::cout << "yes";
    bool isfirst = true;
    for (const auto [nsid, rtid] : ns)
    {
      const std::string_view varname = vardict[nsid];
      if (varname == "_")
        continue;
      if (isfirst) std::cout << ": ";
      else         std::cout << ", ";
      isfirst = false;
      if (const auto varval = rt.dereference(rtid))
        std::cout << varname << " = "
                  << dump_object(m_symdict, rt.reconstruct(*varval));
      else
        std::cout << varname << " is unbound";
    }
    std::cout << std::endl;
  });
}


void
interpreter::eval(std::string_view text)
{
  prolog_parser p;
  dictionary vardict;
  const object expr = p.parse_expr(m_symdict, vardict, text);
  eval(expr, vardict);
}


void
interpreter::interpret(object_view stmt, const dictionary &vardict)
{
  assert(not stmt.empty());

  basic_encoder ec;
  basic_decoder dc;
  #define ATOM(name, arity) ec.encode(term_header(m_symdict[name], arity))
  #define ARITY(term) dc.decode_term_header(term).arity

  if (the_word(stmt[0]) == ATOM(":-", 1)) // Directive
  {
    if (the_word(stmt[1]) == ATOM("ensure_loaded", 1))
    {
      assert(is_blob(stmt[2]));
      assert(blob_tag(stmt[2]) == blob_tag::string);
      ensure_loaded(string(stmt[2]));
      return;
    }

    if (the_word(stmt[1]) == ATOM("import_directory", 1))
    {
      assert(is_blob(stmt[2]));
      assert(blob_tag(stmt[2]) == blob_tag::string);
      import_directory(string(stmt[2]));
      return;
    }
  }

  else if (the_word(stmt[0]) == ATOM(":-", 2)) // Predicate
  {
    auto it = stmt.begin() + 1;
    const object_view sign = dc.decode_object(it);
    const object_view body = dc.decode_object(it);
    add_predicate(sign, body);
    return;
  }

  else if (is_term(stmt[0])) // Statement
  {
    add_predicate(stmt);
    return;
  }

  throw std::runtime_error {
      std::format("can't interpret {}", dump_object(m_symdict, stmt))};
}
