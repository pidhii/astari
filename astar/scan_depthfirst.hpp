#pragma once

#include "graph.hpp"

#include "pl/core/interpreter.hpp"


class lib_scan_depthfirst {
  const char *m_c_stack_limit;
  interpreter &m_pl;
  ::graph m_graph;
  graph_shortest_path m_gsp;

  // Runtime stuff:
  float m_gn_acc;
  ssize_t m_source; // Proxy for building edges

  // Misc
  std::string m_msg;

  protected:
  void
  message(std::string_view msg)
  { m_msg = msg; }

  public:
  lib_scan_depthfirst(interpreter &pl, char *c_stack_limit);

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

