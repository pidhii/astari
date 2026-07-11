#include "interpreter.hpp"
#include "match.hpp"

#include "utl/state_saver.hpp"


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
          _make_true__if(rt, e.begin() + 1, cont);
          return;

        case op_fail:
          return;

        default: // predicate
          _make_true__predicate(rt, e, cont);
          return;
      }
    }

    default:
      cont(rt);
      return;
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
  const continuation thencont = [this, condp, ethen, cont] (runtime &rt) {
    *condp = 1;
    _make_true(rt, ethen, cont);
  };

  {
    state_saver _ {rt};
    _make_true(rt, econd, thencont);
  }

  if (*condp == 0)
  {
    const object_view eelse = dc.decode_object(eit);
    _make_true(rt, eelse, cont);
  }
}


void
interpreter::_make_true__predicate(runtime &rt, object_view e,
                                   const continuation &cont)
{
  basic_decoder dc;
  const size_t n = m_predicates.bucket(e[0]);
  const auto begin = m_predicates.begin(n);
  const auto end = m_predicates.end(n);
  for (auto it = begin; it != end; ++it)
  { // TODO: (opt) no need to lock RT state before the last / single option
    state_saver _ {rt};
    varnamespace ns;
    const auto &[sign, body] = it->second;;
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
  }

  if (begin == end)
  {
    term_header hdr;
    dc.decode(e[0], hdr);
    const auto it = m_metaops.find(hdr.id);
    if (it == m_metaops.end())
      throw std::runtime_error {
          std::format("no such predicate ({})", m_symdict[hdr.id])};
    it->second(rt, hdr.arity, e.begin() + 1, cont);
  }
}
