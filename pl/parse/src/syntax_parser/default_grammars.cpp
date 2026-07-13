#include "lexer.hpp"
#include "pl/dictionary.hpp"
#include "syntax_parser.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"

#include <functional>


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

struct inbetween: grammar {
  template <typename Functor>
  inbetween(int op1, int a, int op2, int assoc, Functor f)
  : grammar(3, assoc),
    m_op1 {op1},
    m_a {a},
    m_op2 {op2},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    if (it[0].type == m_op1 and it[1].type == m_a and it[2].type == m_op2)
    {
      result = m_f(std::get<object>(it[1].val));
      return true;
    }
    else
      return false;
  }

  private:
  int m_op1, m_a, m_op2;
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


struct generic_grammar: grammar {
  template <typename Functor, typename ...Types>
  generic_grammar(int assoc, Functor f, Types ...types)
  : grammar(sizeof...(types), assoc),
    m_types {types...},
    m_f {f}
  { }

  bool
  apply(token_iterator it, token &result) override
  {
    for (size_t i = 0; i < size; ++i)
    {
      if (it[i].type != m_types[i])
        return false;
    }

    result = m_f(it);
    return true;
  }

  private:
  std::vector<int> m_types;
  std::function<token(token_iterator)> m_f;
};


void
load_default_grammar(syntax_parser &stxparser)
{
  dictionary &symdict = stxparser.symbols();

  #define STR(tok) std::get<std::string>(tok.val)
  #define OBJ(tok) std::get<object>(tok.val)
  #define SYM(sym) symdict[sym]
  #define TERM(sym, arity) basic_encoder().encode(term_header(SYM(sym), arity))
  #define LEN(obj) basic_decoder().count_objects(object_view(obj).begin(), object_view(obj).end())

  // binary operators
  for (auto optype : {mullike, pluslike, cmplike, assignlike})
  {
    stxparser.add_grammar<generic_grammar>(
      left, [&](token_iterator it) -> token {
        object result;
        result += TERM(STR(it[1]), 2);
        result += OBJ(it[0]);
        result += OBJ(it[2]);
        return token {obj, result};
      }, obj, optype, obj
    );
  }

  stxparser.add_grammar<generic_grammar>(
    left, [&](token_iterator it) -> token {
      object result;
      result += TERM(STR(it[0]), 0);
      return {obj, result};
    }, terminal_symbol, '(', ')'
  );
  stxparser.add_grammar<generic_grammar>(
    left, [&](token_iterator it) -> token {
      object result;
      result += TERM(STR(it[0]), 1);
      result += OBJ(it[2]);
      return {obj, result};
    }, terminal_symbol, '(', obj, ')'
  );
  stxparser.add_grammar<generic_grammar>(
    left, [&](token_iterator it) -> token {
      object result;
      result += TERM(STR(it[0]), LEN(OBJ(it[2])));
      result += OBJ(it[2]);
      return {obj, result};
    }, terminal_symbol, '(', andseq, ')'
  );
  stxparser.add_grammar<generic_grammar>(
    left, [&](token_iterator it) -> token {
      object result;
      result += TERM("", 0);
      return {obj, result};
    }, '(', ')'
  );
  stxparser.add_grammar<generic_grammar>(
    left, [&](token_iterator it) -> token {
      object result;
      result += TERM("", 1);
      result += OBJ(it[1]);
      return {obj, result};
    }, '(', obj, ')'
  );
  stxparser.add_grammar<generic_grammar>(
    left, [&](token_iterator it) -> token {
      object result;
      result += TERM("", LEN(OBJ(it[1])));
      result += OBJ(it[1]);
      return {obj, result};
    }, '(', andseq, ')'
  );

  // cons-lists
  stxparser.add_grammar<inbetween>(
    '[', obj, ']', left,
    [&](const object &x) -> token {
      object result;
      result += TERM("cons", 2);
      result += x;
      result += TERM("nil", 0);
      return {obj, result};
    }
  );
  stxparser.add_grammar<inbetween>(
    '[', cons, ']', left,
    [&](const object &x) -> token {
      return {obj, x};
    }
  );
  stxparser.add_grammar<inbetween>(
    '[', andseq, ']', left,
    [&](object_view x) -> token {
      object result;
      size_t n = LEN(x);
      auto it = x.begin();
      while (n--)
      {
        result += TERM("cons", 2);
        result += basic_decoder().decode_object(it);
      }
      result += TERM("nil", 0);
      return {obj, result};
    }
  );
  stxparser.add_grammar<binary_operator>(
    obj, '|', obj, left,
    [&](const object &lhs, const object &rhs) -> token {
      object result;
      result += TERM("cons", 2);
      result += lhs;
      result += rhs;
      return token {cons, result};
    }
  );
  stxparser.add_grammar<binary_operator>(
    obj, ',', cons, left,
    [&](const object &lhs, const object &rhs) -> token {
      object result;
      result += TERM("cons", 2);
      result += lhs;
      result += rhs;
      return token {cons, result};
    }
  );


  // andseq (',')
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

  // unary operators
  for (auto optype : {pluslike, mullike, cmplike, assignlike})
  {
    stxparser.add_grammar<generic_grammar>(
      left, [&](token_iterator it) -> token {
        object result;
        result += TERM(STR(it[0]), 1);
        result += OBJ(it[1]);
        return token {obj, result};
      }, optype, obj
    );
  }


  // andseq -> and(andseq)
  stxparser.add_grammar<simple_grammar>(
      andseq, left,
      [&](const object_view &x) -> token {
        object result;
        result += TERM("", LEN(x));
        result += x;
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
        result += TERM("if", 3);
        result += ifthen;
        result += TERM("or", LEN(orseq));
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
        result += TERM("if", 3);
        result += ifthen;
        result += els;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      orseq, left,
      [&](object_view x) -> token {
        object result;
        result += TERM("or", LEN(x));
        result += x;
        return token {obj, result};
      }
  );
  stxparser.add_grammar<simple_grammar>(
      ifthen, left,
      [&](object_view ifthen) -> token {
        object result;
        result += TERM("if", 3);
        result += ifthen;
        result += TERM("fail", 0);
        return token {obj, result};
      }
  );

  // :-
  // signature :- body .
  stxparser.add_grammar<subternary_operator>(
      obj, colon_minus, obj, '.', left,
      [&](object_view sign, object_view body) -> token {
        object result;
        result += sign;
        result += body;
        return token {predicate, result};
      }
  );
  // :- directive .
  stxparser.add_grammar<inbetween>(
    colon_minus, obj, '.', left,
    [&](const object &x) -> token {
      return {directive, x};
    }
  );

  // signature .
  stxparser.add_grammar<unary_sufix_operator>(
      obj, '.', left,
      [&](const object &sign) -> token {
        return token {statement, sign};
      }
  );
}