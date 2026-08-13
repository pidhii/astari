#include "pl/builtins/iso.hpp"
#include "pl/builtins/parsing.hpp"
#include "pl/builtins/tabulate.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/misc/term_utils.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <getopt.h>


int
main(int argc, char **argv)
{
  interpreter pl;

  iso _iso {pl};
  lib_tabulate _tab {pl};
  lib_parsing _pars {pl};

  const option longopts[] = {
    {"verbose", false, 0, 'v'},
    {0, 0, 0, 0}
  };

  int verbosity = 0;
  std::string opath = "out.scm";
  int opt;
  while ((opt = getopt_long(argc, argv, "vo:", longopts, nullptr)) >= 0)
  {
    switch (opt)
    {
      case 'v':
        verbosity++;
        break;

      case 'o':
        opath = optarg;
        break;

      default:
        std::cerr << "invalid option (" << argv[optind] << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
  }

  //
  //                        Init internals
  //
  pl.import_directory("../pl/libs");
  pl.load_file("./core/index.pl");
  pl.load_file("./emitters/scheme.pl");
  pl.load_file("./parsers/opium.pl");
  switch (verbosity)
  {
    default: // fall through
    case 2: pl << "veryverbose."; // fall through
    case 1: pl << "verbose."; // fall through
    case 0: ;
  }
  _iso.io.open<std::ofstream>("emitter_output", opath);

  //
  //                        Load input file
  //
  // File contents will be stored as a predicate input/1, i.e.:
  //                     input(<text-string>)
  if (optind == argc)
  {
    std::cerr << "must specify input file" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::ifstream is;
  is.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  try { is.open(argv[optind]); }
  catch (const std::ios_base::failure &exn)
  {
    std::cerr << std::format("failed to open \"{}\" for reading ({})",
                             argv[optind], exn.what())
              << std::endl;
    exit(EXIT_FAILURE);
  }
  try {
    const std::string text {std::istreambuf_iterator<char>(is),
                            std::istreambuf_iterator<char>()};
    const word_t textstr = strdup(pl.global_memory(), text);
    pl.asserta(make_term(pl, term("input", object_view {&textstr, 1})));
  }
  catch (const std::ios_base::failure &exn)
  {
    std::cerr << std::format("failed to read from \"{}\" ({})", argv[optind],
                             exn.what())
              << std::endl;
    exit(EXIT_FAILURE);
  }

  const clock_t start = clock();
  pl.make_true("input(A), runall(A, parse, emit)");
  const clock_t end = clock();
  std::cerr << std::fixed << std::setprecision(2) << "CPU time used: "
            << 1000.0 * (end - start) / CLOCKS_PER_SEC << "ms\n";
}