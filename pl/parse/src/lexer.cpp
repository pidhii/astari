#include "lexer.hpp"

#include <format>


tokstream
lexer::tokenize(std::istream &in)
{
  tokstream result;
  while (true)
  {
    const std::istream::pos_type pstart = in.tellg();
    token tok = _read_token(in);
    const std::istream::pos_type pend = in.tellg();
    if (tok.type == eof)
      return result;
    else
    {
      result.tokens.emplace_back(std::move(tok));
      result.pos.emplace_back(pstart, pend);
    }
  }
}


bool
lexer::_is_word_char(int c)
{
  static const std::array chars = {
    '_',
  };
  return std::isalnum(c) or
          std::find(chars.begin(), chars.end(), c) != chars.end();
}

bool
lexer::_is_num_char(int c)
{
  static const std::array chars = {'.'};
  return std::isalnum(c) or
          std::find(chars.begin(), chars.end(), c) != chars.end();
}

std::string
lexer::_read_word(std::istream &in) const
{
  std::string result;
  while (_is_word_char(in.peek()))
    result.push_back(in.get());
  return result;
}

std::string
lexer::_read_number(std::istream &in) const
{
  std::string result;
  if (in.peek() == '-' or in.peek() == '+')
    result.push_back(in.get());
  while (_is_num_char(in.peek()))
    result.push_back(in.get());
  return result;
}


static bool
_tryget(std::istream &in, std::string_view s)
{
  if (s.empty())
    return true;

  if (in.eof())
    return false;

  if (in.peek() == s[0])
  {
    in.get();
    if (_tryget(in, s.substr(1)))
      return true;
    else
      in.unget();
  }
  return false;
}


token
lexer::_read_token(std::istream &in) const
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

  if (in.peek() == '=')
  {
    in.get();
    if (in.peek() == '=')
    {
      in.get();
      return {cmp, "termeq"};
    }
    return {cmp, "eq"};
  }

  if (_tryget(in, "[")) return {'[', "["};
  if (_tryget(in, "]")) return {']', "]"};
  if (_tryget(in, "|")) return {'|', "|"};
  if (_tryget(in, "\\==")) return {cmp, "termne"};
  if (_tryget(in, "\\=")) return {cmp, "neq"};
  if (_tryget(in, "@>=")) return {cmp, "termge"};
  if (_tryget(in, "@=<")) return {cmp, "termle"};
  if (_tryget(in, "@>")) return {cmp, "termgt"};
  if (_tryget(in, "@<")) return {cmp, "termlt"};

  throw std::runtime_error {std::format("invalid symbol ({})", char(in.peek()))};
}