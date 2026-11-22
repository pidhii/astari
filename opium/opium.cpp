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


static std::string
_read_file(const std::string_view path)
{
  std::ifstream is;
  is.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  try
  {
    is.open(path.data());
  }
  catch (const std::ios_base::failure &exn)
  {
    std::cerr << std::format("failed to open \"{}\" for reading ({})", path,
                             exn.what())
              << std::endl;
    exit(EXIT_FAILURE);
  }

  return {std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()};
}


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
  while ((opt = getopt_long(argc, argv, "hvo:L:", longopts, nullptr)) >= 0)
  {
    switch (opt)
    {
      case 'h':
        std::cout
          << std::format("usage: {} [OPTIONS] <script/path.osl>\n", argv[0])
          << std::format("\n")
          << std::format("OPTIONS:\n")
          << std::format("  -v          x1: log-level = verbose\n")
          << std::format("              x2: log-Level = debug\n")
          << std::format("  -o <opath>  output path\n")
          << std::format("  -L <dir>    library lookup path for Prolog\n")
          << std::endl;
        return EXIT_SUCCESS;

      case 'v':
        verbosity++;
        break;

      case 'o':
        opath = optarg;
        break;

      case 'L':
        pl.import_directory(optarg);
        break;

      default:
        std::cerr << "invalid option (" << argv[optind] << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
  }

  //
  //                        Init internals
  //
  // pl.import_directory("../pl/libs");
  pl.ensure_loaded("core/index.pl");
  pl.ensure_loaded("emitters/scheme.pl");
  pl.ensure_loaded("parsers/opium.pl");
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
  const std::string text = _read_file(argv[optind]);
  const word_t textobj = strdup(pl.global_memory(), text);
  pl.asserta(make_term(pl, term("input", object_view {&textobj, 1})));

  //
  //                          Run in
  //
  const clock_t start = clock();
  pl.make_true("input(A), runall(A, parse, emit)");
  const clock_t end = clock();
  std::cerr << std::fixed << std::setprecision(2) << "CPU time used: "
            << 1000.0 * (end - start) / CLOCKS_PER_SEC << "ms\n";
}