#include "pl/dictionary.hpp"
#include "pl/misc/object_file.hpp"
#include "pl/parse/prolog_parser.hpp"

#include <fstream>

int
main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "usage: astari-plo <path>" << std::endl;
    return 1;
  }

  const std::string inpath = argv[1];

  object_file objfile;

  { // fill in `objfile`
    prolog_parser p;
    std::ifstream file {inpath, std::ios_base::binary};
    tokens toks = p.tokenize(file);
    p.parse_stmts(objfile.symbols, toks, std::back_inserter(objfile.objects));
  }

  { // write out `objfile`
    const std::string opath =
        inpath.ends_with(".pl") ? inpath + "o" : inpath + ".plo";
    std::ofstream file {opath};
    objfile.write(file);
  }
}