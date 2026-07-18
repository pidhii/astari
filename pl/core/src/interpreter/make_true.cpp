#include "interpreter.hpp"
#include "match.hpp"

#include "pl/misc/display.hpp"
#include "pl/obj/object.hpp"
#include "utl/state_saver.hpp"

#include <iostream>


void
interpreter::_make_true(runtime &rt, object_view e, const continuation &cont)
{
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
          _make_true__and(rt, hdr.arity, e.begin() + 1, cont);
          return;

        case op_or:
          _make_true__or(rt, hdr.arity, e.begin() + 1, cont);
          return;

        case op_if:
          assert(hdr.arity == 3);
          if (hdr.arity == 3)
            _make_true__if(rt, e.begin() + 1, cont);
          else
            _make_true__and(rt, 2, e.begin() + 1, cont);
          return;

        case op_fail:
          return;

        default: // predicate
          _make_true__predicate(rt, e, cont);
          return;
      }
    }

    case word_type::nonterminal:
    {
      basic_decoder dc;
      nonterminal var;
      dc.decode(e[0], var);
      if (auto val = rt.dereference(var.id))
      {
        _make_true(rt, dc.decode_object(val.value()), cont);
        return;
      }
      else
        raise(term("instantiation_error"));
    }

    default:
      raise(term("type_error", term("callable"), e));
  }
}


void
interpreter::_make_true__and(runtime &rt, size_t i, object_iterator eit,
                             const continuation &cont)
{
  basic_decoder dc;
  if (i > 0)
  {
    const object_view e = dc.decode_object(eit);
    _make_true(rt, e, [this, i, eit, cont](runtime &rt) {
      _make_true__and(rt, i - 1, eit, cont);
    });
  }
  else // i == 0
    cont(rt);
}


void
interpreter::_make_true__or(runtime &rt, size_t i, object_iterator eit,
                            const continuation &cont)
{
  basic_decoder dc;
  // (opt) no need to lock RT state before the last / single clause
  assert(i >= 1);
  while (i-- > 1)
  { 
    state_saver _ {rt};
    _make_true(rt, dc.decode_object(eit), cont);
  }
  _make_true(rt, dc.decode_object(eit), cont);
}


void
interpreter::_make_true__if(runtime &rt, object_iterator eit,
                            const continuation &cont)
{
  basic_decoder dc;

  word_t *condp = allocate(1);
  *condp = 0;

  const object_view econd = dc.decode_object(eit);
  const object_view ethen = dc.decode_object(eit);
  const object_view eelse = dc.decode_object(eit);
  const continuation thencont = [this, condp, ethen, cont] (runtime &rt) {
    *condp = 1;
    _make_true(rt, ethen, cont);
  };

  {
    state_saver _ {rt};
    _make_true(rt, econd, thencont);
  }

  if (*condp == 0)
    _make_true(rt, eelse, cont);
}


void
interpreter::_make_true__predicate(runtime &rt, object_view e,
                                   const continuation &cont)
{
  basic_decoder dc;
  const auto it = m_predicates.find(e[0]);
  if (it != m_predicates.end())
  {
    const std::vector<std::pair<object, object>> &variants = it->second;
    size_t n = variants.size();
    for (const auto [sign, body] : variants)
    { // TODO: (opt) no need to lock RT state before the last / single option
      if (n-- == 1)
      {
        varnamespace ns;
        const object_view predsign = rt.adopt(ns, sign);
        matcher match {rt, dc};
        if (match(e, predsign))
        {
          if (not body.empty())
          {
            const object_view predbody = rt.adopt(ns, body);
            return _make_true(rt, predbody, cont);
          }
          else
            return cont(rt);
        }
        else
          rt.unallocate(predsign);
        return;
      }

      state_saver _ {rt};
      varnamespace ns;
      const object_view predsign = rt.adopt(ns, sign);
      matcher match {rt, dc};
      if (match(e, predsign))
      {
        if (not body.empty())
        {
          const object_view predbody = rt.adopt(ns, body);
          _make_true(rt, predbody, cont);
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
      std::cerr << std::format("no such predicate ({}/{})", m_symdict[hdr.id], hdr.arity) << std::endl;
      for (const auto &[w, _] : m_predicates)
      {
        term_header hdr;
        dc.decode(w, hdr);
        std::cerr << std::format("  have {}/{}", m_symdict[hdr.id], hdr.arity)
                  << std::endl;
      }

      throw std::runtime_error {
          std::format("no such predicate ({}/{})", m_symdict[hdr.id], hdr.arity)};
    }
    it->second(rt, hdr.arity, e.begin() + 1, cont);
    return;
  }
}
