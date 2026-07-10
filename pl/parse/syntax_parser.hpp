#pragma once

#include "lexer.hpp"
#include "object_parser.hpp"
#include "../coding/basic_decoder.hpp"
#include "../coding/basic_encoder.hpp"

#include <memory>
#include <functional>
#include <iostream>


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
  parse()
  {
    for (auto &g : m_grammars)
    {
      if (g->assoc > 0)
      {
        token_iterator s = m_data.end() - g->size;
        token_iterator e = m_data.end();
        while (s >= m_data.begin())
        {
          token result;
          if (g->apply(s, result))
          {
            m_data.erase(s, e);
            m_data.insert(s, result);
            return parse();
          }
          s--;
          e--;
        }
      }
      else
      {
        token_iterator s = m_data.begin();
        token_iterator e = m_data.begin() + g->size;
        while (e <= m_data.end())
        {
          token result;
          if (g->apply(s, result))
          {
            m_data.erase(s, e);
            m_data.insert(s, result);
            return parse();
          }
          s++;
          e++;
        }
      }
    }

    if (m_data.size() != 1)
      throw std::runtime_error {"parsing did not terminate"};
    token result = std::move(m_data[0]);
    m_data.clear();
    return result;
  }

  private:
  std::vector<std::unique_ptr<grammar>> m_grammars;
  std::vector<token> m_data;
};


static void
load_default_grammar(dictionary &symdict, syntax_parser &stxparser)
{
  basic_encoder ec;
  basic_decoder dc;

  #define SYM(sym) symdict[#sym]
  #define TERM(sym, arity) ec.encode(term_header(SYM(sym), arity))

  // ','
  stxparser.add_grammar<binary_operator>(
      andseq, ',', obj, left,
      [](const object &lhs, const object &rhs) -> token {
        return token {andseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<binary_operator>(
      obj, ',', obj, left,
      [](const object &lhs, const object &rhs) -> token {
        return token {andseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      andseq, left,
      [&](const object_view &x) -> token {
        const size_t id = symdict[""];
        size_t arity = 0;
        for (auto it = x.begin(); it != x.end(); arity++)
          dc.decode_object(it);
        object result;
        result.push_back(ec.encode(term_header(id, arity)));
        result.append(x);
        return token {obj, result};
      }
  );

  // '->'
  stxparser.add_grammar<binary_operator>(
      obj, rarrow, obj, right,
      [&](const object &a, const object &b) -> token {
        return token {ifthen, a + b};
      }
  );

  // ';'
  stxparser.add_grammar<binary_operator>(
      obj, ';', orseq, right,
      [&](const object &lhs, const object &rhs) -> token {
        return token {orseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<binary_operator>(
      ifthen, ';', orseq, right,
      [&](const object &ifthen, const object_view &orseq) -> token {
        object result;
        result += TERM(if, 3);
        result += ifthen;
        result += TERM(or, dc.count_objects(orseq.begin(), orseq.end()));
        result += orseq;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<binary_operator>(
      obj, ';', obj, right,
      [&](const object &lhs, const object &rhs) -> token {
        return token {orseq, lhs + rhs};
      }
  );
  stxparser.add_grammar<binary_operator>(
      ifthen, ';', obj, right,
      [&](object_view ifthen, object_view els) -> token {
        object result;
        result += TERM(if, 3);
        result += ifthen;
        result += els;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      orseq, left,
      [&](object_view x) -> token {
        object result;
        result += TERM(or, dc.count_objects(x.begin(), x.end()));
        result += x;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      ifthen, left,
      [&](object_view ifthen) -> token {
        object result;
        result += TERM(if, 3);
        result += ifthen;
        result += TERM(fail, 0);
        return token {obj, result};
      }
  );

  // :-
  stxparser.add_grammar<subternary_operator>(
      obj, colon_minus, obj, '.', left,
      [&](object_view sign, object_view body) -> token {
        object result;
        result += sign;
        result += body;
        return token {predicate, result};
      }
  );
  stxparser.add_grammar<unary_sufix_operator>(
      obj, '.', left,
      [&](const object &sign) -> token {
        return token {statement, sign};
      }
  );
}