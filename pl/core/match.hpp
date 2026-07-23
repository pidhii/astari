#pragma once

#include "runtime.hpp"

#include "pl/obj/object.hpp"

#include <unordered_set>


struct matcher {
  public:
  using memory_entry = std::pair<object_iterator, object_iterator>;

  struct memhash {
    size_t
    operator () (memory_entry x) const noexcept
    {
      if (x.first > x.second)
        std::swap(x.first, x.second);

      std::hash<object_iterator> hasher;
      const size_t a = hasher(x.first);
      const size_t b = hasher(x.second);
      return a ^ (b + 0x9e3779b9 + (a<<6) + (a>>2));
    }
  };

  using memory = std::unordered_set<memory_entry, memhash>;
};

bool
shallow_match(object_iterator lhs, object_iterator rhs, size_t n = 1);

bool
match(runtime &rt, object_iterator lhs, object_iterator rhs, size_t n,
      matcher::memory &mem);

bool
match_uw(runtime &rt, object_iterator lhs, object_iterator rhs, size_t n,
         matcher::memory &mem, barrier bar);