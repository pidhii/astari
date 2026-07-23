#include "runtime.hpp"
#include "match.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/obj/object.hpp"


size_t unwind_heap[unwind_heap_length];
size_t *unwind_p = unwind_heap;
barrier *choice_point = nullptr;

object_view
runtime::adopt(varnamespace &ns, object_view in)
{
  object_view out = allocate_object(in.size());
  word_t *outiter = const_cast<word_t*>(out.begin());
  _adopt(ns, in, outiter);
  return out;
}


object
runtime::reconstruct(object_iterator in)
{
  object result;
  auto out = std::back_inserter(result);
  _reconstruct(in, out);
  return result;
}



bool
runtime::match(object_view lhs, object_view rhs)
{
  static matcher::memory mem;
  mem.clear();
  return ::match_uw(*this, lhs.begin(), rhs.begin(), 1, mem, *::choice_point);
}


std::optional<object_iterator>
runtime::dereferencer(size_t &varid)
{
  const auto [v, i] = m_dsf.find(varid);
  varid = i;
  return v ? std::make_optional(v) : std::nullopt;
}


void
runtime::assign(size_t varid, object_iterator value)
{
  cell *c = m_dsf.find_cell(varid).first;
  c->tag = cell::tag::val;
  c->value = value;
}


void
runtime::assign_uw(size_t varid, object_iterator value, barrier bar)
{
  const auto [c, i] = m_dsf.find_cell(varid);
  assert(c->tag == cell::tag::var);
  c->tag = cell::tag::val;
  c->value = value;
  if (i < bar.varbar)
    *unwind_p++ = i;
}


void
runtime::_adopt(varnamespace &ns, object_view in, word_t *out)
{
  const size_t var0 = m_dsf.size();
  size_t varn = var0;

  for (size_t i = 0; i < in.size(); ++i)
  {
    if (not is_nonterminal(in[i]))
      out[i] = in[i];
    else // nonterminal
    {
      nonterminal var;
      basic_decoder().decode(in[i], var);
      const auto it = ns.find(var.id);
      size_t runtimeid;
      if (it == ns.end())
      { // Create new variable
        runtimeid = varn++;
        ns.emplace(size_t(var.id), runtimeid);
      }
      else
        runtimeid = it->second;
      out[i] = basic_encoder().encode(nonterminal(runtimeid));
    }
  }

  m_dsf.make_n_sets(varn - var0);
}


template <typename OutputIter>
void
runtime::_reconstruct(object_iterator &in, OutputIter &out)
{
  switch (word_type(*in))
  {
    case word_type::blob:
    case word_type::signed_int_number:
    case word_type::unsigned_int_number:
    case word_type::float_number:
      *out++ = *in++;
      return;

    case word_type::structure:
    {
      term_header hdr;
      basic_decoder().decode(*in, hdr);
      *out++ = the_word(*in++);
      for (size_t i = 0; i < hdr.arity; ++i)
        _reconstruct(in, out);
      return;
    }

    case word_type::nonterminal:
    {
      nonterminal var;
      basic_decoder().decode(*in++, var);
      const auto [v, i] = m_dsf.find(var.id);
      if (v)
      {
        object_iterator it = v;
        _reconstruct(it, out);
      }
      else
        *out++ = basic_encoder().encode(nonterminal {i});
      return;
    }
  }
}


object_iterator
runtime::reduce(object_iterator x)
{
  if (is_nonterminal(x[0]))
  {
    basic_decoder dc;
    nonterminal var;
    dc.decode(x[0], var);
    if (auto val = dereference(var.id))
      return val.value();
  }

  return x;
}


object_view
runtime::reduce(object_view x)
{
  if (is_nonterminal(x[0]))
  {
    basic_decoder dc;
    nonterminal var;
    dc.decode(x[0], var);
    if (auto val = dereference(var.id))
      return dc.decode_object(val.value());
  }

  return x;
}



void
runtime::push_choice_point(barrier *bar) const noexcept
{
  bar->varbar = m_dsf.size();
  bar->uwbar = ::unwind_p;
  bar->cut = false;
  bar->prev = ::choice_point;
  ::choice_point = bar;
}

void
runtime::unwind(barrier *cp)
{
  assert(cp == ::choice_point);
  assert(::unwind_p >= cp->uwbar);
  for (size_t *uwp = cp->uwbar; uwp < ::unwind_p; ++uwp)
  {
    const size_t i = *uwp;
    if (i < cp->varbar)
    {
      cell &c = m_dsf[i];
      c.tag = cell::tag::var;
      c.next = i;
    }
  }
  ::unwind_p = cp->uwbar;
  ::choice_point = cp->prev;
  m_dsf.resize(cp->varbar);
}

void
runtime::cut(barrier *tgt)
{
  barrier *cp = ::choice_point;
  while (true)
  {
    cp->cut = true;
    if (cp == tgt)
      break;
    cp = cp->prev;
  }
}

void
runtime::pop_choice_point(barrier *cp)
{
  assert(cp == ::choice_point);
  ::choice_point = cp->prev;
}

bool
runtime::uwuc(barrier *cp)
{
  if (cp->cut)
  {
    pop_choice_point(cp);
    return true;
  }
  else
  {
    unwind(cp);
    return false;
  }
}
