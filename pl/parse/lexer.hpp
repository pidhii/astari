#pragma once

#include "../obj/object.hpp"

#include <istream>
#include <string>
#include <variant>


enum token_type {
  nonterminal_symbol,
  terminal_symbol,
  number,
  eof,
  colon_minus,
  obj,
  andseq,
  orseq,
  rarrow,
  ifthen,
  predicate,
  statement,
};

struct token {
  int type;
  std::variant<std::string, object> val;

  bool
  operator == (const token &other) const noexcept
  { return type == other.type and val == other.val; }
};


class lexer {
  public:
  template <typename OIter>
  void
  tokenize(std::istream &in, OIter oit)
  {
    while (true) {
      const token tok = _read_token(in);
      if (tok.type != eof)
        *oit++ = tok;
      else
        return;
    }
  }

  private:
  static bool
  _is_word_char(int c);
  
  static bool
  _is_num_char(int c);

  std::string
  _read_word(std::istream &in) const;

  std::string
  _read_number(std::istream &in) const;

  token
  _read_token(std::istream &in) const;
};