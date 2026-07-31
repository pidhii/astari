#include "scan_depthfirst.hpp"
#include "scan_breadthfirst.hpp"
#include "find_astar.hpp"

#include "pl/core/interpreter.hpp"
#include "pl/builtins/iso.hpp"

#include <cstring>
#include <getopt.h>

#include <sys/resource.h>


int
main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "usage: astar <mode> <script>" << std::endl;
    return -1;
  }

  rlimit lim;
  getrlimit(RLIMIT_STACK, &lim);
  std::clog << "stack limit: " << (double(lim.rlim_cur) / (1 << 20)) << " [MB]"
            << std::endl;
  char *const c_stack_limit = argv[0] - (lim.rlim_cur - (1 << 20));

  interpreter pl;
  iso _iso {pl, iso_all};

  std::optional<lib_scan_depthfirst> lib_scan_dfs;
  std::optional<lib_scan_breadthfirst> lib_scan_bfs;
  std::optional<lib_seach_astar> lib_find_astar;
  if (std::strcmp(argv[1], "scan-dfs") == 0)
    lib_scan_dfs.emplace(pl, c_stack_limit);
  else if (std::strcmp(argv[1], "scan-bfs") == 0)
    lib_scan_bfs.emplace(pl, c_stack_limit);
  else if (std::strcmp(argv[1], "find-astar") == 0)
    lib_find_astar.emplace(pl, c_stack_limit);

  pl.load_file(argv[2]);
  pl.eval("main");
}
