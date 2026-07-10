#pragma once

#include "../../pvector/pvector.hpp"
#include "../../utl/arena_allocator.hpp"
#include "../../utl/rooted_forest.hpp"
#include "../coding/basic_decoder.hpp"
#include "../coding/basic_encoder.hpp"
#include "../obj/object.hpp"

#include <optional>
#include <unordered_map>


using varnamespace = std::unordered_map<size_t, size_t>;


class object_allocator: arena_allocator<512 << 10, alignof(word_t)> {
  public:
  object_view
  allocate_object(size_t nwords)
  {
    word_t *p = static_cast<word_t *>(
        arena_allocator::allocate(nwords * sizeof(word_t)));
    return {p, nwords};
  }

  word_t*
  allocate(size_t nwords)
  {
    return static_cast<word_t *>(
        arena_allocator::allocate(nwords * sizeof(word_t)));
  }
};


class runtime {
  public:
  runtime()
  : m_assignments {std::make_shared<std::unordered_map<size_t, object_iterator>>()},
    m_arena {std::make_shared<object_allocator>()}
  { }

  runtime(const runtime &other)
  : m_assignments {other.m_assignments},
    m_dsf {other.m_dsf},
    m_arena {other.m_arena}
  { }

  object_allocator&
  allocator()
  { return *m_arena; }

  object_view
  adopt(varnamespace &ns, object_view in)
  {
    object_view out = m_arena->allocate_object(in.size());
    word_t *outiter = const_cast<word_t*>(out.begin());
    _adopt(ns, in.begin(), in.end(), outiter);
    return out;
  }

  object
  reconstruct(object_iterator in)
  {
    object result;
    auto out = std::back_inserter(result);
    _reconstruct(in, out);
    return result;
  }

  std::optional<object_iterator>
  dereference(size_t varid)
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

  bool
  bind(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.join(lhsid, rhsid); }

  void
  assign(size_t varid, object_iterator value)
  {
    varid = m_dsf.find(varid);
    if (dereference(varid))
      throw std::runtime_error {"double assignment"};
    const size_t proxyvar = m_dsf.make_root_set();
    m_dsf.join(varid, proxyvar);
    m_assignments->insert_or_assign(proxyvar, value);
  }

  auto &
  assignments() const
  { return *m_assignments; }

  private:
  template <typename InputIter, typename OutputIter>
  void
  _adopt(varnamespace &ns, InputIter in, InputIter end, OutputIter out)
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
  _reconstruct(object_iterator &in, OutputIter &out)
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

  private:
  // TODO: shared_ptr is slow
  std::shared_ptr<std::unordered_map<size_t, object_iterator>> m_assignments;
  rooted_forest<pidhii::pvector> m_dsf;
  std::shared_ptr<object_allocator> m_arena;
};


