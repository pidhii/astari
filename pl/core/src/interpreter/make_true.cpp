#include "interpreter.hpp"
#include "match.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/term_utils.hpp"
#include "pl/obj/object.hpp"

#include <iostream>


#define PLUG 0

object_view
_obj(object_iterator e)
{
  basic_decoder dc;
  return dc.decode_object(e);
}


continuation
interpreter::_make_true(runtime &rt, size_t, object_iterator e, barrier *clause,
                        const continuation &cont)
{
  static basic_decoder dc;
  // std::clog << "[make_true] " << dump(rt.reconstruct(_obj(e))) << std::endl;

  switch (word_type(e[0]))
  {
    case word_type::structure:
    {
      term_header hdr;
      dc.decode(e[0], hdr);
      switch (hdr.id)
      {
        case op_and:
          return _make_true__and(rt, hdr.arity, e + 1, clause, cont);

        case op_or:
          return _make_true__or(rt, hdr.arity, e + 1, clause, cont);

        case op_if:
          assert(hdr.arity == 3);
          return _make_true__if(rt, PLUG, e + 1, clause, cont);

        case op_cut:
          assert(hdr.arity == 0);
          assert(clause);
          rt.cut_exc(clause);
          return cont;

        case op_fail:
          assert(hdr.arity == 0);
          return continuation {};

        default: // predicate
          return _make_true__predicate(rt, PLUG, e, clause, cont);
      }
    }

    case word_type::nonterminal:
    {
      nonterminal var;
      dc.decode(e[0], var);
      if (auto val = rt.dereference(var.id))
        return _make_true(rt, PLUG, val.value(), clause, cont);
      else
        raise(*this, term("instantiation_error"));
    }

    default:
      raise(*this, term("type_error", term("callable"), dc.decode_object(e)));
  }
}


continuation
interpreter::_make_true__and(runtime &rt, size_t i, object_iterator eit,
                             barrier *clause, const continuation &cont)
{
  // std::clog << "[make_true/and] " << dump(rt.reconstruct(_obj(eit))) << std::endl;
  basic_decoder dc;
  if (i == 1)
    return _make_true(rt, PLUG, eit, clause, cont);
  if (i > 0)
  {
    const object_iterator e = eit;
    dc.decode_object(eit); // call for side-effects
    continuation thencont = continuation::from_lambda(
        // [this, i, eit, cont = std::move(cont), clause](CONT_ARGS) {
        [this, i, eit, cc=std::move(cont), clause](CONT_ARGS) {
          return _make_true__and(rt, i - 1, eit, clause, cc)
              .reinterpret<void()>();
        });
    return _make_true(rt, PLUG, e, clause, thencont);
  }
  else // i == 0
    return cont;
}


continuation
interpreter::_make_true__or(runtime &rt, size_t i, object_iterator eit,
                            barrier *clause, const continuation &cont)
{
  // std::clog << "[make_true/or] ..." << std::endl;
  basic_decoder dc;

  assert(i >= 1);
  while (i-- > 1) // Will taill-call on the last clause
  { 
    barrier cp;
    rt.push_choice_point(&cp);
    continuation cc = _make_true(rt, PLUG, eit, clause, cont);
    if (rt.driveuc(&cp, cc))
      return cc;
    rt.unwind(&cp);
    dc.decode_object(eit); // call for side-effects
  }
  return _make_true(rt, PLUG, eit, clause, cont);
}

// Soft cut version
// void
// interpreter::_make_true__if(runtime &rt, size_t _, object_iterator eit,
//                             const continuation &cont)
// {
//   basic_decoder dc;

//   word_t *condp = allocate(1);
//   *condp = 0;

//   const object_view econd = dc.decode_object(eit);
//   const object_view ethen = dc.decode_object(eit);
//   const object_view eelse = dc.decode_object(eit);

//   {
//     const continuation thencont = [this, condp, ethen, cont] (runtime &rt) {
//       *condp = 1;
//       return _make_true(rt, PLUG, ethen.begin(), cont); // TODO: tailcall
//     };

//     state_saver _ {rt};
//     _make_true(rt, PLUG, econd.begin(), thencont);
//   }

//   if (*condp == 0)
//     [[clang::musttail]] return _make_true(rt, PLUG, eelse.begin(), cont);
// }


// Strong cut version
continuation
interpreter::_make_true__if(runtime &rt, size_t _, object_iterator eit,
                            barrier *clause, const continuation &cont)
{
  // std::clog << "[make_true/if] ..." << std::endl;
  basic_decoder dc;

  const object_view econd = dc.decode_object(eit);
  const object_view ethen = dc.decode_object(eit);
  const object_view eelse = dc.decode_object(eit);

  barrier cp;
  rt.push_choice_point(&cp);

  continuation thencont = continuation::from_lambda(
      [this, &cp, cc=cont, ethen](CONT_ARGS) {
        rt.cut(&cp);
        return _make_true(rt, PLUG, ethen.begin(), &cp, cc)
            .reinterpret<void()>();
      });

  continuation cc = _make_true(rt, PLUG, econd.begin(), &cp, thencont);
  if (rt.driveuc(&cp, cc))
      return cc;

  rt.unwind(&cp);
  return _make_true(rt, PLUG, eelse.begin(), clause, cont);
}


#define MAKE_PREDICATE_TRUE(rt, e, pred, cont)                                 \
  const auto &[sign, body, n] = pred;                                          \
  if (not shallow_match(e.begin(), sign.data()))                               \
    continue;                                                                  \
                                                                               \
  barrier cp;                                                                  \
  rt.push_choice_point(&cp);                                                   \
                                                                               \
  const size_t base = rt.n_vars();                                             \
  const object_view predsign = rt.adopt_hp_n(base, sign);                      \
  rt.make_n_vars(n);                                                           \
  if (rt.match(e, predsign))                                                   \
  {                                                                            \
    if (not body.empty())                                                      \
    {                                                                          \
      const object_view predbody = rt.adopt_hp_n(base, body);                  \
      continuation cc = _make_true(rt, PLUG, predbody.begin(), cp.prev, cont); \
      if (rt.driveuc(&cp, cc))                                                 \
        return cc;                                                             \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      continuation cc = cont;                                                  \
      if (rt.driveuc(&cp, cc))                                                 \
        return cc;                                                             \
    }                                                                          \
  }                                                                            \
  rt.unwind(&cp);


#define MAKE_PREDICATE_TRUE_TC(rt, e, pred, cont)                              \
  const auto &[sign, body, n] = pred;                                          \
                                                                               \
  if (not shallow_match(e.begin(), sign.data()))                               \
    return continuation {};                                                    \
                                                                               \
  const size_t base = rt.n_vars();                                             \
  const object_view predsign = rt.adopt_hp_n(base, sign);                      \
  rt.make_n_vars(n);                                                           \
  if (rt.match(e, predsign))                                                   \
  {                                                                            \
    if (not body.empty())                                                      \
    {                                                                          \
      const object_view predbody = rt.adopt_hp_n(base, body);                  \
      return _make_true(rt, PLUG, predbody.begin(), m_query->cp, cont);        \
    }                                                                          \
    else                                                                       \
      return cont;                                                             \
  }                                                                            \
  return continuation {};


continuation
interpreter::_make_true__predicate(runtime &rt, size_t _, object_iterator e_,
                                   barrier *__, const continuation &cont)
{
  // std::clog << "[make_true/pred] " << dump(rt.reconstruct(_obj(e_))) << std::endl;
  static basic_decoder dc;

  const object_view e = dc.decode_object(e_);
  const word_t key = e[0] & term_mask;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //                        static predicates
  //
  if (const auto it = m_predicates.find(key); it != m_predicates.end())
  {
    const std::vector<predicate_entry> &variants = it->second;
    for (size_t i = 0; i < variants.size() - 1; ++i)
    { // NOTE: dont remove these curly brackets
      MAKE_PREDICATE_TRUE(rt, e, variants[i], cont);
    }
    // Tail-call on the last variant
    MAKE_PREDICATE_TRUE_TC(rt, e, variants.back(), cont);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //                             meta-ops
  //
  term_header hdr;
  dc.decode(e[0], hdr);
  if (const auto it = m_metaops.find(hdr.id); it != m_metaops.end())
    return it->second(rt, hdr.arity, e.begin() + 1, cont);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //                        dynamic predicates
  //
  if (auto it = rt.variants_begin(key); it != rt.variants_end())
  {
    auto nextit = std::next(it);
    for (; nextit != m_dyndb.end(key); it = nextit, ++nextit)
    { // NOTE: dont remove these curly brackets
      MAKE_PREDICATE_TRUE(rt, e, *it, cont)
    }
    // Tail-call on the last variant
    MAKE_PREDICATE_TRUE_TC(rt, e, *it, cont);
  }
  else if (is_dynamic(key))
    return FAIL; // absense of dynamic predicate definitions is not an error

  std::cerr << std::format("no such predicate ({}/{})", m_symdict[hdr.id],
                           hdr.arity)
            << std::endl;
  std::cerr << "static database:\n";
  debug();
  std::cerr << "dynamic database:\n";
  rt.print_dynamic_database(m_symdict);

  throw std::runtime_error {
      std::format("no such predicate ({}/{})", m_symdict[hdr.id], hdr.arity)};
}
