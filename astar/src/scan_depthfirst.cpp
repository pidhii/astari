#include "scan_depthfirst.hpp"
#include "memory_limits.hpp"

#include "pl/misc/term_utils.hpp"
#include "utl/state_saver.hpp"


lib_scan_depthfirst::lib_scan_depthfirst(interpreter &pl, char *c_stack_limit)
: m_c_stack_limit {c_stack_limit},
  m_pl {pl},
  m_graph {pl.symbols()},
  m_gn {0}
{
  pl.add_meta_op("$graph:graph_entry", [&](runtime &rt, size_t argc,
                                            object_iterator argv,
                                            continuation &cont) {
    return graph_entry(rt, argc, argv, cont);
  });

  pl.add_meta_op("$graph:yield", [&] NOINLINE (runtime &rt, size_t argc,
                                                object_iterator argv,
                                                continuation &cont) {
    TAILCALL yield(rt, argc, argv, cont);
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


void
lib_scan_depthfirst::graph_entry(runtime &rt, size_t argc, object_iterator argv,
                                 continuation &cont)
{
  assert(argc == 0);

  cont(rt, 0, 0, 0, 0); // begins graph generation

  message("done");
  print_stats(std::clog);
}


void
lib_scan_depthfirst::yield(runtime &rt, size_t argc, object_iterator argv,
                           continuation &cont)
{
  assert(argc == 3);
  if (memory_limit_crossed(rt, 500 * (1 << 10), m_c_stack_limit))
  {
    message("\e[38;5;1;1mresource limit reached\e[0m");
    return;
  }

  basic_decoder dc;

  // argv[0] Length of the edge from the previous node (provided explicitly)
  const object_view edgelen = rt.reduce(dc.decode_object(argv));
  const float len =
      number(m_pl, edgelen.begin(), [](auto x) { return float(x); });

  // argv[1] Node name
  const object_view nodenam = rt.reduce(dc.decode_object(argv));
  const object name = rt.reconstruct(nodenam);

  // argv[2] Heuristic distance to destiante
  // ... ignore ...

  const float gn = len + m_gn;

  // Record node
  // (don't continue if visiting an old node via a longer or equal path)
  if (not m_graph.node(name, gn))
    return;

  // Record edge
  if (not m_source.empty())
    m_graph.edge(m_source, name, len);

  static size_t _cnt = 0;
  if (_cnt++ % 10000 == 0)
  {
    print_stats(std::clog, true);
    message("processing");
  }

  state_saver _ {m_source, m_gn};
  m_source = name;
  m_gn = gn;
  cont(rt, 0, 0, 0, 0);
}