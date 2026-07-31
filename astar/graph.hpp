#pragma once

#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"

#include <map>


class graph {
  public:
  graph(dictionary &symbols)
  : m_symbols {symbols}
  { }

  bool
  node(object_view name, float gn);

  bool
  edge(object_view n1, object_view n2, float w);

  size_t
  nodes_size() const noexcept
  { return m_nodes.size(); }

  size_t
  edges_size() const noexcept
  { return m_edges.size(); }

  void
  write(std::ostream &os) const;

  void
  load(std::istream &is);

  private:
  dictionary &m_symbols;
  std::map<object, float> m_nodes;
  std::map<object, float> m_edges;
};
