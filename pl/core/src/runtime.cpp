#include "runtime.hpp"
#include "interpreter.hpp"
#include "match.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/obj/object.hpp"


void
runtime::print_dynamic_database(dictionary &symbols) const
{
  basic_decoder dc;
  for (const auto &[w, variants] : m_dyndb)
  {
    term_header hdr;
    dc.decode(w, hdr);
    std::cerr << std::format("  have {}/{}:", symbols[hdr.id], hdr.arity)
              << std::endl;
  }
}


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
  assert(m_query->heap_p + in.size() <= m_query->heap_e);
  word_t *p = m_query->heap_p;
  m_query->heap_p += in.size();
  _adopt(ns, in, p);
  return {p, in.size()};
}

object_view
runtime::adopt_hp_n(size_t base, object_view in)
{
  assert(m_query->heap_p + in.size() <= m_query->heap_e);
  word_t *p = m_query->heap_p;
  m_query->heap_p += in.size();
  _adopt_n(base, in, p);
  return {p, in.size()};
}

object_view
runtime::adopt_clause_hp(dyn_variant_iterator it)
{
  basic_encoder ec;
  const size_t base = m_dsf.size();

  word_t *p = m_query->heap_p;
  if (it->body.empty())
    adopt_hp_n(base, it->sign);
  else
  {
    *m_query->heap_p++ = ec.encode(term_header(op_penis, 2));
    adopt_hp_n(base, it->sign);
    adopt_hp_n(base, it->body);
  }
  make_n_vars(it->nvars);

  return {p, m_query->heap_p};
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
  return ::match_uw(*this, lhs.begin(), rhs.begin(), 1, mem, *m_query->cp);
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
    *m_query->unwind_p++ = i;
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
runtime::_adopt_n(size_t base, object_view in, word_t *out)
{
  basic_decoder dc;
  basic_encoder ec;

  for (size_t i = 0; i < in.size(); ++i)
  {
    const size_t w = in[i];
    if (is_nonterminal(w))
    {
      nonterminal var;
      dc.decode(w, var);
      const size_t rtid = base + var.id;
      out[i] = ec.encode(nonterminal(rtid));
    }
    else
      out[i] = w;
  }
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
  bar->uwbar = m_query->unwind_p;
  bar->hpbar = m_query->heap_p;
  bar->cut = false;
  bar->noreclaim = false;
  bar->prev = m_query->cp;
  m_query->cp = bar;
}

void
runtime::unwind(barrier *cp)
{
  assert(cp == m_query->cp);
  assert(not cp->cut);
  assert(m_query->unwind_p >= cp->uwbar);
  for (size_t *uwp = cp->uwbar; uwp < m_query->unwind_p; ++uwp)
  {
    const size_t i = *uwp;
    if (i < cp->varbar)
      set_next(m_dsf[i], i);
  }
  m_query->unwind_p = cp->uwbar;
  m_query->cp = cp->prev;
  if (not cp->noreclaim)
    m_query->heap_p = cp->hpbar;
  m_dsf.resize(cp->varbar);
}

void
runtime::cut(barrier *tgt)
{
  barrier *cp = m_query->cp;
  while (true)
  {
    cp->cut = true;
    if (cp == tgt)
      break;
    cp = cp->prev;
  }
}

void
runtime::cut_exc(barrier *tgt)
{
  barrier *cp = m_query->cp;
  while (cp != tgt)
  {
    cp->cut = true;
    cp = cp->prev;
  }
}

void
runtime::pop_choice_point(barrier *cp)
{
  assert(cp == m_query->cp);
  m_query->cp = cp->prev;
}

runtime::recovery
runtime::asserta_dyn(object_view signobj, object_view bodyobj)
{
  const predicate_entry pred = prepare_predicate(signobj, bodyobj);
  const size_t k = signobj[0] & term_mask;
  return {k, m_dyndb.push_front(k, pred)};
}

runtime::recovery
runtime::assertz_dyn(object_view signobj, object_view bodyobj)
{
  const predicate_entry pred = prepare_predicate(signobj, bodyobj);
  const size_t k = signobj[0] & term_mask;
  return {k, m_dyndb.push_back(k, pred)};
}

runtime::recovery
runtime::retract_dyn(word_t signkey, dyn_variant_iterator it)
{
  signkey = the_word(signkey);
  return {signkey, m_dyndb.erase(signkey, it)};
}

void
runtime::recover(recovery &&recovery)
{ m_dyndb.rollback(recovery.key, std::move(recovery.entrecov)); }


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