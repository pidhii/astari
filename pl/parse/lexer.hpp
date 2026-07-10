#pragma once

#include "../obj/object.hpp"

#include <algorithm>
#include <array>
#include <format>
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
  _is_word_char(int c)
  {
    static const std::array chars = {
      '_', '-', '+'
    };
    return std::isalnum(c) or
           std::find(chars.begin(), chars.end(), c) != chars.end();
  }

  static bool
  _is_num_char(int c)
  {
    static const std::array chars = {'.'};
    return std::isalnum(c) or
           std::find(chars.begin(), chars.end(), c) != chars.end();
  }

  std::string
  _read_word(std::istream &in) const
  {
    std::string result;
    while (_is_word_char(in.peek()))
      result.push_back(in.get());
    return result;
  }

  std::string
  _read_number(std::istream &in) const
  {
    std::string result;
    if (in.peek() == '-' or in.peek() == '+')
      result.push_back(in.get());
    while (_is_num_char(in.peek()))
      result.push_back(in.get());
    return result;
  }

  token
  _read_token(std::istream &in) const
  {
    // Skip whitespaces
    while (std::isspace(in.peek())) in.get();

    // EOF
    if (not in or in.eof())
      return {eof, ""};

    if (in.peek() == '-')
    {
      in.get();
      if (in.peek() == '>')
      {
        in.get();
        return {rarrow, "->"};
      }
      in.unget();
    }

    // Nonterminal
    if (std::isupper(in.peek()) or in.peek() == '_')
      return {nonterminal_symbol, _read_word(in)};
    // Terminal
    if (std::islower(in.peek()))
      return {terminal_symbol, _read_word(in)};
    // Numbers
    if (std::isdigit(in.peek()) or in.peek() == '+' or in.peek() == '-')
      return {number, _read_number(in)};
    // Comma
    if (in.peek() == ',')
    {
      in.get();
      return {',', ","};
    }
    // Open bracket
    if (in.peek() == '(')
    {
      in.get();
      return {'(', "("};
    }
    // Close bracket
    if (in.peek() == ')')
    {
      in.get();
      return {')', ")"};
    }

    if (in.peek() == ':')
    {
      in.get();
      if (in.peek() == '-')
      {
        in.get();
        return {colon_minus, ":-"};
      }
      else
        return {':', ":"};
    }

    if (in.peek() == '.')
    {
      in.get();
      return {'.', "."};
    }

    if (in.peek() == '?')
    {
      in.get();
      return {'?', "?"};
    }

    if (in.peek() == ';')
    {
      in.get();
      return {';', ";"};
    }

    throw std::runtime_error {std::format("invalid symbol ({})", in.peek())};
  }
};