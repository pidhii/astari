#include "scan_depthfirst.hpp"
#include "scan_breadthfirst.hpp"
#include "find_astar.hpp"

#include "pl/core/interpreter.hpp"
#include "pl/builtins/iso.hpp"

#include <filesystem>
#include <fstream>

#include <sys/resource.h>


static void
_print_help(std::ostream &os)
{
  os << "usage: 1) astar scan {dfs,bfs} <graph-file> <script>" << std::endl
     << "       3) astar find {astar} <script>" << std::endl;
}

static void
_invalid_arguments_error()
{
  _print_help(std::cerr);
  throw std::runtime_error {"invalid command-line arguments"};
}

static unsigned
_find_max_generation(const object_file &objfile)
{
  interpreter pl;
  pl.load_objfile(objfile);

  unsigned maxgen = 0;
  pl.make_true("generation(X)", [&](const interpreter::solution &sol) {
    const unsigned gen =
        number(pl, sol.at("X").data(), [](auto x) { return unsigned(x); });
    maxgen = std::max(gen, maxgen);
  });
  return maxgen;
}

static void
_remove_duplicates(object_file &objfile)
{
  // Filter unique objects into the basket
  std::set<object_view> basket;
  for (const object &obj : objfile.objects)
    basket.emplace(obj);
  // Assign objects from the basket
  objfile.objects.assign(basket.begin(), basket.end());
}

static void
_create_graph_objfile(interpreter &pl, const graph &graph,
                      std::string_view filename)
{
  object_file objfile;

  // Write graph to the object file as generation zero
  graph.write(objfile, pl.symbols(), 0);

  // Write the object file to disk
  std::ofstream os {filename.data()};
  objfile.write(os);
}

static void
_update_graph_objfile(interpreter &pl, const graph &graph,
                      std::string_view filename)
{
  object_file objfile;

  // Read object file from disk
  {
    std::ifstream is {filename.data()};
    objfile.read(is, pl.global_memory());
  }

  // Add new graph to the object file as a new generation
  const unsigned gen = 1 + _find_max_generation(objfile);
  graph.write(objfile, pl.symbols(), gen);

  // Remove duplicates
  _remove_duplicates(objfile);

  // Update object file on disk
  std::ofstream os {filename.data()};
  objfile.write(os);
}


int
main(int argc, char **argv)
{
  rlimit lim;
  getrlimit(RLIMIT_STACK, &lim);
  std::clog << "stack limit: " << (double(lim.rlim_cur) / (1 << 20)) << " [MB]"
            << std::endl;
  char *const c_stack_limit = argv[0] - (lim.rlim_cur - (1 << 20));

  int optind = 1;
  const auto getopt = [&]() -> std::string_view {
    return argv[optind];
  };
  const auto shiftopt = [&]() -> void {
    if (++optind >= argc)
      _invalid_arguments_error();
  };


  interpreter pl;
  iso _iso {pl, iso_all};

  std::optional<lib_scan_depthfirst> lib_scan_dfs;
  std::optional<lib_scan_breadthfirst> lib_scan_bfs;
  std::optional<lib_seach_astar> lib_find_astar;

  std::optional<std::string_view> graphfilename;
  std::optional<graph*> graphp;

  if (getopt() == "scan")
  {
    // Scan method
    shiftopt();
    if (getopt() == "dfs")
    {
      auto &lib = lib_scan_dfs.emplace(pl, c_stack_limit);
      graphp = &lib.graph();
    }
    else if (getopt() == "bfs")
    {
      auto &lib = lib_scan_bfs.emplace(pl, c_stack_limit);
      graphp = &lib.graph();
    }
    else
      _invalid_arguments_error();

    // Graph filename
    shiftopt();
    graphfilename = getopt();
  }
  else if (getopt() == "scan")
  {
    // Find method
    shiftopt();
    if (getopt() == "astar")
      lib_find_astar.emplace(pl, c_stack_limit);
    else
      _invalid_arguments_error();
  }
  else
    _invalid_arguments_error();

  shiftopt();
  pl.load_file(getopt());

  pl.eval("initial_state(S), graph(start_from(S))");

  if (not graphfilename.has_value())
    return 0;

  if (std::filesystem::exists(graphfilename->data()))
  {
    std::cout << "update graph object file" << std::endl;
    _update_graph_objfile(pl, *graphp.value(), graphfilename.value());
    return 0;
  }
  else
  {
    std::cout << "create graph object file" << std::endl;
    _create_graph_objfile(pl, *graphp.value(), graphfilename.value());
    return 0;
  }
}
