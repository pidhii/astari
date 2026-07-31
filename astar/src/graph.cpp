#include "graph.hpp"

#include "pl/misc/term_utils.hpp"


bool
graph::node(object_view name, float gn)
{
  object node = make_term(m_symbols, term("node", name));
  const auto [it, isnew] = m_nodes.emplace(name, gn);
  if (isnew)
    return true;
  else if (gn < it->second)
  {
    it->second = gn;
    return true;
  }
  else
    return false;
}


bool
graph::edge(object_view n1, object_view n2, float w)
{
  object edge = make_term(m_symbols, term("edge", n1, n2));
  const auto [_, isnew] = m_edges.emplace(std::move(edge), w);
  return isnew;
}
