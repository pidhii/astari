#pragma once

#include "pl/obj/object.hpp"

#include <vector>


struct cell {
  enum class tag { var, val } tag;
  union {
    size_t next;
    object_iterator value;
  };
};


template <template <typename> typename Container = std::vector>
class rooted_forest {
  public:
  rooted_forest() = default;

  rooted_forest(size_t size)
  {
    while (size--)
      make_set();
  }

  size_t
  size() const noexcept
  { return m_els.size(); }

  size_t
  make_set()
  {
    const size_t i = m_els.size();
    cell &c = m_els.emplace_back(i);
    c.tag = cell::tag::var;
    c.next = i;
    return i;
  }

  size_t
  make_root(object_iterator val)
  {
    const size_t i = m_els.size();
    cell &c = m_els.emplace_back();
    c.tag = cell::tag::val;
    c.value = val;
    return i;
  }

  void
  make_n_sets(size_t n)
  {
    m_els.append(n, [](size_t i) -> cell {
      return cell {.tag = cell::tag::var, .next = i};
    });
  }

  bool
  join(size_t i, size_t j)
  {
    auto [ci, ri] = find_cell(i);
    auto [cj, rj] = find_cell(j);
    if (ci->tag == cell::tag::var and cj->tag == cell::tag::var)
    {
      if (i < j)
        ci->next = j;
      else
        cj->next = i;
      return true;
    }
    else if (ci->tag == cell::tag::var)
    {
      ci->next = j;
      return true;
    }
    else if (cj->tag == cell::tag::var)
    {
      cj->next = i;
      return true;
    }
    else
      return false;
  }

  [[nodiscard]] std::pair<object_iterator, size_t>
  find(size_t i) const
  {
    cell c = m_els[i];
    while (true)
    {
      if (c.tag == cell::tag::val)
        return {c.value, i};
      if (c.next == i)
        return {nullptr, i};
      i = c.next;
      c = m_els[i];
    }
  }

  [[nodiscard]] std::pair<cell*, size_t>
  find_cell(size_t i)
  {
    do {
      cell &c = m_els[i];
      if (c.tag == cell::tag::val or c.next == i)
        return {&c, i};
      i = c.next;
    } while (true);
  }

  private:
  Container<cell> m_els;
}; // class disjoint_forest_set
