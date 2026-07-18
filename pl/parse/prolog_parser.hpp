#pragma once

#include "pl/builtins/breadthfirst.hpp"
#include "pl/builtins/tabulate.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"


// Make into a class
struct tokens {
  object list;
  dictionary vars;
};

class prolog_parser {
  public:
  prolog_parser();

  object
  parse_expr(dictionary &symbols, dictionary &vars, std::string_view text);

  object
  parse_expr(dictionary &symbols, std::string_view text)
  { dictionary vars; return parse_expr(symbols, vars, text); }

  object
  parse_expr(dictionary &symbols, const tokens &toks);

  object
  parse_one_stmt(dictionary &symbols, tokens &toks);

  tokens
  tokenize(std::string_view text);

  object
  tokenize(dictionary &vardict, std::string_view text);

  void
  tokenize_more(tokens &tokens, std::string_view text);

  void
  pop_token(tokens &tokens)
  {
    assert(not tokens.list.empty());
    const auto [elts, nelts] =
        unmake_list(m_pl, object_view(tokens.list).begin());
    tokens.list = make_list(m_pl, nelts - 1, object_view(elts).begin());
  }

  dictionary &
  symbols()
  { return m_pl.symbols(); }

  private:
  void
  _load(std::istream &in);

  void
  _interpret_token(const token &stmt, const dictionary &vardict);

  std::pair<object, object>
  _parse_first_stmt(dictionary &vardict, object_view toklist);

  private:
  interpreter m_pl;

  lib_breadthfirst m_lib_bf;
  lib_tabulate m_lib_tab;
};