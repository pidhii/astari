#include "interpreter.hpp"
#include "match.hpp"

#include "pl/obj/object.hpp"
#include "utl/state_saver.hpp"

#include <iostream>


#define PLUG 0


void
interpreter::_make_true(runtime &rt, size_t, object_iterator e,
                        const continuation &cont)
{
  basic_decoder dc;

  switch (word_type(e[0]))
  {
    case word_type::structure:
    {
      basic_decoder dc;
      term_header hdr;
      dc.decode(e[0], hdr);
      switch (hdr.id)
      {
        case op_and:
          [[clang::musttail]] return _make_true__and(rt, hdr.arity, e + 1,
                                                     cont);

        case op_or:
          [[clang::musttail]] return _make_true__or(rt, hdr.arity, e + 1, cont);

        case op_if:
          assert(hdr.arity == 3);
          [[clang::musttail]] return _make_true__if(rt, PLUG, e + 1, cont);

        case op_fail:
          return;

        default: // predicate
          [[clang::musttail]] return _make_true__predicate(rt, PLUG, e, cont);
      }
    }

    case word_type::nonterminal:
    {
      basic_decoder dc;
      nonterminal var;
      dc.decode(e[0], var);
      if (auto val = rt.dereference(var.id))
        [[clang::musttail]] return _make_true(rt, PLUG, val.value(), cont);
      else
        raise(term("instantiation_error"));
    }

    default:
      raise(term("type_error", term("callable"), dc.decode_object(e)));
  }
}


void
interpreter::_make_true__and(runtime &rt, size_t i, object_iterator eit,
                             const continuation &cont)
{
  basic_decoder dc;
  if (i == 1)
    [[clang::musttail]] return _make_true(rt, PLUG, eit, cont);
  if (i > 0)
  {
    const object_iterator e = eit;
    dc.decode_object(eit); // call for side-effects
    return _make_true(rt, PLUG, e, [this, i, eit, cont](runtime &rt) {
      return _make_true__and(rt, i - 1, eit, cont); // TODO: tailcall
    });
  }
  else // i == 0
    return cont(rt); // TODO: tailcall
}


void
interpreter::_make_true__or(runtime &rt, size_t i, object_iterator eit,
                            const continuation &cont)
{
  basic_decoder dc;

  assert(i >= 1);
  while (i-- > 1) // Will taill-call on the last clause
  { 
    state_saver _ {rt};
    _make_true(rt, PLUG, eit, cont);
    dc.decode_object(eit); // call for side-effects
  }
  [[clang::musttail]] return _make_true(rt, PLUG, eit, cont);
}


void
interpreter::_make_true__if(runtime &rt, size_t _, object_iterator eit,
                            const continuation &cont)
{
  basic_decoder dc;

  const object_view econd = dc.decode_object(eit);
  const object_view ethen = dc.decode_object(eit);
  const object_view eelse = dc.decode_object(eit);

  object_view econt = eelse;
  {
    runtime contrt = rt;
    _make_true(rt, PLUG, econd.begin(), [&](runtime &rt) {
      contrt = rt;
      econt = ethen;
    });
    rt = contrt;
  }

  [[clang::musttail]] return _make_true(rt, PLUG, econt.begin(), cont);
}


void
interpreter::_make_true__predicate(runtime &rt, size_t _, object_iterator e_,
                                   const continuation &cont)
{
  static varnamespace ns;
  static matcher::memory mmem;

  basic_decoder dc;
  const object_view e = dc.decode_object(e_);

  const auto it = m_predicates.find(e[0]);
  if (it != m_predicates.end())
  {
    const std::vector<std::pair<object, object>> &variants = it->second;
    size_t n = variants.size();
    for (const auto &[sign, body] : variants)
    { // Tail-call on the last variant
      if (n-- == 1)
      {
        ns.clear();
        const object_view predsign = rt.adopt(ns, sign);
        if (mmem.clear(), matcher(rt, dc).match(e, predsign, mmem))
        {
          if (not body.empty())
          {
            const object_view predbody = rt.adopt(ns, body);
            [[clang::musttail]] return _make_true(rt, PLUG, predbody.begin(),
                                                  cont);
          }
          else
            return cont(rt); // TODO: tailcall
        }
        else
          rt.unallocate(predsign);
        return;
      }

      state_saver _ {rt};
      ns.clear();
      const object_view predsign = rt.adopt(ns, sign);
      if (mmem.clear(), matcher(rt, dc).match(e, predsign, mmem))
      {
        if (not body.empty())
        {
          const object_view predbody = rt.adopt(ns, body);
          _make_true(rt, PLUG, predbody.begin(), cont);
        }
        else
          cont(rt);
      }
      else
        rt.unallocate(predsign);
    }
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
    return it->second(rt, hdr.arity, e.begin() + 1, cont); // TODO: tailcall (?)
  }
}
