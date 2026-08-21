#pragma once

#include "astar.hpp"
#include "graph.hpp"

#include "pl/core/interpreter.hpp"


class lib_scan_breadthfirst {
  const char *m_c_stack_limit;
  interpreter &m_pl;
  barrier root_cp; // Choice point at the root of the graph

  ssize_t m_source;
  int m_gn_acc; // Proxy for tracking g(n)

  astar m_astar;
  ::graph m_graph;
  graph_shortest_path m_gsp;
  std::deque<std::pair<astar::const_iterator, astar::const_iterator>> m_edges;

  // TODO: move it out
  static void
  _lock_heap(runtime &rt, barrier *root_cp)
  {
    barrier *cp = rt.query()->cp;
    while (cp != root_cp)
    {
      if (cp->noreclaim)
        break; // rest was already marked (TODO: find a way to assure this)
      cp->noreclaim = true;
      cp = cp->prev;
    }
  }

  public:
  lib_scan_breadthfirst(interpreter &pl, char *c_stack_limit);

  ::graph &
  graph() noexcept
  { return m_graph; }

  void
  print_stats(std::ostream &os, bool revcurs = false) const noexcept;

  continuation
  graph_entry(runtime &rt, size_t argc, object_iterator argv,
              continuation &cont);

  continuation
  yield(runtime &rt, size_t argc, object_iterator argv, continuation &cont);
};
