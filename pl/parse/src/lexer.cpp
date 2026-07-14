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

static bool
_is_op_char(int c)
{
  static const std::array chars = {
    '+', '-', '@', '=', ':', '*', '/', '\\',
    '<', '>'
  };
  return std::find(chars.begin(), chars.end(), c) != chars.end();
}

std::string
_read_op(std::istream &in)
{
  std::string result;
  while (_is_op_char(in.peek()))
    result.push_back(in.get());
  return result;
}


static bool
_is_word_char(int c)
{
  static const std::array chars = {
    '_',
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

static bool
_is_number(std::istream &in)
{
  if (std::isdigit(in.peek()))
    return true;

  if (in.peek() == '+' or in.peek() == '-')
  {
    in.get();
    const bool result = _is_number(in);
    in.unget();
    return result;
  }

  if (in.peek() == '.')
  {
    in.get();
    const bool result = std::isdigit(in.peek());
    in.unget();
    return result;
  }

  return false;
}

static std::string
_read_word(std::istream &in)
{
  std::string result;
  while (_is_word_char(in.peek()))
    result.push_back(in.get());
  return result;
}

static std::string
_read_number(std::istream &in)
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

static std::string
_read_string(std::istream &in)
{
  // Opening quote is expected to be already consumed by the caller.
  std::string result;
  while (true)
  {
    if (not in or in.eof())
      throw std::runtime_error {"unterminated string literal"};

    const int c = in.get();

    if (c == '"')
      break;

    if (c == '\\')
    {
      if (not in or in.eof())
        throw std::runtime_error {"unterminated string literal"};

      const int esc = in.get();
      switch (esc)
      {
        case 'n': result.push_back('\n'); break;
        case 't': result.push_back('\t'); break;
        case 'r': result.push_back('\r'); break;
        case 'v': result.push_back('\v'); break;
        case 'f': result.push_back('\f'); break;
        case 'b': result.push_back('\b'); break;
        case 'a': result.push_back('\a'); break;
        case '0': result.push_back('\0'); break;
        case '\\': result.push_back('\\'); break;
        case '\'': result.push_back('\''); break;
        case '"': result.push_back('"'); break;
        case '\n': break; // line continuation: escaped newline is dropped
        default:
          throw std::runtime_error {
              std::format("invalid escape sequence (\\{})", char(esc))};
      }
    }
    else
      result.push_back(char(c));
  }
  return result;
}

static bool
_trygetword(std::istream &in, std::string_view word)
{
  if (word.empty())
    return not std::isalnum(in.peek());

  if (in.get() == word[0] and _trygetword(in, word.substr(1)))
    return true;
  in.unget();
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

  if (_tryget(in, "->"))
    return {rarrow, "->"};

  // String literal
  if (in.peek() == '"')
  {
    in.get();
    return {str, _read_string(in)};
  }

  if (_trygetword(in, "is"))
    return {assignlike, "is"};

  // Nonterminal
  if (std::isupper(in.peek()) or in.peek() == '_')
    return {nonterminal_symbol, _read_word(in)};
  // Terminal
  if (std::islower(in.peek()))
    return {terminal_symbol, _read_word(in)};
  // Numbers
  if (_is_number(in))
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

  if (_tryget(in, "+")) return {pluslike, "+"};
  if (_tryget(in, "-")) return {pluslike, "-"};
  if (_tryget(in, "*")) return {mullike, "*"};
  if (_tryget(in, "//")) return {mullike, "//"};
  if (_tryget(in, "/")) return {mullike, "/"};

  if (_tryget(in, "[")) return {'[', "["};
  if (_tryget(in, "]")) return {']', "]"};
  if (_tryget(in, "|")) return {'|', "|"};
  if (_tryget(in, "==")) return {cmplike, "=="};
  if (_tryget(in, "\\==")) return {cmplike, "\\=="};
  if (_tryget(in, "\\=")) return {cmplike, "\\="};
  if (_tryget(in, "@>=")) return {cmplike, "@>="};
  if (_tryget(in, "@=<")) return {cmplike, "@=<"};
  if (_tryget(in, "@>")) return {cmplike, "@>"};
  if (_tryget(in, "@<")) return {cmplike, "@<"};

  if (_tryget(in, "=:=")) return {cmplike, "=:="};
  if (_tryget(in, "=\\=")) return {cmplike, "=\\="};
  if (_tryget(in, "=<")) return {cmplike, "=<"};
  if (_tryget(in, ">=")) return {cmplike, ">="};
  if (_tryget(in, "<")) return {cmplike, "<"};
  if (_tryget(in, ">")) return {cmplike, ">"};

  if (_tryget(in, "=")) return {assignlike, "="};

  throw std::runtime_error {std::format("invalid symbol ({})", char(in.peek()))};
}