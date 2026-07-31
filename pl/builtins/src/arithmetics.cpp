#include "iso.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/misc/term_utils.hpp"
#include "pl/obj/object.hpp"

#include <cmath>


static struct {
  int operator () (int lhs, int rhs) { return lhs + rhs; }
  template <typename T, typename U>
  auto operator () (T lhs, U rhs) { return lhs + rhs; }
} ev_add;

static struct {
  template <typename T, typename U>
  auto operator () (T lhs, U rhs) { return lhs - rhs; }
} ev_sub;

static struct {
  template <typename T, typename U>
  auto operator () (T lhs, U rhs) { return lhs * rhs; }
} ev_mul;

static struct {
  template <typename T, typename U>
  float operator () (T lhs, U rhs) { return float(lhs) / float(rhs); }
} ev_fdiv;

static struct {
  template <typename T, typename U>
  auto operator () (T lhs, U rhs)
  {
    if (std::is_same_v<std::remove_cvref_t<T>, float>)
      throw std::runtime_error {"type error, integer"};
    else if (std::is_same_v<std::remove_cvref_t<U>, float>)
      throw std::runtime_error {"type error, integer"};
    else
      return lhs / rhs;
  }
} ev_idiv;


template <typename Op>
word_t
_eval(word_t lhs, word_t rhs, Op op = Op { })
{
  basic_decoder dc;
  basic_encoder ec;

  switch (word_type(lhs))
  {
    case word_type::signed_int_number:
    {
      int lhsval;
      dc.decode(lhs, lhsval);
      switch (word_type(rhs))
      {
        case word_type::signed_int_number:
        {
          int rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        case word_type::unsigned_int_number:
        {
          unsigned rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        case word_type::float_number:
        {
          float rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        default:
          throw std::runtime_error {"evaluation error"};
      }
    }

    case word_type::unsigned_int_number:
    {
      unsigned lhsval;
      dc.decode(lhs, lhsval);
      switch (word_type(rhs))
      {
        case word_type::signed_int_number:
        {
          int rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        case word_type::unsigned_int_number:
        {
          unsigned rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        case word_type::float_number:
        {
          float rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        default:
          throw std::runtime_error {"evaluation error"};
      }
    }

    case word_type::float_number:
    {
      float lhsval;
      dc.decode(lhs, lhsval);
      switch (word_type(rhs))
      {
        case word_type::signed_int_number:
        {
          int rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        case word_type::unsigned_int_number:
        {
          unsigned rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        case word_type::float_number:
        {
          float rhsval;
          dc.decode(rhs, rhsval);
          return ec.encode(op(lhsval, rhsval));
        }
        default:
          throw std::runtime_error {"evaluation error"};
      }
    }

    default:
      throw std::runtime_error {"evaluation error"};
  }
}

static void
_evaluate(interpreter &pl, runtime &rt, object_iterator e, size_t n,
          word_t *stack)
{
  basic_decoder dc;
  basic_encoder ec;
  term_header hdr;

#define TYPE_ERROR(e)                                                          \
  raise(pl, term("type_error", term("evaluable"), dc.decode_object(e)))

  while (n--)
  {
    if (is_number(*e))
    {
      *stack++ = *e;
      e++;
      continue;
    }

    if (is_nonterminal(*e))
    {
      nonterminal var;
      dc.decode(*e, var);
      if (auto val = rt.dereference(var.id))
      {
        _evaluate(pl, rt, *val, 1, stack);
        stack++;
        e++;
        continue;
      }
      else
        TYPE_ERROR(e);
    }

    dc.decode(*e, hdr);
    switch (hdr.id)
    {
      case op_plus:
        if (hdr.arity == 2)
        {
          _evaluate(pl, rt, e + 1, 2, stack);
          *stack = _eval(stack[0], stack[1], ev_add);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else if (hdr.arity == 1)
        {
          _evaluate(pl, rt, e + 1, 1, stack);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else
          TYPE_ERROR(e);

      case op_minus:
        if (hdr.arity == 2)
        {
          _evaluate(pl, rt, e + 1, 2, stack);
          *stack = _eval(stack[0], stack[1], ev_sub);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else if (hdr.arity == 1)
        {
          _evaluate(pl, rt, e + 1, 1, stack);
          const word_t zero = ec.encode(int(0));
          *stack = _eval(zero, stack[0], ev_sub);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else
          TYPE_ERROR(e);

      case op_mul:
        if (hdr.arity == 2)
        {
          _evaluate(pl, rt, e + 1, 2, stack);
          *stack = _eval(stack[0], stack[1], ev_mul);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else
          TYPE_ERROR(e);

      case op_div:
        if (hdr.arity == 2)
        {
          _evaluate(pl, rt, e + 1, 2, stack);
          *stack = _eval(stack[0], stack[1], ev_fdiv);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else
          TYPE_ERROR(e);

      case op_divdiv:
        if (hdr.arity == 2)
        {
          _evaluate(pl, rt, e + 1, 2, stack);
          *stack = _eval(stack[0], stack[1], ev_idiv);
          stack++;
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else
          TYPE_ERROR(e);

      case op_sqrt:
        if (hdr.arity == 1) 
        {
          _evaluate(pl, rt, e + 1, 1, stack);
          const double val =
              number(pl, stack, [](double x) { return std::sqrt(x); });
          *stack++ = ec.encode(float(val));
          dc.decode_object(e); // call for side-effects
          continue;
        }
        else
          TYPE_ERROR(e);

      default:
        TYPE_ERROR(e);
    }
  }
}

void NOINLINE 
_is(interpreter &pl, runtime &rt, size_t argc, object_iterator argv,
    continuation &cont)
{
  assert_arity(pl, "is", argc, 2);
  static basic_decoder dc;
  const object_view lhs = rt.reduce(dc.decode_object(argv));
  const object_iterator rhs = argv;

  _evaluate(pl, rt, rhs, 1, rt.query()->heap_p);
  const object_view result = {rt.query()->heap_p++, 1};
  if (rt.match(lhs, result))
    TAILCALL cont.call_tc(rt, 0, 0, 0, 0);
}

void iso_arithmetics(interpreter &pl)
{
  pl.add_meta_op("is", [&](runtime &rt, size_t argc, object_iterator argv,
                           continuation &cont) {
    TAILCALL _is(pl, rt, argc, argv, cont);
  });


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#define DEFINE_CMP(name, op)                                                   \
  pl.add_meta_op(name, [&](runtime &rt, size_t argc, object_iterator argv,     \
                           continuation &cont) {                               \
    assert_arity(pl, name, argc, 2);                                           \
    _evaluate(pl, rt, argv, 2, rt.query()->heap_p);                            \
    const bool ans = number(pl, rt.query()->heap_p + 0, [&](auto &&lhs) {      \
              return number(pl, rt.query()->heap_p + 1, [&](auto &&rhs) {      \
              return lhs op rhs; });});                                        \
    if (ans)                                                                   \
      TAILCALL cont.call_tc(rt, 0, 0, 0, 0);                                   \
  });
  DEFINE_CMP("=:=", ==)
  DEFINE_CMP("=\\=", !=)
  DEFINE_CMP("<", <)
  DEFINE_CMP(">", >)
  DEFINE_CMP("=<", <=)
  DEFINE_CMP(">=", >=)
#pragma GCC diagnostic pop
}