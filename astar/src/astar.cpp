#include "astar.hpp"


static struct {
  bool
  operator () (const node &a, const node &b) const noexcept
  { return (a.gn + a.hn) > (b.gn + b.hn); }
} node_gt;


static struct {
  bool
  operator () (const astar::iterator &a,
               const astar::iterator &b) const noexcept
  { return node_gt(a->second, b->second); }
} frontier_cmp;


astar::iterator
astar::pop()
{
  iterator n = m_frontier.front();

  // Remove node from the frontier
  std::pop_heap(m_frontier.begin(), m_frontier.end(), frontier_cmp);
  m_frontier.pop_back();
  n->second.is_frontier = false;

  return n;
}


astar::iterator
astar::put(object_view name, const state &st, float gn, float hn)
{
  const auto [it, isnew] = m_nodes.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(name),
    std::forward_as_tuple(st, gn, hn, true)
  );

  if (isnew) // new node
  {
    ninstalls++;
    m_frontier.emplace_back(it);
    std::push_heap(m_frontier.begin(), m_frontier.end(), frontier_cmp);
    return it;
  }
  else // revisiting old node
  {
    node &n = it->second;
    if (n.gn + n.hn <= gn + hn) // old is shorter or equal
    {
      nrejects++;
      return m_nodes.end(); // reject new sprout
    }
    else // new is better
    { // replace old sprout with the new one
      n.st = st;
      n.gn = gn;
      n.hn = hn;
      if (n.is_frontier) // the node is already in the active frontier
      {
        nupdates++;
        // adjust the heap
        // FIXME: investigate performance
        std::make_heap(m_frontier.begin(), m_frontier.end(), frontier_cmp);
        return it;
      }
      else // reinstall node in the frontier
      {
        nrestarts++;
        n.is_frontier = true;
        m_frontier.emplace_back(it);
        std::push_heap(m_frontier.begin(), m_frontier.end(), frontier_cmp);
        return it;
      }
    }
  }
}
