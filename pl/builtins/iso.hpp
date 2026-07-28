#pragma once

#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/term_utils.hpp"
#include "pl/obj/object.hpp"
#include "pl/parse/lexer.hpp"

#include <map>
#include <iostream>


[[noreturn]] inline void
assert_arity(interpreter &pl, std::string_view who, size_t argc)
{ raise(pl, term("arity_error", term(who), int(argc))); }

template <typename... Args>
void
assert_arity(interpreter &pl, std::string_view who, size_t argc, size_t n,
             Args... args)
{ if (argc != n) assert_arity(pl, who, argc, args...); }



struct iso_io {
  dictionary &symbols;
  const object stdout_term, stderr_term, stdin_term;
  std::map<word_t, std::pair<object_view, std::unique_ptr<std::ostream>>>
      ostreams;
  std::map<word_t, std::pair<object_view, std::unique_ptr<std::istream>>>
      istreams;
  object_view current_output, current_input;

  iso_io(interpreter &pl);

  std::ostream &
  get_output(object_view s);
};


void iso_basic(interpreter &pl);
void iso_writing_terms(iso_io &io, interpreter &pl);
void iso_writing_characters(iso_io &io, interpreter &pl);
void iso_type_testing(interpreter &pl);
void iso_term_comparison(interpreter &pl);
void iso_arithmetics(interpreter &pl);
void iso_term_creation_and_decomposition(interpreter &pl);
void iso_throwcatch(interpreter &pl);
void iso_all_solutions(interpreter &pl);
void iso_clause_creation_and_destruction(interpreter &pl);

#define BIT(n) (1 << (n))
enum iso_lib {
  basic                           = BIT(0),
  writing_terms                   = BIT(1),
  writing_characters              = BIT(2),
  io                              = writing_terms | writing_characters,
  type_testing                    = BIT(3),
  term_comparison                 = BIT(4),
  arithmetics                     = BIT(5),
  term_creation_and_decomposition = BIT(6),
  throwcatch                      = BIT(7),
  all_solutions                   = BIT(8),
  clause_creation_and_destruction = BIT(9),
};
#undef BIT
static constexpr unsigned iso_all = -1;

struct iso {
  iso_io io;

  iso(interpreter &pl, unsigned libs = iso_all);
};