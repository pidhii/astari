#include "scan_breadthfirst.hpp"
#include "memory_limits.hpp"

#include "pl/misc/term_utils.hpp"


lib_scan_breadthfirst::lib_scan_breadthfirst(interpreter &pl,
                                             char *c_stack_limit)
: m_c_stack_limit {c_stack_limit}, m_pl {pl}, m_source {-1}
{
  using namespace std::placeholders;
  pl.add_meta_op("$graph:graph_entry", [&](runtime &rt, size_t argc,
                                            object_iterator argv,
                                            continuation &cont) {
    return graph_entry(rt, argc, argv, cont);
  });
  pl.add_meta_op("$graph:yield", [&](runtime &rt, size_t argc,
                                      object_iterator argv,
                                      continuation &cont) {
    return yield(rt, argc, argv, cont);
  });

  pl.load_file("graph.pl");
}


void
lib_scan_breadthfirst::print_stats(std::ostream &os,
                                   bool revcurs) const noexcept
{
  const double totwords = m_pl.heap_size();
  double remwords = m_pl.heap_remsize();
  if (remwords < 0)
    remwords = totwords - remwords;
  const double heapusage = (totwords - remwords) * 100 / totwords;

  os << "\e[Kscan profile:\n"
      << "\e[K- heap usage  " << heapusage << "%" << std::endl
      << "\e[K- graph size " << m_astar.graph_size() << std::endl
      << "\e[K- frontier size " << m_astar.frontier_size() << std::endl
      << "\e[K- install " << m_astar.ninstalls << std::endl
      << "\e[K- reject " << m_astar.nrejects << std::endl
      << "\e[K- update " << m_astar.nupdates << std::endl
      << "\e[K- restart " << m_astar.nrestarts << std::endl;
  if (revcurs)
    os << "\r\e[8A";
}


continuation
lib_scan_breadthfirst::graph_entry(runtime &rt, size_t argc,
                                   object_iterator argv, continuation &cont)
{
  assert(argc == 0);

  // Initialize library
  // - establish choice point
  rt.push_choice_point(&root_cp);

  // First round of sprouts
  m_gn_acc = 0;
  rt.exhaust(cont);

  // Keep growing until all sprouts have been exhausted or until cut
  while (not m_astar.empty() and not root_cp.cut)
  {
    astar::iterator it = m_astar.pop();
    state st = it->second.drain_state();
    m_source = m_graph.node(it->first);
    m_gn_acc = it->second.gn;
    st.rt.exhaust(st.cont);
  }

  print_stats(std::clog);

  rt.pop_choice_point(&root_cp);

  return FAIL;
}


continuation
lib_scan_breadthfirst::yield(runtime &rt, size_t argc, object_iterator argv,
                             continuation &cont)
{
  assert(argc == 3);
  if (memory_limit_crossed(rt, 500 * (1 << 10), m_c_stack_limit))
    return FAIL;

  basic_decoder dc;

  // argv[0] Length of the edge from the previous node (provided explicitly)
  const object_view edgelenobj = rt.reduce(dc.decode_object(argv));
  const float edgelen =
      number(m_pl, edgelenobj.begin(), [](auto x) { return float(x); });

  // argv[1] Node name
  const object nodename = rt.reconstruct(dc.decode_object(argv));

  // argv[2] Heuristic distance to destination
  // ... ignore ...

  const float gn = m_gn_acc + edgelen;

  // Record node and edge
  const size_t node = m_graph.node(nodename);
  if (m_source >= 0)
    m_graph.edge(m_source, node, edgelen);

  // (don't continue if visiting an old node via a longer or equal path)
  if (not m_gsp(node, gn))
    return FAIL;

  static size_t _cnt = 0;
  if (_cnt++ % 5000 == 0)
    print_stats(std::clog, true);

  // Suspend the sprout
  const auto it = m_astar.put(nodename, {rt, std::move(cont)}, gn, 1);
  if (it != m_astar.end())
    _lock_heap(rt, &root_cp);

  return FAIL;
}