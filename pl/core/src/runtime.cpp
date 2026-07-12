#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"


object_view
runtime::adopt(varnamespace &ns, object_view in)
{
  object_view out = allocate_object(in.size());
  word_t *outiter = const_cast<word_t*>(out.begin());
  _adopt(ns, in.begin(), in.end(), outiter);
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
runtime::match(object_view lhs, object_view rhs, basic_decoder &dc)
{
  matcher m {*this, dc};
  return m(lhs, rhs);
}


std::optional<object_iterator>
runtime::dereference(size_t varid)
{
  varid = m_dsf.find(varid);
  if (not m_dsf.is_root(varid))
    return std::nullopt;
  const auto it = m_assignments->find(varid);
  if (it != m_assignments->end())
    return it->second;
  else
    return std::nullopt;
}


void
runtime::assign(size_t varid, object_iterator value)
{
  varid = m_dsf.find(varid);
  if (dereference(varid))
    throw std::runtime_error {"double assignment"};
  const size_t proxyvar = m_dsf.make_root_set();
  m_dsf.join(varid, proxyvar);
  m_assignments->insert_or_assign(proxyvar, value);
}


template <typename InputIter, typename OutputIter>
void
runtime::_adopt(varnamespace &ns, InputIter in, InputIter end, OutputIter out)
{
  while (in != end)
  {
    if (not is_nonterminal(*in))
      *out++ = *in++;
    else // nonterminal
    {
      nonterminal var;
      basic_decoder().decode(*in++, var);
      const auto it = ns.find(var.id);
      size_t runtimeid;
      if (it == ns.end())
      {
        runtimeid = m_dsf.make_set();
        ns.emplace(size_t(var.id), runtimeid);
      }
      else
        runtimeid = it->second;
      *out++ = basic_encoder().encode(nonterminal(runtimeid));
    }
  }
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
      *out++ = *in++;
      for (size_t i = 0; i < hdr.arity; ++i)
        _reconstruct(in, out);
      return;
    }

    case word_type::nonterminal:
    {
      nonterminal var;
      basic_decoder().decode(*in++, var);
      if (const auto val = dereference(var.id))
      {
        auto it = *val;
        _reconstruct(it, out);
      }
      else
        *out++ = basic_encoder().encode(nonterminal {var.id});
      return;
    }
  }
}
