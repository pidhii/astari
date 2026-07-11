#pragma once

#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"

#include <unordered_set>


class matcher {
  public:
  matcher(runtime &rt, basic_decoder &dc): m_decoder {dc}, m_runtime {rt} { }

  bool
  operator () (object_view lhs, object_view rhs)
  {
    object_iterator lhsiter = lhs.begin();
    object_iterator rhsiter = rhs.begin();
    return _match(lhsiter, rhsiter);
  }

  private:
  bool
  _match(object_iterator &lhs, object_iterator &rhs);

  using memory_entry = std::pair<object_iterator, object_iterator>;

  struct memhash {
    size_t
    operator () (const memory_entry &x) const noexcept
    {
      std::hash<object_iterator> hasher;
      const size_t a = hasher(x.first);
      const size_t b = hasher(x.second);
      return a ^ (b + 0x9e3779b9 + (a<<6) + (a>>2));
    }
  };

  private:
  basic_decoder &m_decoder;
  runtime &m_runtime;
  std::unordered_set<memory_entry, memhash> m_memory;
  bool m_is_matched;
};
