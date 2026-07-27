#pragma once

#include "pl/obj/object.hpp"
#include "utl/arena_allocator.hpp"


class object_allocator {
  public:
  static constexpr size_t block_size = 20 * (2 << 20);

  object_allocator()
  : m_arena {std::make_shared<arena_allocator<block_size, alignof(word_t)>>()}
  { }

  word_t*
  allocate(size_t nwords)
  { return static_cast<word_t *>(m_arena->allocate(nwords * sizeof(word_t))); }

  void
  unallocate(size_t nwords)
  { m_arena->unallocate(nwords * sizeof(word_t)); }

  object_view
  allocate_object(size_t nwords)
  { return {allocate(nwords), nwords}; }

  void
  unallocate(object_view obj)
  { unallocate(obj.size()); }

  private:
  std::shared_ptr<arena_allocator<block_size, alignof(word_t)>> m_arena;
};
