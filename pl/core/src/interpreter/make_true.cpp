#include "interpreter.hpp"
#include "match.hpp"

#include "pl/obj/object.hpp"
#include "utl/state_saver.hpp"

#include <iostream>


#define PLUG 0



void
interpreter::_make_true(runtime &rt, size_t, object_iterator e, barrier *clause,
                        continuation &cont)
{
  static basic_decoder dc;

  switch (word_type(e[0]))
  {
    case word_type::structure:
    {
      term_header hdr;
      dc.decode(e[0], hdr);
      switch (hdr.id)
      {
        case op_and:
          TAILCALL _make_true__and(rt, hdr.arity, e + 1, clause, cont);

        case op_or:
          TAILCALL _make_true__or(rt, hdr.arity, e + 1, clause, cont);

        case op_if:
          assert(hdr.arity == 3);
          TAILCALL _make_true__if(rt, PLUG, e + 1, clause, cont);

        case op_cut:
          assert(hdr.arity == 0);
          assert(clause);
          rt.cut_exc(clause);
          TAILCALL cont(rt);

        case op_fail:
          assert(hdr.arity == 0);
          return;

        default: // predicate
          TAILCALL _make_true__predicate(rt, PLUG, e, clause, cont);
      }
    }

    case word_type::nonterminal:
    {
      nonterminal var;
      dc.decode(e[0], var);
      if (auto val = rt.dereference(var.id))
        TAILCALL _make_true(rt, PLUG, val.value(), clause, cont);
      else
        raise(term("instantiation_error"));
    }

    default:
      raise(term("type_error", term("callable"), dc.decode_object(e)));
  }
}


void
interpreter::_make_true__and(runtime &rt, size_t i, object_iterator eit,
                             barrier *clause, continuation &cont)
{
  basic_decoder dc;
  if (i == 1)
    TAILCALL _make_true(rt, PLUG, eit, clause, cont);
  if (i > 0)
  {
    const object_iterator e = eit;
    dc.decode_object(eit); // call for side-effects
    cont = [this, i, eit, cont, clause](runtime &rt) mutable {
      TAILCALL _make_true__and(rt, i - 1, eit, clause, cont);
    };
    TAILCALL _make_true(rt, PLUG, e, clause, cont);
  }
  else // i == 0
    TAILCALL cont(rt);
}


void
interpreter::_make_true__or(runtime &rt, size_t i, object_iterator eit,
                            barrier *clause, continuation &cont)
{
  basic_decoder dc;

  assert(i >= 1);
  while (i-- > 1) // Will taill-call on the last clause
  { 
    state_saver _ {cont};
    barrier cp;
    rt.push_choice_point(&cp);
    _make_true(rt, PLUG, eit, clause, cont);
    if (rt.uwuc(&cp))
      return;

    dc.decode_object(eit); // call for side-effects
  }
  TAILCALL _make_true(rt, PLUG, eit, clause, cont);
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
void
interpreter::_make_true__if(runtime &rt, size_t _, object_iterator eit,
                            barrier *clause, continuation &cont)
{
  basic_decoder dc;

  const object_view econd = dc.decode_object(eit);
  const object_view ethen = dc.decode_object(eit);
  const object_view eelse = dc.decode_object(eit);

  bool cond = false;
  barrier cp;
  rt.push_choice_point(&cp);
  {
    continuation condcont = [&](runtime &rt) {
      rt.cut(&cp);
      cond = true;
    };
    _make_true(rt, PLUG, econd.begin(), &cp, condcont);
  }

  if (cond)
  { // Don't unwind and proceed with the then clause.
    rt.pop_choice_point(&cp);
    TAILCALL _make_true(rt, PLUG, ethen.begin(), clause, cont);
  }
  else
  { // Unwind unifications from the cond clause and go into the else clause.
    rt.unwind(&cp);
    TAILCALL _make_true(rt, PLUG, eelse.begin(), clause, cont);
  }
}


void
interpreter::_make_true__predicate(runtime &rt, size_t _, object_iterator e_,
                                   barrier *__, continuation &cont)
{
  static basic_decoder dc;

  const object_view e = dc.decode_object(e_);

  const auto it = m_predicates.find(e[0] & term_mask);
  if (it != m_predicates.end())
  {
    const std::vector<predicate_entry> &variants = it->second;
    for (size_t i = 0; i < variants.size() - 1; ++i)
    {
      const auto &[sign, body, n] = variants[i];

      if (not shallow_match(e.begin(), sign.data()))
        continue;

      barrier cp;
      rt.push_choice_point(&cp);

      const size_t base = rt.n_vars();
      const object_view predsign = rt.adopt_hp_n(base, sign);
      rt.make_n_vars(n);
      if (rt.match(e, predsign))
      {
        state_saver _ {cont};
        if (not body.empty())
        {
          const object_view predbody = rt.adopt_hp_n(base, body);
          _make_true(rt, PLUG, predbody.begin(), cp.prev, cont);
        }
        else
          cont(rt);
      }

      if (rt.uwuc(&cp))
        return;
    }

    // Tail-call on the last variant
    const auto &[sign, body, n] = variants.back();

    if (not shallow_match(e.begin(), sign.data()))
      return;

    const size_t base = rt.n_vars();
    const object_view predsign = rt.adopt_hp_n(base, sign);
    rt.make_n_vars(n);
    if (rt.match(e, predsign))
    {
      if (not body.empty())
      {
        const object_view predbody = rt.adopt_hp_n(base, body);
        TAILCALL _make_true(rt, PLUG, predbody.begin(), m_query->cp, cont);
      }
      else
        TAILCALL cont(rt);
    }
    return;
  }
  else
  {
    term_header hdr;
    dc.decode(e[0], hdr);
    const auto it = m_metaops.find(hdr.id);
    if (it == m_metaops.end())
    {
      std::cerr << std::format("no such predicate ({}/{})", m_symdict[hdr.id],
                               hdr.arity)
                << std::endl;
      for (const auto &[w, _] : m_predicates)
      {
        term_header hdr;
        dc.decode(w, hdr);
        std::cerr << std::format("  have {}/{}", m_symdict[hdr.id], hdr.arity)
                  << std::endl;
      }

      throw std::runtime_error {std::format("no such predicate ({}/{})",
                                            m_symdict[hdr.id], hdr.arity)};
    }
    TAILCALL it->second(rt, hdr.arity, e.begin() + 1, cont);
  }
}
