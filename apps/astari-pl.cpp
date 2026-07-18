#include "pl/builtins/breadthfirst.hpp"
#include "pl/builtins/iso.hpp"
#include "pl/builtins/parsing.hpp"
#include "pl/builtins/tabulate.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/parse/prolog_parser.hpp"

#include <iostream>

#include <getopt.h>


static void
repl(interpreter &pl)
{
  basic_encoder ec;

  prolog_parser parser;
  tokens tokens;


  const auto prompt = [&]() {
    std::cout << (tokens.list.empty() ? "> " : "~ ");
    std::cout.flush();
  };

  std::string line;
  while (prompt(), std::getline(std::cin, line))
  {
    parser.tokenize_more(tokens, line);

    const word_t dot = ec.encode(term_header(parser.symbols()["."], 0));
    if (not tokens.list.empty() and tokens.list.end()[-2] == dot)
    {
      try {
        parser.pop_token(tokens);
        const object expr = parser.parse_expr(pl.symbols(), tokens);
        pl.eval(expr, tokens.vars);
      }
      catch (const std::exception &exn)
      { std::cerr << "error: " << exn.what() << std::endl; }

      tokens.list.clear();
      tokens.vars.clear();
    }
  }
}


int
main(int argc, char **argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "h")) >= 0)
  {
    switch (optopt)
    {
    }
  }

  interpreter pl;

  while (optind < argc)
    pl.load_file(argv[optind++]);

  iso _ {pl};
  lib_breadthfirst _bf {pl};
  lib_tabulate _tab {pl};
  lib_parsing _pars {pl};

  pl.eval("write('Hello World!'), nl");
  // pl.eval("X is 1 + 2");

  pl.load_file("parser.pl");
  // pl.debug();

  repl(pl);
}
