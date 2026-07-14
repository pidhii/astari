#include "iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/core/interpreter.hpp"


// unsigned + signed = signed
// unsigned + float = float
// signed + float = float
template <typename Op, typename OType>
object_view
eval(interpreter &pl, runtime &rt, int argc, object_iterator argv, OType acc, Op op = Op { })
{
  basic_decoder dc;
  basic_encoder ec;

  assert(argc >= 0);
  if (argc == 0)
  {
    word_t *p = rt.allocate(1);
    *p = ec.encode(acc);
    return {p, 1};
  }

  if (is_nonterminal(argv[0]))
  {
    nonterminal var;
    dc.decode(argv[0], var);
    if (auto xval = rt.dereference(var.id))
      argv = xval.value();
  }

  switch (word_type(argv[0]))
  {
    case word_type::signed_int_number:
    {
      int val;
      dc.decode(argv[0], val);
      if constexpr (std::is_same_v<OType, unsigned>)
        return eval<Op>(pl, rt, argc - 1, argv + 1, op(int(acc), int(val)), op);
      else if constexpr (std::is_same_v<OType, float>)
        return eval<Op>(pl, rt, argc - 1, argv + 1, op(float(acc), float(val)), op);
      else
        return eval<Op>(pl, rt, argc - 1, argv + 1, op(acc, val), op);
    }

    case word_type::unsigned_int_number:
    {
      unsigned val;
      dc.decode(argv[0], val);
      if constexpr (std::is_same_v<OType, int>)
        return eval<Op>(pl, rt, argc - 1, argv + 1, op(int(acc), int(val)), op);
      else if constexpr (std::is_same_v<OType, float>)
        return eval<Op>(pl, rt, argc - 1, argv + 1, op(float(acc), float(val)), op);
      else
        return eval<Op>(pl, rt, argc - 1, argv + 1, op(acc, val), op);
    }

    case word_type::float_number:
    {
      float val;
      dc.decode(argv[0], val);
      return eval<Op>(pl, rt, argc - 1, argv + 1, op(float(acc), float(val)), op);
    }

    default:
      pl.raise(term("type_error", term("number"), dc.decode_object(argv)));
  }
}

template <typename Op>
object_view
eval_(interpreter &pl, runtime &rt, int argc, object_iterator argv, Op op = Op {})
{
  basic_decoder dc;

  if (argc <= 0)
    pl.raise(term("arity_error", term("arithmetics")));

  if (is_nonterminal(argv[0]))
  {
    nonterminal var;
    dc.decode(argv[0], var);
    if (auto xval = rt.dereference(var.id))
      argv = xval.value();
    else
      pl.raise(term("instantiation_error"));
  }

  switch (word_type(argv[0]))
  {
    case word_type::signed_int_number:
    {
      int val;
      dc.decode(argv[0], val);
      return eval<Op>(pl, rt, argc - 1, argv + 1, val, op);
    }

    case word_type::unsigned_int_number:
    {
      unsigned val;
      dc.decode(argv[0], val);
      return eval<Op>(pl, rt, argc - 1, argv + 1, val, op);
    }

    case word_type::float_number:
    {
      float val;
      dc.decode(argv[0], val);
      return eval<Op>(pl, rt, argc - 1, argv + 1, val, op);
    }

    default:
      pl.raise(term("type_error", term("number"), dc.decode_object(argv)));
  }
}


struct op_add {
  template <typename T>
  T operator () (T lhs, T rhs) { return lhs + rhs; }
};

struct op_sub {
  template <typename T>
  T operator () (T lhs, T rhs) { return lhs - rhs; }
};

struct op_mul {
  template <typename T, typename U>
  auto operator () (T lhs, U rhs) { return lhs * rhs; }
};

struct op_fdiv {
  template <typename T, typename U>
  float operator () (T lhs, U rhs) { return float(lhs) / float(rhs); }
};

struct op_idiv {
  interpreter &pl;

  template <typename T, typename U>
  auto operator () (T lhs, U rhs)
  {
    if (std::is_same_v<std::remove_cvref_t<T>, float>)
      pl.raise(term("type_error", term("integer"), lhs));
    else if (std::is_same_v<std::remove_cvref_t<U>, float>)
      pl.raise(term("type_error", term("integer"), rhs));
    else
      return lhs / rhs;
  }
};


void
iso_arithmetics(interpreter &pl)
{
  pl.add_meta_op("sum", [&](runtime &rt, int argc, object_iterator argv,
                            const continuation &cont) {
    basic_decoder dc;
    const object_view x = eval<op_add>(pl, rt, argc - 1, argv, 0u);
    if (rt.match(x, dc.decode_object(argv + argc - 1)))
      cont(rt);
  });
  pl.add_meta_op("sub", [&](runtime &rt, int argc, object_iterator argv,
                            const continuation &cont) {
    basic_decoder dc;
    const object_view x = eval_<op_sub>(pl, rt, argc - 1, argv);
    if (rt.match(x, dc.decode_object(argv + argc - 1)))
      cont(rt);
  });
  pl.add_meta_op("prod", [&](runtime &rt, int argc, object_iterator argv,
                             const continuation &cont) {
    basic_decoder dc;
    const object_view x = eval<op_mul>(pl, rt, argc - 1, argv, 1u);
    if (rt.match(x, dc.decode_object(argv + argc - 1)))
      cont(rt);
  });
  pl.add_meta_op("fdiv", [&](runtime &rt, int argc, object_iterator argv,
                            const continuation &cont) {
    basic_decoder dc;
    const object_view x = eval_<op_fdiv>(pl, rt, argc - 1, argv);
    if (rt.match(x, dc.decode_object(argv + argc - 1)))
      cont(rt);
  });
  pl.add_meta_op("idiv", [&](runtime &rt, int argc, object_iterator argv,
                            const continuation &cont) {
    basic_decoder dc;
    const object_view x = eval_(pl, rt, argc - 1, argv, op_idiv {pl});
    if (rt.match(x, dc.decode_object(argv + argc - 1)))
      cont(rt);
  });


#define DEFINE_CMP(name, op)                                                   \
  pl.add_meta_op(name, [&](runtime &rt, int argc, object_iterator argv,        \
                           const continuation &cont) {                         \
    assert_arity(pl, name, argc, 2);                                           \
    pl.number(rt, argv + 0, [&](auto &&lhs) {                                  \
      pl.number(rt, argv + 1, [&](auto &&rhs) {                                \
        if (lhs op rhs)                                                        \
          cont(rt);                                                            \
      });                                                                      \
    });                                                                        \
  });
  DEFINE_CMP("numeq", ==)
  DEFINE_CMP("numne", !=)
  DEFINE_CMP("numlt", <)
  DEFINE_CMP("numgt", >)
  DEFINE_CMP("numle", <=)
  DEFINE_CMP("numge", >=)

  pl << R"(
    Result is Expr :-
      number(Expr)  -> Result = Expr;
      Expr = X + Y  -> Lhs is X, Rhs is Y, sum(Lhs, Rhs, Result);
      Expr = X - Y  -> Lhs is X, Rhs is Y, sub(Lhs, Rhs, Result);
      Expr = X * Y  -> Lhs is X, Rhs is Y, prod(Lhs, Rhs, Result);
      Expr = X / Y  -> Lhs is X, Rhs is Y, fdiv(Lhs, Rhs, Result);
      Expr = X // Y -> Lhs is X, Rhs is Y, idiv(Lhs, Rhs, Result);
      throw(type_error(evaluable, Expr)).

    X =:= Y :- Lhs is X, Rhs is Y, numeq(Lhs, Rhs).
    X =\= Y :- Lhs is X, Rhs is Y, numne(Lhs, Rhs).
    X < Y   :- Lhs is X, Rhs is Y, numlt(Lhs, Rhs).
    X > Y   :- Lhs is X, Rhs is Y, numgt(Lhs, Rhs).
    X =< Y  :- Lhs is X, Rhs is Y, numle(Lhs, Rhs).
    X >= Y  :- Lhs is X, Rhs is Y, numge(Lhs, Rhs).
  )";
}