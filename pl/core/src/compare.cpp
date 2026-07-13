#include "pl/coding/basic_decoder.hpp"
#include "pl/core/runtime.hpp"
#include "pl/obj/object.hpp"

#include <cassert>
#include <compare>
#include <unordered_set>


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


static std::strong_ordering
_compare(runtime &rt, object_iterator &lhs, object_iterator &rhs,
         std::unordered_set<memory_entry, memhash> &mem)
{
  basic_decoder dc;

  if (lhs[0] == rhs[0])
  {
    if (is_term(lhs[0]))
    {
      term_header hdr;
      dc.decode(lhs[0], hdr);
      lhs++;
      rhs++;
      for (size_t i = 0; i < hdr.arity; ++i)
      {
        const std::strong_ordering cmp = _compare(rt, lhs, rhs, mem);
        if (cmp != 0)
          return cmp;
      }
      return std::strong_ordering::equal;
    }
    else
      return std::strong_ordering::equal;
  }

  if (not mem.emplace(lhs, rhs).second)
    return std::strong_ordering::equal;

  if (is_nonterminal(lhs[0]))
  {
    nonterminal var;
    dc.decode(lhs[0], var);
    if (auto val = rt.dereference(var.id))
    {
      lhs++;
      return _compare(rt, val.value(), rhs, mem);
    }
  }

  if (is_nonterminal(rhs[0]))
  {
    nonterminal var;
    dc.decode(rhs[0], var);
    if (auto val = rt.dereference(var.id))
    {
      rhs++;
      return _compare(rt, lhs, val.value(), mem);
    }
  }

  return std::compare_three_way()(lhs[0], rhs[0]);
}


std::strong_ordering
compare(runtime &rt, object_iterator lhs, object_iterator rhs)
{
  std::unordered_set<memory_entry, memhash> mem;
  return _compare(rt, lhs, rhs, mem);
}