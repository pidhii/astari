#include "runtime.hpp"
#include "match.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/obj/object.hpp"


size_t unwind_heap[unwind_heap_length];
size_t *unwind_p = unwind_heap;
barrier *choice_point = nullptr;
word_t term_heap[TERM_HEAP_SIZE];
word_t *heap_p = term_heap;


object_view
runtime::adopt_g(varnamespace &ns, object_view in)
{
  object_view out = allocate_object(in.size());
  word_t *outiter = const_cast<word_t*>(out.begin());
  _adopt(ns, in, outiter);
  return out;
}

object_view
runtime::adopt_hp(varnamespace &ns, object_view in)
{
  assert(heap_p + in.size() <= term_heap + TERM_HEAP_SIZE);
  word_t *p = heap_p;
  heap_p += in.size();
  _adopt(ns, in, p);
  return {p, in.size()};
}

object_view
runtime::adopt_hp_n(word_t ns[], object_view in)
{
  assert(heap_p + in.size() <= term_heap + TERM_HEAP_SIZE);
  word_t *p = heap_p;
  heap_p += in.size();
  _adopt_n(ns, in, p);
  return {p, in.size()};
}

object
runtime::reconstruct(object_iterator in)
{
  object result;
  _reconstruct(in, std::back_inserter(result), 1);
  return result;
}

object
runtime::reconstruct(object_view in)
{
  object result;
  result.reserve(in.size());
  _reconstruct(in.begin(), std::back_inserter(result), 1);
  return result;
}

void
runtime::reconstruct(object_iterator in, word_t *out)
{ _reconstruct(in, out, 1); }


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
  assert(is_var(*c));
  set_val(*c, value);
}


void
runtime::assign_uw(size_t varid, object_iterator value, barrier bar)
{
  const auto [c, i] = m_dsf.find_cell(varid);
  assert(is_var(*c));
  set_val(*c, value);
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
      const auto [it, isnew] = ns.emplace(var.id, varn);
      const size_t runtimeid = it->second;
      varn += isnew;
      out[i] = basic_encoder().encode(nonterminal(runtimeid));
    }
  }

  m_dsf.make_n_sets(varn - var0);
}

void
runtime::_adopt_n(word_t ns[], object_view in, word_t *out)
{
  basic_decoder dc;
  basic_encoder ec;

  const size_t var0 = m_dsf.size();
  size_t varn = var0;

  for (size_t i = 0; i < in.size(); ++i)
  {
    const size_t w = in[i];
    if (is_nonterminal(w))
    {
      nonterminal var;
      dc.decode(w, var);
      const word_t magic = word_magic(w);
      const size_t nsid = ns[var.id];
      if (nsid != -1ull)
        out[i] = add_magic(ec.encode(nonterminal(nsid)), magic);
      else
      {
        const size_t rtid = varn++;
        ns[var.id] = rtid;
        out[i] = add_magic(ec.encode(nonterminal(rtid)), magic);
      }
    }
    else
      out[i] = w;
  }

  m_dsf.make_n_sets(varn - var0);
}


template <typename OutputIter>
void
runtime::_reconstruct(object_iterator in, OutputIter out, size_t n)
{
  while (n--)
  {
    switch (word_type(*in))
    {
      case word_type::blob:
      case word_type::signed_int_number:
      case word_type::unsigned_int_number:
      case word_type::float_number:
        *out++ = *in++;
        break;

      case word_type::structure:
      {
        term_header hdr;
        basic_decoder().decode(*in, hdr);
        *out++ = (*in++ & term_mask);
        n += hdr.arity;
        break;
      }

      case word_type::nonterminal:
      {
        nonterminal var;
        basic_decoder().decode(*in++, var);
        const auto [v, i] = m_dsf.find(var.id);
        if (v)
          _reconstruct(v, out, 1);
        else
          *out++ = basic_encoder().encode(nonterminal {i});
        break;
      }
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
  bar->hpbar = ::heap_p;
  bar->cut = false;
  bar->noreclaim = false;
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
      set_next(m_dsf[i], i);
  }
  ::unwind_p = cp->uwbar;
  ::choice_point = cp->prev;
  if (not cp->noreclaim)
    ::heap_p = cp->hpbar;
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


size_t
normalize_r(object_view in, word_t *out, varnamespace &ns, size_t varn)
{
  for (size_t i = 0; i < in.size(); ++i)
  {
    if (not is_nonterminal(in[i]))
      out[i] = the_word(in[i]);
    else // nonterminal
    {
      nonterminal var;
      basic_decoder().decode(in[i], var);
      const auto [it, isnew] = ns.emplace(var.id, varn);
      const size_t newid = it->second;
      varn += isnew;
      out[i] = basic_encoder().encode(nonterminal(newid));
    }
  }
  return varn;
}


size_t
normalize(object_view in, word_t *out)
{
  static varnamespace ns;
  ns.clear();
  return normalize_r(in, out, ns, 0);
}