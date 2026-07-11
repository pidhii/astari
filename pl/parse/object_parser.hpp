#pragma once

#include "lexer.hpp"

#include "pl/coding/basic_encoder.hpp"
#include "pl/dictionary.hpp"

#include <format>
#include <sstream>
#include <vector>


class object_parser {
  public:
  object_parser(dictionary &symdict, dictionary &vardict)
  : m_vardict {vardict},
    m_symdict {symdict}
  { }

  dictionary &
  symbols() noexcept
  { return m_symdict; }

  dictionary &
  variables() noexcept
  { return m_vardict; }

  object
  parse_object(const std::string_view text)
  {
    // Tokenize
    std::istringstream textstream {std::string {text}};
    std::vector<token> tokens;
    lexer lex;
    lex.tokenize(textstream, std::back_inserter(tokens));

    // Parse
    std::basic_string<word_t> result;
    auto tokbegin = tokens.begin();
    auto oit = std::back_inserter(result);
    _parse_object(tokbegin, tokens.end(), oit);

    return result;
  }

  template <typename TokIter>
  object
  parse_object(TokIter &tokit, TokIter tokend)
  {
    // Parse
    std::basic_string<word_t> result;
    auto oit = std::back_inserter(result);
    _parse_object(tokit, tokend, oit);

    return result;
  }

  private:
  template <typename TokIter, typename OIter>
  size_t
  _parse_list(TokIter &tokit, TokIter tokend, OIter &oit, const token &endtok)
  {
    const auto get = [&]() -> const token& { return _get(tokit, tokend); };
    const auto peek = [&]() -> const token& { return _peek(tokit, tokend); };

    // Empty list
    if (peek() == endtok)
      return 0;

    // Non-empty list
    size_t n = 0;
    while (true)
    {
      _parse_object(tokit, tokend, oit); // parse next field
      n += 1;

      const token delim = peek();
      if (delim.type == ',')
      { // skip comma and continue
        get();
        continue;
      }
      else if (delim.type == endtok.type)
      {
        // finish without consuming the end-token
        return n;
      }
      else
      {
        throw std::runtime_error {std::format(
            "unexpected token ({})", std::get<std::string>(delim.val))};
      }
    }
  }

  template <typename TokIter, typename OIter>
  void
  _parse_object(TokIter &tokit, TokIter tokend, OIter &oit)
  {
    const auto get = [&]() -> const token& { return _get(tokit, tokend); };
    const auto peek = [&]() -> const token& { return _peek(tokit, tokend); };

    size_t termid;
    switch (const token tok = get(); tok.type)
    {
      case nonterminal_symbol:
      {
        *oit++ = m_encoder.encode(nonterminal(m_vardict[std::get<std::string>(tok.val)]));
        return;
      }

      case '(':
        termid = m_symdict[""];
        goto l_members;
        // fallthrough
      case terminal_symbol:
      {
        termid = m_symdict[std::get<std::string>(tok.val)];
        if (peek().type == '(') // List of fields for compound term
        {
          get(); // skip the bracket

l_members:
          // Parse fields
          std::vector<word_t> fields; // buffer for fields
          auto fldit = std::back_inserter(fields);
          const size_t arity =
              _parse_list(tokit, tokend, fldit, {')', ")"});
          get(); // skip closing bracket

          // Encode the compisite token
          *oit++ = m_encoder.encode(term_header(termid, arity));
          std::copy(fields.begin(), fields.end(), oit);
        }
        else // Atomic token
          *oit++ = m_encoder.encode(term_header(termid, 0));

        return;
      }

      case number:
      {
        size_t pos;

        try {
          const int32_t v = std::stoi(std::get<std::string>(tok.val), &pos);
          if (pos == std::get<std::string>(tok.val).size())
          {
            *oit++ = m_encoder.encode(v);
            return;
          }
        }
        catch (...) { }

        try {
          const float v = std::stof(std::get<std::string>(tok.val), &pos);
          if (pos == std::get<std::string>(tok.val).size())
          {
            *oit++ = m_encoder.encode(v);
            return;
          }
        }
        catch (...) { }
      }

      // fall through
      default:
        throw std::runtime_error {
            std::format("unexpected token ({})", std::get<std::string>(tok.val))};
    }
  }

  private:
  template <typename Iter>
  const token&
  _get(Iter &it, Iter end)
  {
    static token eof {token_type::eof, ""};
    if (it >= end)
      return eof;
    else
      return *it++;
  }

  template <typename Iter>
  const token&
  _peek(Iter it, Iter end)
  {
    static token eof {token_type::eof, ""};
    if (it >= end)
      return eof;
    else
      return *it;
  }

  protected:
  dictionary &m_vardict;
  dictionary &m_symdict;
  basic_encoder m_encoder;
};
