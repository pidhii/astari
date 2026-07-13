#pragma once

#include "pl/obj/object.hpp"
#include "utl/arena_allocator.hpp"


class object_allocator {
  public:
  object_allocator()
  : m_arena {std::make_shared<arena_allocator<512 << 10, alignof(word_t)>>()}
  { }

  word_t*
  allocate(size_t nwords)
  { return static_cast<word_t *>(m_arena->allocate(nwords * sizeof(word_t))); }

  object_view
  allocate_object(size_t nwords)
  { return {allocate(nwords), nwords}; }

  word_t
  make_string(std::string_view str)
  {
    const size_t size = sizeof(string_data) + str.size();
    const size_t nwords = size / sizeof(word_t) + !!(size % sizeof(word_t));
    string_data *p = reinterpret_cast<string_data*>(allocate(nwords));
    p->tag = blob_tag::string;
    p->size = str.size();
    std::copy(str.begin(), str.end(), p->data);
    return reinterpret_cast<word_t>(p);
  }

  private:
  std::shared_ptr<arena_allocator<512 << 10, alignof(word_t)>> m_arena;
};
