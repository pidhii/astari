#pragma once

#include "pl/dictionary.hpp"
#include "pl/misc/object_file.hpp"
#include "pl/obj/object.hpp"

#include <boost/container_hash/hash.hpp>
#include <map>
#include <unordered_map>


class graph {
  public:
  size_t
  node(object_view name)
  {
    const auto [it, isnew] = m_nodes.emplace(name, m_index_counter);
    m_index_counter += isnew;
    return it->second;
  }

  void
  edge(size_t node1, size_t node2, float weight)
  { m_edges[node1][node2] = weight; }

  size_t
  nodes_size() const noexcept
  { return m_nodes.size(); }

  size_t
  edges_size() const noexcept
  { return m_edges.size(); }

  void
  write(object_file &objfile, dictionary &symbols, unsigned generation) const;

  private:
  size_t m_index_counter {0};
  std::unordered_map<object, size_t> m_nodes;
  std::unordered_map<size_t, std::map<size_t, float>> m_edges;
};


class graph_shortest_path {
  public:
  bool
  operator () (size_t node, float gn);

  private:
  std::unordered_map<size_t, float> m_minpath;
};
