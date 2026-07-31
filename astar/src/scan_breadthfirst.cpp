#include "scan_breadthfirst.hpp"
#include "memory_limits.hpp"

#include "pl/misc/term_utils.hpp"


lib_scan_breadthfirst::lib_scan_breadthfirst(interpreter &pl,
                                             char *c_stack_limit)
: m_c_stack_limit {c_stack_limit}, m_pl {pl}, m_graph {pl.symbols()}
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


void
lib_scan_breadthfirst::graph_entry(runtime &rt, size_t argc,
                                   object_iterator argv, continuation &cont)
{
  assert(argc == 0);

  // Initialize library
  // - establish choice point
  rt.push_choice_point(&root_cp);

  // First round of sprouts
  m_acc = 0;
  cont(rt, 0, 0, 0, 0);

  // Keep growing until all sprouts have been exhausted or until cut
  while (not m_astar.empty() and not root_cp.cut)
  {
    astar::iterator it = m_astar.pop();
    state st = it->second.drain_state();
    m_source = it->first;
    m_acc = it->second.gn;
    st.cont(st.rt, 0, 0, 0, 0);
  }

  print_stats(std::clog);

  rt.pop_choice_point(&root_cp);
}


void
lib_scan_breadthfirst::yield(runtime &rt, size_t argc, object_iterator argv,
                             continuation &cont)
{
  assert(argc == 3);
  if (memory_limit_crossed(rt, 500 * (1 << 10), m_c_stack_limit))
    return;

  basic_decoder dc;

  // argv[0] Length of the edge from the previous node (provided explicitly)
  const object_view edgelenobj = rt.reduce(dc.decode_object(argv));
  const float edgelen =
      number(m_pl, edgelenobj.begin(), [](auto x) { return float(x); });

  // argv[1] Node name
  const object_view nodenameobj = rt.reduce(dc.decode_object(argv));
  object nodename = rt.reconstruct(nodenameobj);

  // argv[2] Heuristic distance to destination
  // ... ignore ...

  const float gn = m_acc + edgelen;

  // Record node
  // (don't continue if visiting an old node via a longer or equal path)
  if (not m_graph.node(nodename, gn))
    return;

  // Record edge
  if (not m_source.empty())
    m_graph.edge(m_source, nodename, edgelen);

  static size_t _cnt = 0;
  if (_cnt++ % 5000 == 0)
    print_stats(std::clog, true);

  // Suspend the sprout
  const auto it = m_astar.put(nodename, {rt, std::move(cont)}, gn, 1);
  if (it != m_astar.end())
    _lock_heap(rt, &root_cp);
}