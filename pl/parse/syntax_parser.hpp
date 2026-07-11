#pragma once

#include "lexer.hpp"
#include "object_parser.hpp"

#include <memory>
#include <functional>


using token_iterator = std::vector<token>::const_iterator;

enum associativity { left = -1, right = 1 };

struct grammar {
  grammar(size_t size, int assoc): size {size}, assoc {assoc} {}
  virtual ~grammar() = default;
  const size_t size;
  const int assoc;
  virtual bool apply(token_iterator, token &result) = 0;
};


struct simple_grammar: grammar {
  template <typename Functor>
  simple_grammar(int type, int assoc, Functor f)
  : grammar(1, assoc),
    m_type {type},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    if (it[0].type == m_type)
    {
      result = m_f(std::get<object>(it[0].val));
      return true;
    }
    else
      return false;
  }

  private:
  int m_type;
  std::function<token(const object&)> m_f;
};


struct unary_sufix_operator: grammar {
  template <typename Functor>
  unary_sufix_operator(int lhs, int op, int assoc, Functor f)
  : grammar(2, assoc),
    m_lhs {lhs},
    m_op {op},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    if (it[0].type == m_lhs and it[1].type == m_op)
    {
      result = m_f(std::get<object>(it[0].val));
      return true;
    }
    else
      return false;
  }

  private:
  int m_lhs, m_op;
  std::function<token(const object&)> m_f;
};


struct binary_operator: grammar {
  template <typename Functor>
  binary_operator(int lhs, int op, int rhs, int assoc, Functor f)
  : grammar(3, assoc),
    m_lhs {lhs},
    m_op {op},
    m_rhs {rhs},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    if (it[0].type == m_lhs and it[1].type == m_op and it[2].type == m_rhs)
    {
      result = m_f(std::get<object>(it[0].val), std::get<object>(it[2].val));
      return true;
    }
    else
      return false;
  }

  private:
  int m_lhs, m_op, m_rhs;
  std::function<token(const object&, const object&)> m_f;
};


struct subternary_operator: grammar {
  template <typename Functor>
  subternary_operator(int a, int op1, int b, int op2, int assoc, Functor f)
  : grammar(4, assoc),
    m_a {a},
    m_op1 {op1},
    m_b {b},
    m_op2 {op2},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    if (it[0].type == m_a and
        it[1].type == m_op1 and
        it[2].type == m_b and
        it[3].type == m_op2)
    {
      result = m_f(std::get<object>(it[0].val), std::get<object>(it[2].val));
      return true;
    }
    else
      return false;
  }

  private:
  int m_a, m_op1, m_b, m_op2;
  std::function<token(const object &, const object &)> m_f;
};


struct ternary_operator: grammar {
  template <typename Functor>
  ternary_operator(int a, int op1, int b, int op2, int c, int assoc, Functor f)
  : grammar(5, assoc),
    m_a {a},
    m_op1 {op1},
    m_b {b},
    m_op2 {op2},
    m_c {c},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    if (it[0].type == m_a and
        it[1].type == m_op1 and
        it[2].type == m_b and
        it[3].type == m_op2 and
        it[4].type == m_c)
    {
      result = m_f(std::get<object>(it[0].val),
                   std::get<object>(it[2].val),
                   std::get<object>(it[4].val));
      return true;
    }
    else
      return false;
  }

  private:
  int m_a, m_op1, m_b, m_op2, m_c;
  std::function<token(const object &, const object &, const object &)> m_f;
};


class syntax_parser: object_parser {
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
load_default_grammar(dictionary &symdict, syntax_parser &stxparser);
