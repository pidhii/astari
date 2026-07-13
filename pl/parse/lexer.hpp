#pragma once

#include "../obj/object.hpp"

#include <istream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>


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
  directive,
  cons,

  cmp,
};

struct token {
  int type;
  std::variant<std::string, object> val;

  bool
  operator == (const token &other) const noexcept
  { return type == other.type and val == other.val; }
};


struct tokstream {
  std::vector<token> tokens;
  std::vector<std::pair<std::istream::pos_type, std::istream::pos_type>> pos;
};

class lexer {
  public:
  template <typename OIter>
  void
  tokenize(std::istream &in, OIter oit)
  {
    while (true)
    {
      const token tok = _read_token(in);
      if (tok.type != eof)
        *oit++ = tok;
      else
        return;
    }
  }

  template <typename OIter>
  void
  tokenize(std::string_view str, OIter oit)
  { std::istringstream ss {str.data()}; tokenize(ss, oit); }


  tokstream
  tokenize(std::istream &in);

  private:
  token
  _read_token(std::istream &in) const;
};