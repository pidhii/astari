#pragma once

#include "pl/coding/basic_decoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "utl/state_saver.hpp"

#include <map>
#include <iostream>


[[noreturn]] inline void
assert_arity(interpreter &pl, std::string_view who, int argc)
{ pl.raise(term("arity_error", term(who), argc)); }

template <typename ...Args>
void
assert_arity(interpreter &pl, std::string_view who, int argc, int n, Args ...args)
{ if (argc != n) assert_arity(pl, who, argc, args...); }



struct iso_io {
  dictionary &symbols;
  const object_view stdout_term, stderr_term, stdin_term;
  std::map<word_t, std::pair<object_view, std::unique_ptr<std::ostream>>> ostreams;
  std::map<word_t, std::pair<object_view, std::unique_ptr<std::istream>>> istreams;
  object_view current_output, current_input;

  iso_io(interpreter &pl)
  : symbols {pl.symbols()},
    stdout_term {pl.make_term(term("stdout"))},
    stderr_term {pl.make_term(term("stderr"))},
    stdin_term {pl.make_term(term("stdin"))},
    current_output {stdout_term},
    current_input {stdin_term}
  {
    // current_output/1
    pl.add_meta_op("current_output", [&](runtime &rt, int argc,
                                        object_iterator argv,
                                        const continuation &cont) {
      assert_arity(pl, "current_output", argc, 1);
      basic_decoder dc;
      if (rt.match(dc.decode_object(argv), current_output))
        cont(rt);
    });
  }

  std::ostream &
  get_output(object_view s)
  {
    if (is_term(s))
    {
      if (s == stdout_term)
        return std::cout;
      else if (s == stderr_term)
        return std::cerr;
      else if (auto it = ostreams.find(s[0]); it != ostreams.end())
        return *it->second.second;
      else
        throw std::runtime_error {
            std::format("no such output stream ({})", dump_object(symbols, s))};
    }
    else
      throw std::runtime_error {
          std::format("invalid output stream ({})", dump_object(symbols, s))};
  }
};


void iso_writing_terms(iso_io &io, interpreter &pl);
void iso_writing_characters(iso_io &io, interpreter &pl);

void iso_type_testing(interpreter &pl);
void iso_term_comparison(interpreter &pl);

void iso_arithmetics(interpreter &pl);

struct iso {
  iso_io io;

  iso(interpreter &pl)
  : io {pl}
  {
    ////////////////////////////////////////////////////////////////////////////
    // Control constructs.
    //
    pl << R"(
      true.

      call(Goal) :-
        Goal.
    )";
    // throw/1
    pl.add_meta_op("throw", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
      assert_arity(pl, "throw", argc, 1);
      basic_decoder dc;
      pl.raise(rt.reconstruct(dc.decode_object(argv)));
    });
    // catch/3
    pl.add_meta_op("catch", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
      assert_arity(pl, "catch", argc, 3);
      basic_decoder dc;
      const object_view goal = dc.decode_object(argv);
      const object_view catcher = dc.decode_object(argv);
      const object_view handler = dc.decode_object(argv);
      try
      {
        state_saver _ {rt};
        pl.make_true(rt, goal, cont);
      }
      catch (const exception &exn)
      {
        const object_view exnterm = pl.adopt(exn.term());
        if (rt.match(catcher, exnterm))
          pl.make_true(rt, handler, cont);
      }
    });

    ////////////////////////////////////////////////////////////////////////////
    // Logic and Control
    //
    pl << R"(
      \+ Goal :-
        Goal -> fail;
        true.

      repeat.
      repeat :-
        repeat.
    )";
    // once/1
    pl.add_meta_op("once", [&](runtime &rt, int argc, object_iterator argv,
                               const continuation &cont) {
      basic_decoder dc;
      assert_arity(pl, "once", argc, 1);
      word_t *once = rt.allocate(1);
      *once = false;
      pl.make_true(rt, dc.decode_object(argv), [&](runtime &rt) {
        if (not *once)
        {
          cont(rt);
          *once = true;
        }
      });
    });

    ////////////////////////////////////////////////////////////////////////////
    // Unification
    //
    pl << R"(
      X = X.

      X \= Y :-
        \+ X = Y.
    )";

    ////////////////////////////////////////////////////////////////////////////
    // halt/0, halt/1
    //
    pl.add_meta_op("halt", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
      assert_arity(pl, "halt", argc, 0, 1);
      if (argc == 1)
        pl.number(rt, argv, std::exit);
      else
        std::exit(0);
    });

    // Type testing
    iso_type_testing(pl);

    // Term comparison
    iso_term_comparison(pl);

    // I/O
    iso_writing_terms(io, pl);
    iso_writing_characters(io, pl);

    // Arithmetics
    iso_arithmetics(pl);
  }
};