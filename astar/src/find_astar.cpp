#include "find_astar.hpp"
#include "memory_limits.hpp"

#include "pl/misc/term_utils.hpp"


lib_seach_astar::lib_seach_astar(interpreter &pl, char *c_stack_limit)
: m_c_stack_limit {c_stack_limit},
  m_pl {pl}
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
lib_seach_astar::print_stats(std::ostream &os) const noexcept
{
  const double totwords = m_pl.heap_size();
  double remwords = m_pl.heap_remsize();
  if (remwords < 0)
    remwords = totwords - remwords;
  const double heapusage = (totwords - remwords) * 100 / totwords;

  os << "A* profile:\n"
     << "- heap usage  " << heapusage << "%" << std::endl
     << "- graph size " << m_astar->graph_size() << std::endl
     << "- frontier size " << m_astar->frontier_size() << std::endl
     << "- install " << m_astar->ninstalls << std::endl
     << "- reject " << m_astar->nrejects << std::endl
     << "- update " << m_astar->nupdates << std::endl
     << "- restart " << m_astar->nrestarts << std::endl;
}


void
lib_seach_astar::graph_entry(runtime &rt, size_t argc, object_iterator argv,
                             continuation &cont)
{
  assert(argc == 0);

  if (m_astar.has_value())
    raise(m_pl, term("graph_evaluation_error", term("nested_graph")));

  // Initialize library
  // - init A*
  m_astar.emplace();
  // - establish choice point
  rt.push_choice_point(&root_cp);

  // First round of sprouts
  m_acc = 0;
  cont(rt, 0, 0, 0, 0);

  // Keep growing until all sprouts have been exhausted or until cut
  while (not m_astar->empty() and not root_cp.cut)
  {
    astar::iterator it = m_astar->pop();
    state st = it->second.drain_state();
    m_acc = it->second.gn;
    st.cont(st.rt, 0, 0, 0, 0);
  }

  print_stats(std::clog);

  rt.pop_choice_point(&root_cp);
  m_astar.reset();
}


void
lib_seach_astar::yield(runtime &rt, size_t argc, object_iterator argv,
                       continuation &cont)
{
  assert(argc == 3);
  if (memory_limit_crossed(rt, 500 * (1 << 10), m_c_stack_limit))
  {
    std::clog << "memory limits reached" << std::endl;
    return;
  }

  basic_decoder dc;

  // Record upper bound on heap usage
  m_minremwords = std::min(m_minremwords, m_pl.heap_remsize());

  // argv[0] Length of the edge from the previous node (provided explicitly)
  const object_view edgelen = rt.reduce(dc.decode_object(argv));
  const float len =
      number(m_pl, edgelen.begin(), [](auto x) { return float(x); });

  // argv[1] Node name
  const object_view nodenam = rt.reduce(dc.decode_object(argv));
  object node = rt.reconstruct(nodenam);

  // argv[2] Heuristic distance to destination
  const object_view heurlen = rt.reduce(dc.decode_object(argv));
  const float hn =
      number(m_pl, heurlen.begin(), [](auto x) { return float(x); });

  // Suspend the sprout
  const float gn = m_acc + len;
  const auto it = m_astar->put(node, {rt, std::move(cont)}, gn, hn);
  if (it != m_astar->end())
    _lock_heap(rt, &root_cp);
}