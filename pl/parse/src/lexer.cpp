#include "lexer.hpp"
#include "parse_error.hpp"

#include "pl/coding/basic_encoder.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/object_allocator.hpp"

#include <format>


static bool
_is_op_char(int c)
{
  static const std::array chars = {
    '+', '-', '@', '=', ':', '*', '/', '\\',
    '<', '>', '!', '.'
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
  return std::isalnum(c);
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

  while (_is_num_char(in.peek()))
    result.push_back(in.get());

  if (in.peek() == '.')
  {
    result.push_back(in.get());
    while (_is_num_char(in.peek()))
      result.push_back(in.get());
  }

  return result;
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


std::pair<object, size_t>
lexer::tokens(dictionary &symbols, dictionary &vardict, std::istream &in)
{
  object result;
  size_t nelts = 0;
  while (true)
  {
    const std::istream::pos_type pstart = in.tellg();
    bool quote;
    word_t elt;
    try { elt = _read_elt(symbols, vardict, in, quote); }
    catch (const std::exception &exn)
    { throw parse_error {exn.what(), {pstart, pstart + 1l}}; }
    // const std::istream::pos_type pend = in.tellg();
    if (elt == 0)
      return {result, nelts};
    else
    {
      if (quote)
      {
        basic_encoder ec;
        result += ec.encode(term_header(symbols["q"], 1));
      }
      result += elt;
      nelts++;
    }
  }
}


bool
lexer::is_operator(std::string_view name)
{
  if (name.empty())
    return false;
  else
    return std::all_of(name.begin(), name.end(), _is_op_char) or name == "is";
}


bool
lexer::is_symbol(std::string_view name)
{
  if (name.empty())
    return false;
  else
    return std::all_of(name.begin(), name.end(), _is_word_char);
}


static word_t
_make_number(std::string_view x)
{
  basic_encoder ec;
  size_t pos;

  try {
    const int32_t v = std::stoi(std::string(x), &pos);
    if (pos == x.size())
      return ec.encode(v);
  }
  catch (...) { }

  try {
    const float v = std::stof(std::string(x), &pos);
    if (pos == x.size())
      return ec.encode(v);
  }
  catch (...) { }

  throw std::runtime_error {"can't make number"};
}


word_t
lexer::_read_elt(dictionary &symbols, dictionary &vardict, std::istream &in,
                 bool &quote) const
{
  static object_allocator strings;

  quote = false;

  basic_encoder ec;

  #define ATOM(name) ec.encode(term_header(symbols[name], 0))
  #define VAR(name) ec.encode(nonterminal(vardict[name]))
  #define NUM(s) _make_number(s)

  // Skip whitespaces
  while (std::isspace(in.peek())) in.get();

  // Skip comments
  if (in.peek() == '%')
  {
    std::string _line;
    std::getline(in, _line);
    return _read_elt(symbols, vardict, in, quote);
  }

  // EOF
  if (not in or in.eof())
    return 0;

  // Special symbols
  if (in.peek() == '!') return in.get(), ATOM("!");
  if (in.peek() == '.') return in.get(), ATOM(".");

  // Symbol
  if (std::islower(in.peek())) // alphanumeric symbols
    return ATOM(_read_word(in));
  if (_is_op_char(in.peek())) // operator symbols
    return ATOM(_read_op(in));

  // Quoted symbol
  if (in.peek() == '\'')
  {
    in.get();
    std::string result;
    while (in and in.peek() != '\'')
      result += in.get();
    in.get();
    quote = true;
    return ATOM(result);
  }

  // Nonterminal
  if (std::isupper(in.peek()) or in.peek() == '_')
  {
    quote = true;
    return VAR(_read_word(in));
  }

  // String literal
  if (in.peek() == '"')
  {
    in.get();
    return strings.make_string(_read_string(in));
  }

  // Numerical literal
  if (std::isdigit(in.peek()))
    return NUM(_read_number(in));

  // Punctuations
  if (std::ispunct(in.peek()))
    return ATOM(std::string {char(in.get())});

  throw std::runtime_error {std::format("invalid symbol ({})", char(in.peek()))};

  #undef ATOM
  #undef VAR
  #undef NUM
}
