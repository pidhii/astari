#include "pl/coding/basic_decoder.hpp"
#include "pl/core/runtime.hpp"
#include "pl/misc/display.hpp"
#include "pl/obj/object.hpp"

#include <cassert>
#include <compare>
#include <unordered_set>
#include <iostream>


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

  if (is_nonterminal(lhs[0]) or is_nonterminal(rhs[0]))
  {
    if (is_nonterminal(lhs[0]) and is_nonterminal(rhs[0]))
    {
      nonterminal lhsvar, rhsvar;
      dc.decode(lhs[0], lhsvar);
      dc.decode(rhs[0], rhsvar);
      if (rt.bound(lhsvar.id, rhsvar.id))
        return std::strong_ordering::equal;
      else
        return lhs[0] <=> rhs[0];
    }

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
  }

  if (is_blob(lhs[0]) or is_blob(rhs[0]))
  {
    if (is_blob(lhs[0]) and is_blob(rhs[0]))
    {
      const enum blob_tag ltag = blob_tag(lhs[0]);
      const enum blob_tag rtag = blob_tag(rhs[0]);
      if (ltag != rtag)
        return word_t(ltag) <=> word_t(rtag);

      switch (ltag)
      {
        case blob_tag::string:
          return string(lhs[0]) <=> string(rhs[0]);
        default:
          throw std::runtime_error {"invalid blob tag"};
      }
    }
    else if (is_blob(lhs[0]))
      return std::strong_ordering::less;
    else if (is_blob(rhs[0]))
      return std::strong_ordering::greater;
  }

  return lhs[0] <=> rhs[0];
}



std::strong_ordering
compare(runtime &rt, object_view lhs, object_view rhs)
{
  std::unordered_set<memory_entry, memhash> mem;
  object_iterator a = lhs.begin(), b = rhs.begin();
  return _compare(rt, a, b, mem);
}