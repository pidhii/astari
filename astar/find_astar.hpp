#pragma once

#include "astar.hpp"

#include "pl/core/interpreter.hpp"


class lib_seach_astar {
  const char *m_c_stack_limit;
  interpreter &m_pl;
  barrier root_cp; // Choice point at the root of the graph

  int m_acc; // Proxy for tracking g(n)
  ssize_t m_minremwords = std::numeric_limits<ssize_t>::max();

  std::optional<::astar> m_astar;
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
  lib_seach_astar(interpreter &pl, char *c_stack_limit);

  void
  print_stats(std::ostream &os) const noexcept;

  void
  graph_entry(runtime &rt, size_t argc, object_iterator argv,
              continuation &cont);

  void
  yield(runtime &rt, size_t argc, object_iterator argv, continuation &cont);
};



