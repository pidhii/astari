#pragma once

#include "lexer.hpp"
#include "object_parser.hpp"

#include <memory>


using token_iterator = std::vector<token>::const_iterator;

enum associativity { left = -1, right = 1 };

struct grammar {
  grammar(size_t size, int assoc): size {size}, assoc {assoc} {}
  virtual ~grammar() = default;
  const size_t size;
  const int assoc;
  virtual bool apply(token_iterator, token &result) = 0;
};



class syntax_parser: public object_parser {
  using chunk = std::vector<token>;

  public:
  using object_parser::object_parser;

  template <typename Grammar, typename ...Args>
  void
  add_grammar(Args &&...args)
  { m_grammars.emplace_back(new Grammar {std::forward<Args>(args)...}); }

  template <typename TokIter>
  void
  load(TokIter tokit, TokIter tokend)
  {
    while (tokit < tokend)
    {
      try
      {
        TokIter tryit = tokit;
        object obj = parse_object(tryit, tokend);
        m_data.emplace_back(token_type::obj, std::move(obj));
        tokit = tryit;
      }
      catch (...)
      {
        m_data.push_back(*tokit++);
      }
    }
  }

  token
  parse();

  private:
  std::vector<std::unique_ptr<grammar>> m_grammars;
  std::vector<token> m_data;
};


void
load_default_grammar(syntax_parser &stxparser);

[[deprecated("Use load_default_grammar/1 instead.")]] inline void
load_default_grammar(dictionary &symdict, syntax_parser &stxparser)
{ load_default_grammar(stxparser); }
