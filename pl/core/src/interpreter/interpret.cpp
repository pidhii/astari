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

  // fill in `objfile`
  prolog_parser p;
  std::ifstream file {path.data()};
  objfile.read(file, *this);

  for (object &obj : objfile.objects)
  {
    transfer_symbols(objfile.symbols, m_symdict, obj);
    interpret(p, obj);
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
    interpret(p, obj);
  }
}


void
interpreter::eval(object_view obj, const dictionary &vardict)
{
  std::cout << "[eval] " << dump_object(m_symdict, obj) << std::endl;
  varnamespace ns;
  const object_view expr = adopt_g(ns, obj);
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


word_t
interpreter::predicate_indicator(object_iterator indicator)
{
  basic_encoder ec;
  basic_decoder dc;
  #define ATOM(name, arity) ec.encode(term_header(m_symdict[name], arity))

  const auto raise_type_error = [&]() {
    raise(term("type_error", term("predicate_indicator"),
               dc.decode_object(indicator)));
  };

  if (the_word(indicator[0]) != ATOM("/", 2))
    raise_type_error();
  object_iterator it = indicator + 1;
  const object_view name = dc.decode_object(it);
  const object_view arity = dc.decode_object(it);
  if (not is_term_n(name[0], 0) or
      word_type(arity[0]) != word_type::signed_int_number)
    raise_type_error();

  term_header hdr;
  int ar;
  dc.decode(name[0], hdr);
  dc.decode(arity[0], ar);
  return ec.encode(term_header(hdr.id, ar));
}


void
interpreter::interpret(prolog_parser &p, object_view stmt,
                       const dictionary &vardict)
{
  assert(not stmt.empty());

  basic_encoder ec;
  basic_decoder dc;
  #define ATOM(name, arity) ec.encode(term_header(m_symdict[name], arity))

  if (is_term(stmt[0], op_penis, 1)) // Directive
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

    if (the_word(stmt[1]) == ATOM("dynamic", 1))
    {
      const word_t indicator = predicate_indicator(stmt.begin() + 2);
      dynamic({&indicator, 1});
      return;
    }

    if (the_word(stmt[1]) == ATOM("op", 3))
    {
      object_iterator it = stmt.begin() + 2;
      const object_view prec = dc.decode_object(it);
      const object_view spec = dc.decode_object(it);
      const object_view name = dc.decode_object(it);
      assert(word_type(prec[0]) == word_type::signed_int_number);
      assert(the_word(spec[0]) == ATOM("xfx", 0) or
             the_word(spec[0]) == ATOM("xfy", 0) or
             the_word(spec[0]) == ATOM("yfx", 0) or
             the_word(spec[0]) == ATOM("fx", 0) or
             the_word(spec[0]) == ATOM("fy", 0) or
             the_word(spec[0]) == ATOM("xf", 0) or
             the_word(spec[0]) == ATOM("yf", 0));
      assert(is_term_n(name[0], 0));
      object opdecl {dc.decode_object(stmt.begin() + 1)};
      transfer_symbols(m_symdict, p.interpreter().symbols(), opdecl);
      p.interpreter().assertz(opdecl);
      return;
    }
  }

  else if (is_term(stmt[0], op_penis, 2)) // Predicate
  {
    auto it = stmt.begin() + 1;
    const object_view sign = dc.decode_object(it);
    const object_view body = dc.decode_object(it);
    assertz(sign, body);
    return;
  }

  else if (is_term(stmt[0])) // Statement
  {
    assertz(stmt);
    return;
  }

  throw std::runtime_error {
      std::format("can't interpret {}", dump_object(m_symdict, stmt))};
}
