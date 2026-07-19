#pragma once

#include "pl/obj/object.hpp"
#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"

#include <unordered_set>


class matcher {
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

  matcher(runtime &rt, basic_decoder &dc): m_decoder {dc}, m_runtime {rt} { }

  bool
  operator () (object_view lhs, object_view rhs)
  {
    object_iterator lhsiter = lhs.begin();
    object_iterator rhsiter = rhs.begin();
    return match(lhsiter, rhsiter, m_memory);
  }

  bool
  match(object_view lhs, object_view rhs, memory &mem)
  {
    object_iterator lhsit = lhs.begin();
    object_iterator rhsit = rhs.begin();
    return _match(lhsit, rhsit, mem);
  }

  private:
  bool
  _match(object_iterator lhs, object_iterator rhs, memory &mem);

  private:
  basic_decoder &m_decoder;
  runtime &m_runtime;
  std::unordered_set<memory_entry, memhash> m_memory;
  bool m_is_matched;
};
