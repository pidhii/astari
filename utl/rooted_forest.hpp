#pragma once

#include "pl/obj/object.hpp"

#include <cassert>
#include <vector>


using cell = size_t;

[[nodiscard, gnu::pure]] static inline
bool
is_var(cell c)
{ return (c & 1ull) == 0; }

[[nodiscard, gnu::pure]] static inline
bool
is_val(cell c)
{ return (c & 1ull) == 1; }

[[nodiscard, gnu::pure]] static inline
cell
make_next(size_t i)
{ return i << 32; }

[[nodiscard, gnu::pure]] static inline
cell
make_val(object_iterator val)
{ return reinterpret_cast<size_t>(val) | 1ull; }

static inline void
set_next(cell &c, size_t i)
{ c = i << 32; }

static inline void
set_val(cell &c, object_iterator val)
{ c = reinterpret_cast<size_t>(val) | 1ull; }

[[nodiscard, gnu::pure]] static inline
object_iterator
val(cell c)
{ return reinterpret_cast<object_iterator>(c & ~1ull); }

[[nodiscard, gnu::pure]] static inline
size_t
next(cell c)
{ return c >> 32; }


template <template <typename> typename Container = std::vector>
class rooted_forest {
  public:
  rooted_forest() = default;

  size_t
  size() const noexcept
  { return m_els.size(); }

  void
  resize(size_t n)
  { m_els.resize(n); }

  cell&
  operator [] (size_t i) noexcept
  { return m_els[i]; }

  size_t
  make_set()
  {
    const size_t i = m_els.size();
    m_els.emplace_back(i);
    return i;
  }

  size_t
  make_root(object_iterator val)
  {
    const size_t i = m_els.size();
    m_els.emplace_back(make_val(val));
    return i;
  }

  void
  make_n_sets(size_t n)
  { m_els.append(n, [](size_t i) -> cell { return make_next(i); }); }

  size_t
  join(size_t i, size_t j)
  {
    const auto [ci, ri] = find_cell(i);
    const auto [cj, rj] = find_cell(j);
    if (is_var(*ci) and is_var(*cj))
    {
      // Prioritize new to old
      if (ri < rj)
      {
        set_next(*cj, ri);
        return rj;
      }
      else
      {
        set_next(*ci, rj);
        return ri;
      }
    }
    else if (is_var(*ci))
    {
      set_next(*ci, rj);
      return ri;
    }
    else if (is_var(*cj))
    {
      set_next(*cj, ri);
      return rj;
    }

    assert(not "must not happen");
  }


  [[nodiscard]] std::pair<object_iterator, size_t>
  find(size_t i) const
  {
    cell c = m_els[i];
    while (true)
    {
      if (is_val(c))
        return {val(c), i};
      if (next(c) == i)
        return {nullptr, i};
      i = next(c);
      c = m_els[i];
    }
  }

  [[nodiscard]] std::pair<cell*, size_t>
  find_cell(size_t i)
  {
    do {
      cell &c = m_els[i];
      if (is_val(c) or next(c) == i)
        return {&c, i};
      i = next(c);
    } while (true);
  }

  private:
  Container<size_t> m_els;
}; // class disjoint_forest_set
