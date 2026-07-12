#pragma once

#include "pl/obj/object.hpp"
#include "pvector/pvector.hpp"
#include "utl/arena_allocator.hpp"
#include "utl/rooted_forest.hpp"

#include <optional>
#include <unordered_map>


using varnamespace = std::unordered_map<size_t, size_t>;


class runtime {
  public:
  runtime()
  : m_assignments {std::make_shared<std::unordered_map<size_t, object_iterator>>()},
    m_arena {std::make_shared<arena_allocator<512 << 10, alignof(word_t)>>()}
  { }

  runtime(const runtime &other) = default;

  object_view
  adopt(varnamespace &ns, object_view in);

  object
  reconstruct(object_iterator in);

  object
  reconstruct(object_view in)
  { return reconstruct(in.begin()); }

  bool
  match(object_view lhs, object_view rhs, basic_decoder &dc);

  bool
  match(object_view lhs, object_view rhs)
  { basic_decoder dc; return match(lhs, rhs, dc); }

  std::optional<object_iterator>
  dereference(size_t varid);

  bool
  bind(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.join(lhsid, rhsid); }

  void
  assign(size_t varid, object_iterator value);

  word_t*
  allocate(size_t nwords)
  { return static_cast<word_t *>(m_arena->allocate(nwords * sizeof(word_t))); }

  object_view
  allocate_object(size_t nwords)
  { return {allocate(nwords), nwords}; }

  private:
  template <typename InputIter, typename OutputIter>
  void
  _adopt(varnamespace &ns, InputIter in, InputIter end, OutputIter out);

  template <typename OutputIter>
  void
  _reconstruct(object_iterator &in, OutputIter &out);

  private:
  // TODO: shared_ptr is slow
  std::shared_ptr<std::unordered_map<size_t, object_iterator>> m_assignments;
  rooted_forest<pidhii::pvector> m_dsf;
  std::shared_ptr<arena_allocator<512 << 10, alignof(word_t)>> m_arena;
};
