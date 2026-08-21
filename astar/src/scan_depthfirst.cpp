#include "scan_depthfirst.hpp"
#include "memory_limits.hpp"

#include "pl/misc/term_utils.hpp"
#include "utl/state_saver.hpp"


lib_scan_depthfirst::lib_scan_depthfirst(interpreter &pl, char *c_stack_limit)
: m_c_stack_limit {c_stack_limit}, m_pl {pl}, m_gn_acc {0}, m_source {-1}
{
  pl.add_meta_op("$graph:graph_entry", [&](runtime &rt, size_t argc,
                                            object_iterator argv,
                                            continuation &cont) {
    return graph_entry(rt, argc, argv, cont);
  });

  pl.add_meta_op("$graph:yield", [&] NOINLINE (runtime &rt, size_t argc,
                                                object_iterator argv,
                                                continuation &cont) {
    return yield(rt, argc, argv, cont);
  });

  pl.load_file("graph.pl");
}


void
lib_scan_depthfirst::print_stats(std::ostream &os, bool revcurs) const noexcept
{
  const double totwords = m_pl.heap_size();
  double remwords = m_pl.heap_remsize();
  if (remwords < 0)
    remwords = totwords - remwords;
  const double heapusage = (totwords - remwords) * 100 / totwords;

  os << "\e[Kscan profile:\n"
      << std::format("\e[K- heap usage {:.0}%", heapusage) << std::endl
      << "\e[K- nodes " << m_graph.nodes_size() << std::endl
      << "\e[K- edges " << m_graph.edges_size() << std::endl
      << (m_msg.empty() ? "..." : "\e[K> " + m_msg) << std::endl;
  if (revcurs)
    os << "\r\e[5A";
}


continuation
lib_scan_depthfirst::graph_entry(runtime &rt, size_t argc, object_iterator argv,
                                 continuation &cont)
{
  assert(argc == 0);

  rt.exhaust(cont); // graph generation

  message("done");
  print_stats(std::clog);

  return FAIL;
}


continuation
lib_scan_depthfirst::yield(runtime &rt, size_t argc, object_iterator argv,
                           continuation &cont)
{
  assert(argc == 3);
  if (memory_limit_crossed(rt, 500 * (1 << 10), m_c_stack_limit))
  {
    message("\e[38;5;1;1mresource limit reached\e[0m");
    return FAIL;
  }

  basic_decoder dc;

  // argv[0] Length of the edge from the previous node (provided explicitly)
  const float edgelen = number(m_pl, rt.reduce(dc.decode_object(argv).begin()),
                               [](auto x) { return float(x); });

  // argv[1] Node name
  const object nodename = rt.reconstruct(dc.decode_object(argv));

  // argv[2] Heuristic distance to destination
  // ... ignore ...

  const float gn = edgelen + m_gn_acc;

  // Record node and edge
  const size_t node = m_graph.node(nodename);
  if (m_source >= 0)
    m_graph.edge(m_source, node, edgelen);

  // (don't continue if visiting an old node via a longer or equal path)
  if (not m_gsp(node, gn))
    return FAIL;

  static size_t _cnt = 0;
  if (_cnt++ % 10000 == 0)
  {
    print_stats(std::clog, true);
    message("processing");
  }

  state_saver _ {m_source, m_gn_acc};
  m_source = node;
  m_gn_acc = gn;
  return cont;
}