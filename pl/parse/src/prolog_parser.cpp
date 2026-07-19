#include "prolog_parser.hpp"

#include "pl/builtins/minimal.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/core/runtime.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/misc/object_file.hpp"
#include "pl/obj/object.hpp"

#include <stdexcept>
#include <fstream>


#define ERROR(fmt, ...)                                                        \
  throw std::runtime_error { std::format(fmt, ##__VA_ARGS__) }

#ifndef PARSER_OBJFILE_PATH
# define PARSER_OBJFILE_PATH "./parser.plo"
#endif


static object
_tokenize(interpreter &pl, dictionary &vardict, std::string_view text)
{
  std::istringstream stream {std::string(text)};
  const auto [elts, nelts] = lexer().tokens(pl, vardict, stream);
  const object result = make_list(pl, nelts, object_view(elts).begin());
  // std::clog << "tokenlist: " << pl.dump(result) << std::endl;
  return result;
}


static void
transfer_symbols(dictionary &from, dictionary &to, object &obj)
{
  basic_encoder ec;
  basic_decoder dc;

  for (word_t &word : obj)
  {
    if (is_term(word))
    {
      term_header hdr;
      dc.decode(word, hdr);
      const size_t newid = to[from[hdr.id]];
      word = ec.encode(term_header(newid, hdr.arity));
    }
  }
}


static void
recover_variables(runtime &rt, varnamespace &ns, object &obj)
{
  basic_encoder ec;
  basic_decoder dc;
  for (word_t &word : obj)
  {
    if (is_nonterminal(word))
    {
      // Find if this variable is bound with an original variable (from `ns`).
      // If so, rename it to this original ID.
      nonterminal var;
      dc.decode(word, var);
      for (const auto [nsid, rtid] : ns)
      {
        if (rt.bound(var.id, rtid))
        {
          word = ec.encode(nonterminal(nsid));
          break;
        }
      }
    }
  }
}



prolog_parser::prolog_parser() : m_lib_bf {m_pl}, m_lib_tab {m_pl}
{
  minimal_predicates(m_pl);

  // tokens/2
  m_pl.add_meta_op("tokens", [this](runtime &rt, int argc, object_iterator argv,
                                    const continuation &cont) {
    assert(argc == 2);
    basic_decoder dc;
    const object_view string = rt.reduce(dc.decode_object(argv));
    const object_view tokens = rt.reduce(dc.decode_object(argv));
    assert(is_blob(string[0]));

    dictionary vardict;
    const object list = _tokenize(m_pl, vardict, ::string(string[0]));
    const object_view pobj = rt.adopt(list);
    if (rt.match(pobj, tokens))
      cont(rt);
    else
      rt.unallocate(pobj);
  });

  m_pl.add_meta_op("debug", [this](runtime &rt, int argc, object_iterator argv,
                                   const continuation &cont) {
    assert(argc == 1);
    basic_decoder dc;
    const object x = rt.reconstruct(dc.decode_object(argv));
    std::cerr << dump_object(m_pl.symbols(), x) << std::endl;
    cont(rt);
  });

  std::ifstream parserfile {PARSER_OBJFILE_PATH};
  assert(parserfile);

  object_file objfile;
  objfile.read(parserfile, m_pl);

  for (object &obj : objfile.objects)
  {
    transfer_symbols(objfile.symbols, m_pl.symbols(), obj);
    m_pl.interpret(obj);
  }
}


object
prolog_parser::parse_expr(dictionary &symbols, dictionary &vardict,
                          std::string_view text)
{
  const object toklist = tokenize(vardict, text);

  dictionary exprvars = vardict.subscope();
  auto var = [&](auto name) { return nonterminal(exprvars[name]); };
  const object goal = m_pl.make_term_noalloc(
      term("parse_expr", toklist, var("Expr")));

  varnamespace ns;
  const object_view adgoal = m_pl.adopt(ns, goal);

  object result;
  m_pl.make_true(m_pl, adgoal, [&] (runtime &rt) {
    result = rt.reconstruct(rt.dereference(ns[exprvars["Expr"]]).value());
    transfer_symbols(m_pl.symbols(), symbols, result);
    recover_variables(rt, ns, result);
  });

  assert(not result.empty());

  return result;
}


object
prolog_parser::parse_expr(dictionary &symbols, const tokens &toks)
{
  dictionary exprvars = toks.vars.subscope();
  auto var = [&](auto name) { return nonterminal(exprvars[name]); };
  const object goal = m_pl.make_term_noalloc(
      term("parse_expr", toks.list, var("Expr")));

  varnamespace ns;
  const object_view adgoal = m_pl.adopt(ns, goal);

  object result;
  m_pl.make_true(m_pl, adgoal, [&] (runtime &rt) {
    result = rt.reconstruct(rt.dereference(ns[exprvars["Expr"]]).value());
    transfer_symbols(m_pl.symbols(), symbols, result);
    recover_variables(rt, ns, result);
  });

  assert(not result.empty());

  return result;
}


tokens
prolog_parser::tokenize(std::string_view text)
{
  dictionary vardict;
  object list = _tokenize(m_pl, vardict, text);
  return {list, vardict};
}

tokens
prolog_parser::tokenize(std::istream &in)
{
  const std::string text {std::istreambuf_iterator<char>(in),
                          std::istreambuf_iterator<char>()};
  return tokenize(text);
}

object
prolog_parser::tokenize(dictionary &vardict, std::string_view text)
{
  object list = _tokenize(m_pl, vardict, text);
  return list;
}

object
prolog_parser::tokenize(dictionary &vardict, std::istream &in)
{
  const std::string text {std::istreambuf_iterator<char>(in),
                          std::istreambuf_iterator<char>()};
  return tokenize(vardict, text);
}


void
prolog_parser::tokenize_more(tokens &tokens, std::string_view text)
{
  object list = tokenize(tokens.vars, text);
  if (tokens.list.empty())
  {
    tokens.list = std::move(list);
    return;
  }

  basic_encoder ec;
  assert(tokens.list.back() ==
          ec.encode(term_header(m_pl.symbols()["nil"], 0)));
  tokens.list.pop_back();
  tokens.list += list;
}


object
prolog_parser::parse_one_stmt(dictionary &symbols, tokens &toks)
{
  // std::clog << "parse_one_stmt from: " << m_pl.dump(toks.list) << std::endl;
  auto [stmt, remlist] = _parse_first_stmt(toks.vars, toks.list);
  toks.list = remlist;
  transfer_symbols(m_pl.symbols(), symbols, stmt);
  return stmt;
}


std::pair<object, object>
prolog_parser::_parse_first_stmt(dictionary &vardict, object_view toklist)
{
  dictionary exprvars = vardict.subscope();
  auto var = [&](auto name) { return nonterminal(exprvars[name]); };
  const object goal = m_pl.make_term_noalloc(
      term("parse_one_stmt", toklist, var("Term"), var("RemTokens")));

  varnamespace ns;
  const object_view adgoal = m_pl.adopt(ns, goal);

  object term, remtokens;
  m_pl.make_true(m_pl, adgoal, [&] (runtime &rt) {
    term = rt.reconstruct(rt.dereference(ns[exprvars["Term"]]).value());
    remtokens = rt.reconstruct(rt.dereference(ns[exprvars["RemTokens"]]).value());
  });


  if (term.empty() or remtokens.empty())
  {
    std::cerr << "parse failure: " << dump_object(m_pl.symbols(), toklist)
              << std::endl;
    throw std::runtime_error {"parse failure"};
  }

  return {term, remtokens};
}