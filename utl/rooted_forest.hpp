#pragma once

#include "../forest/forest.hpp"


template <template <typename> typename Container = std::vector>
class rooted_forest
: protected pidhii::forest<
      pidhii::forest_traits<pidhii::find_method::naive,
                            pidhii::union_method::naive>,
      Container> {
  using base =
      pidhii::forest<pidhii::forest_traits<pidhii::find_method::naive,
                                           pidhii::union_method::naive>,
                     Container>;

  public:
  rooted_forest() = default;

  rooted_forest(size_t size)
  {
    while (size--)
      make_set();
  }

  size_t
  make_set()
  { return _make_set(false); }

  size_t
  make_root_set()
  { return _make_set(true); }

  bool
  is_root(size_t i) const
  { return m_roots[i]; }

  bool
  join(size_t i, size_t j)
  {
    i = base::find(i);
    j = base::find(j);
    if (not is_root(i))
    {
      base::m_els[i].parent = j;
      return true;
    }
    else if (not is_root(j))
    {
      base::m_els[j].parent = i;
      return true;
    }
    else
      return false;
  }

  size_t
  find(size_t i) 
  { return base::find(i); }

  private:
  size_t
  _make_set(bool isroot)
  {
    const size_t k = base::make_set();
    m_roots.push_back(isroot);
    return k;
  }

  private:
  Container<bool> m_roots;
}; // class disjoint_forest_set
