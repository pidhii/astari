#include "iso.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/misc/term_utils.hpp"
#include "pl/obj/object.hpp"


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
_evaluate(runtime &rt, object_iterator e, size_t n, word_t *stack)
{
  basic_decoder dc;
  term_header hdr;

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
        _evaluate(rt, *val, 1, stack);
        stack++;
        e++;
        continue;
      }
      else
        throw std::runtime_error {"evaulation error"};
    }

    dc.decode(*e, hdr);
    switch (hdr.id)
    {
      case op_plus:
      {
        _evaluate(rt, e + 1, 2, stack);
        *stack = _eval(stack[0], stack[1], ev_add);
        stack++;
        dc.decode_object(e); // call for side-effects
        continue;
      }
      case op_minus:
      {
        _evaluate(rt, e + 1, 2, stack);
        *stack = _eval(stack[0], stack[1], ev_sub);
        stack++;
        dc.decode_object(e); // call for side-effects
        continue;
      }
      case op_mul:
      {
        _evaluate(rt, e + 1, 2, stack);
        *stack = _eval(stack[0], stack[1], ev_mul);
        stack++;
        dc.decode_object(e); // call for side-effects
        continue;
      }
      case op_div:
      {
        _evaluate(rt, e + 1, 2, stack);
        *stack = _eval(stack[0], stack[1], ev_fdiv);
        stack++;
        dc.decode_object(e); // call for side-effects
        continue;
      }
      case op_divdiv:
      {
        _evaluate(rt, e + 1, 2, stack);
        *stack = _eval(stack[0], stack[1], ev_idiv);
        stack++;
        dc.decode_object(e); // call for side-effects
        continue;
      }
      default:
        throw std::runtime_error {"evaluation error"};
    }
  }
}

void
iso_arithmetics(interpreter &pl)
{
  pl.add_meta_op("is", [&](runtime &rt, int argc, object_iterator argv,
                           continuation &cont) {
    assert_arity(pl, "is", argc, 2);
    basic_decoder dc;
    const object_view lhs = dc.decode_object(argv);
    const object_iterator rhs = argv;

    _evaluate(rt, rhs, 1, rt.query()->heap_p);
    const object_view result = {rt.query()->heap_p++, 1};
    if (rt.match(lhs, result))
      TAILCALL cont(rt);
  });


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#define DEFINE_CMP(name, op)                                                   \
  pl.add_meta_op(name, [&](runtime &rt, int argc, object_iterator argv,        \
                           continuation &cont) {                               \
    assert_arity(pl, name, argc, 2);                                           \
    _evaluate(rt, argv, 2, rt.query()->heap_p);                                \
    const bool ans = number(pl, rt.query()->heap_p + 0, [&](auto &&lhs) {      \
              return number(pl, rt.query()->heap_p + 1, [&](auto &&rhs) {      \
              return lhs op rhs; });});                                        \
    if (ans)                                                                   \
      TAILCALL cont(rt);                                                       \
  });
  DEFINE_CMP("=:=", ==)
  DEFINE_CMP("=\\=", !=)
  DEFINE_CMP("<", <)
  DEFINE_CMP(">", >)
  DEFINE_CMP("=<", <=)
  DEFINE_CMP(">=", >=)
#pragma GCC diagnostic pop

  // pl.load_objfile(PLO_PATH_iso_arithmetics);
}