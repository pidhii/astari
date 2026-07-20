#pragma once

#include "pl/coding/basic_decoder.hpp"
#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/obj/object.hpp"
#include "pl/parse/lexer.hpp"

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
      const object_view x = dc.decode_object(argv);
      if (rt.match(x, current_output))
        TAILCALL cont(rt);
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
void iso_term_creation_and_decomposition(interpreter &pl);
void iso_throwcatch(interpreter &pl);

struct iso {
  iso_io io;

  iso(interpreter &pl);
};