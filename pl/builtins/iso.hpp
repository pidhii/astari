#pragma once

#include "pl/core/interpreter.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/term_utils.hpp"
#include "pl/obj/object.hpp"
#include "pl/parse/lexer.hpp"

#include <iostream>
#include <map>
#include <type_traits>


[[noreturn]] inline void
assert_arity(interpreter &pl, std::string_view who, size_t argc)
{ raise(pl, term("arity_error", term(who), int(argc))); }

template <typename... Args>
void
assert_arity(interpreter &pl, std::string_view who, size_t argc, size_t n,
             Args... args)
{ if (argc != n) assert_arity(pl, who, argc, args...); }



struct iso_io {
  interpreter &pl;
  dictionary &symbols;
  const object stdout_term, stderr_term, stdin_term;
  std::map<word_t, std::pair<object_view, std::unique_ptr<std::ostream>>>
      ostreams;
  std::map<word_t, std::pair<object_view, std::unique_ptr<std::istream>>>
      istreams;
  object_view current_output, current_input;

  iso_io(interpreter &pl);

  template <typename T, typename... Args>
  void
  open(const object_view key, Args &&...args)
  {
    using stream_type = std::remove_cvref_t<T>;
    if constexpr (std::is_base_of_v<std::ostream, stream_type>)
    {
      if (ostreams.contains(the_word(key[0])))
        throw std::runtime_error {"stream name already occupied"};
      ostreams.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(the_word(key[0])),
          std::forward_as_tuple(key, std::make_unique<stream_type>(std::forward<Args>(args)...)));
    }
    else
    {
      if (istreams.contains(the_word(key[0])))
        throw std::runtime_error {"stream name already occupied"};
      istreams.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(the_word(key[0])),
        std::forward_as_tuple(key, std::make_unique<stream_type>(std::forward<Args>(args)...)));
    }
  }

  template <typename T, typename... Args>
  void
  open(std::string_view key, Args &&...args)
  {
    object keyterm = make_term(pl, term(key));
    object_view keyobj = pl.global_memory().allocate_object(keyterm.size());
    std::copy(keyterm.begin(), keyterm.end(), const_cast<word_t*>(keyobj.data()));
    open<T>(keyobj, std::forward<Args>(args)...);
  }

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
void iso_atom_processing(interpreter &pl);

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
  atom_processing                 = BIT(10),
};
#undef BIT
static constexpr unsigned iso_all = -1;

struct iso {
  iso_io io;

  iso(interpreter &pl, unsigned libs = iso_all);
};