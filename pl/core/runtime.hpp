#pragma once

#include "pl/misc/object_allocator.hpp"
#include "pl/obj/object.hpp"
#include "pvector/pvector.hpp"
#include "ualloc/ualloc.hpp"
#include "utl/rooted_forest.hpp"

#include "radixtrees/pradix256dense.hpp"

#include <optional>
#include <unordered_map>


using varnamespace = std::unordered_map<size_t, size_t>;


class runtime: public object_allocator {
  public:
  runtime() = default;
  runtime(const runtime &other) = default;

  object_view
  adopt(varnamespace &ns, object_view in);

  object_view
  adopt(object_view in)
  { varnamespace ns; return adopt(ns, in); }

  object
  reconstruct(object_iterator in);

  object
  reconstruct(object_view in)
  { return reconstruct(in.begin()); }

  bool
  match(object_view lhs, object_view rhs);

  std::optional<object_iterator>
  dereferencer(size_t &varid);

  std::optional<object_iterator>
  dereference(size_t varid)
  { return dereferencer(varid); }

  object_iterator
  reduce(object_iterator x);

  object_view
  reduce(object_view x);

  bool
  bind(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.join(lhsid, rhsid); }

  [[nodiscard]] bool
  bound(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.find(lhsid).second == m_dsf.find(rhsid).second; }

  void
  assign(size_t varid, object_iterator value);

  private:
  void
  _adopt(varnamespace &ns, object_view in, word_t *out);

  template <typename OutputIter>
  void
  _reconstruct(object_iterator &in, OutputIter &out);

  private:
  pidhii::pradix256dense<object_iterator> m_assignments;
  template <typename T>
  using pvector = pidhii::pvector<T, 5, pidhii::static_uniform_allocator<T>>;
  rooted_forest<pvector> m_dsf;
};
