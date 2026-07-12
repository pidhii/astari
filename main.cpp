#include "pl/builtins/iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/obj/object.hpp"
#include "pl/parse/syntax_parser.hpp"

#include <iostream>

#include <getopt.h>


static void
test()
{
  // Test state saving
  {
    interpreter prolog;

    prolog.add_meta_op("print", [&prolog](runtime &rt, size_t argc,
                                          object_iterator argv,
                                          const continuation &cont) {
      while (argc--)
      {
        const object_view x = basic_decoder().decode_object(argv);
        dump_object(prolog.symbols(), rt.reconstruct(x), std::cout);
        std::cout << " ";
      }
      std::cout << std::endl;
      cont(rt);
    });

    runtime save_rt;
    continuation save_cont;
    prolog.add_meta_op("save",
                       [&save_rt, &save_cont](runtime &rt, size_t argc,
                                              object_iterator argv,
                                              const continuation &cont) {
                         std::cout << "SAVE" << std::endl;
                         save_rt = rt;
                         save_cont = cont;
                       });

    // ---
    prolog << R"(
      test() :-
        print(beforeSave),
        save(),
        print(afterSave).
    )";

    std::cout << "START" << std::endl;
    prolog << "test";
    std::cout << "CONTINUE" << std::endl;
    save_cont(save_rt);
    std::cout << "FINISH" << std::endl;
  };
}


static void
repl(interpreter &pl)
{
  std::vector<token> tape;

  std::string line;
  const auto prompt = [&]() {
    std::cout << (tape.empty() ? "> " : "~ ");
    std::cout.flush();
  };
  while (prompt(), std::getline(std::cin, line))
  {
    lexer().tokenize(line, std::back_inserter(tape));
    if (not tape.empty() and tape.back().type == '.')
    {
      dictionary vardict;
      syntax_parser parser {pl.symbols(), vardict};
      load_default_grammar(parser);
      parser.load(tape.begin(), tape.end() - 1);
      tape.clear();
      try {
        const token tok = parser.parse();
        assert(tok.type == obj);
        pl.eval(std::get<object>(tok.val), vardict);
      }
      catch (const std::exception &exn)
      { std::cout << "error: " << exn.what() << std::endl; }
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
  iso _ {pl};

  if (optind < argc)
    pl.load_file(argv[optind]);

  repl(pl);
}
