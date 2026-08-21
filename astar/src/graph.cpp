#include "graph.hpp"

#include "pl/dictionary.hpp"
#include "pl/misc/term_utils.hpp"


void
graph::write(object_file &objfile, dictionary &symbols, unsigned gen) const
{
  // Put nodes
  for (const auto &[name, idx] : m_nodes)
  {
    object nodeobj = make_term(symbols, term("node", name, unsigned(idx)));
    transfer_symbols(symbols, objfile.symbols, nodeobj);
    objfile.objects.emplace_back(std::move(nodeobj));
  }

  // Put edges
  for (const auto &[srcidx, dsts] : m_edges)
  {
    for (const auto [dstidx, weight] : dsts)
    {
      object edgeobj =
          make_term(objfile.symbols, term("edge", gen, unsigned(srcidx),
                                          unsigned(dstidx), weight));
      objfile.objects.emplace_back(std::move(edgeobj));
    }
  }

  // Put auxilary generation note
  const object gennote = make_term(objfile.symbols, term("generation", gen));
  objfile.objects.emplace_back(gennote);
}


bool
graph_shortest_path::operator () (size_t node, float gn)
{
  const auto [it, isnew] = m_minpath.emplace(node, gn);
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
