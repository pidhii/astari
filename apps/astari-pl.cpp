#include "pl/builtins/breadthfirst.hpp"
#include "pl/builtins/iso.hpp"
#include "pl/builtins/parsing.hpp"
#include "pl/builtins/tabulate.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/parse/prolog_parser.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>

#include <getopt.h>
#include <readline/history.h>
#include <readline/readline.h>


static interpreter repl_pl;

static char*
repl_term_generator(const char *_text, int state)
{
  static std::string_view text;
  static std::vector<std::string_view> names;
  static size_t offset;

  if (state == 0)
  {
    text = _text;
    const auto ns = repl_pl.symbols().names();
    names.assign(ns.begin(), ns.end());
    offset = 0;
  }

  while (offset < names.size())
  {
    std::string_view name = names[offset++];
    if (name.starts_with(text))
      return strdup(name.data());
  }
  
  return nullptr;
}

static char **
repl_matches(const char *text, int start, int end)
{
  rl_attempted_completion_over = true;
  rl_completion_suppress_append = true;
  return rl_completion_matches(text, repl_term_generator);
}


static void
repl()
{
  basic_encoder ec;

  prolog_parser parser;
  tokens tokens;

  rl_attempted_completion_function = repl_matches;
  rl_variable_bind("show-all-if-ambiguous", "on");
  rl_completion_type = '?';

  while (char *_line = readline(tokens.list.empty() ? "?- " : " | "))
  {
    std::string line = _line;
    free(_line);
    parser.tokenize_more(tokens, line);

    const word_t dot = ec.encode(term_header(parser.symbols()["."], 0));
    if (not tokens.list.empty() and the_word(tokens.list.end()[-2]) == dot)
    {
      try {
        parser.pop_token(tokens);
        const object expr = parser.parse_expr(repl_pl.symbols(), tokens);
        const clock_t start = clock();
        repl_pl.eval(expr, tokens.vars);
        const clock_t end = clock();
        std::cerr << std::fixed << std::setprecision(2) << "CPU time used: "
                  << 1000.0 * (end - start) / CLOCKS_PER_SEC << "ms\n";
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

  while (optind < argc)
  {
    if (std::string_view(argv[optind]).ends_with(".pl"))
      repl_pl.load_file(argv[optind++]);
    else if (std::string_view(argv[optind]).ends_with(".plo"))
      repl_pl.load_objfile(argv[optind++]);
    else
    {
      std::cerr << std::format("unrecognized file format ({})", argv[optind])
                << std::endl;
      return -1;
    }
  }

  iso _ {repl_pl};
  lib_breadthfirst _bf {repl_pl};
  lib_tabulate _tab {repl_pl};
  lib_parsing _pars {repl_pl};

  repl_pl.eval("write(\"Hello World!\"), nl");
  repl();
}
