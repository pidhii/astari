#pragma once

#include "astar.hpp"
#include "graph.hpp"

#include "pl/core/interpreter.hpp"


class lib_scan_breadthfirst {
  const char *m_c_stack_limit;
  interpreter &m_pl;
  barrier root_cp; // Choice point at the root of the graph

  object m_source;
  int m_acc; // Proxy for tracking g(n)

  astar m_astar;
  graph m_graph;
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

  void
  print_stats(std::ostream &os, bool revcurs = false) const noexcept;

  void
  graph_entry(runtime &rt, size_t argc, object_iterator argv,
              continuation &cont);

  void
  yield(runtime &rt, size_t argc, object_iterator argv, continuation &cont);
};
