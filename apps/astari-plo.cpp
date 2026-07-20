#include "pl/dictionary.hpp"
#include "pl/misc/object_file.hpp"
#include "pl/parse/prolog_parser.hpp"

#include <fstream>


int
main(int argc, char **argv)
{
  if (argc != 2 and argc != 3)
  {
    std::cerr << "usage: astari-plo <path> [<opath>]" << std::endl;
    return 1;
  }

  const std::string inpath = argv[1];
  const std::string opath = argc == 3                 ? std::string {argv[2]}
                            : inpath.ends_with(".pl") ? inpath + "o"
                                                      : inpath + ".plo";

  object_file objfile;

  { // fill in `objfile`
    prolog_parser p;
    std::ifstream file {inpath, std::ios_base::binary};
    tokens toks = p.tokenize(file);
    p.parse_stmts(objfile.symbols, toks, std::back_inserter(objfile.objects));
  }

  { // write out `objfile`
    std::ofstream file {opath};
    objfile.write(file);
  }
}