#pragma once

#include "object_allocator.hpp"

#include "pl/core/interpreter.hpp"
#include "pl/coding/tape_writer.hpp"


template <typename Object>
[[noreturn]] void
raise(interpreter &pl, Object what)
{
  object term;
  tape_writer tape {std::back_inserter(term), pl.symbols()};
  tape << what;

  std::ostringstream msg;
  dump_object(pl.symbols(), term, msg);

  throw exception {msg.str(), term};
}


object
make_list(interpreter &pl, runtime &rt, size_t n, object_iterator it);

std::pair<object, size_t>
unmake_list(interpreter &pl, runtime &rt, object_iterator it);

object
make_list(interpreter &pl, size_t n, object_iterator it);

std::pair<object, size_t>
unmake_list(interpreter &pl, object_iterator it);


word_t
strdup(object_allocator &alloc, std::string_view str);


template <typename Cont>
auto
number(interpreter &pl, object_iterator x, Cont &&c)
{
  basic_decoder dc;
  if (is_nonterminal(x[0]))
    raise(pl, term("instantiation_error"));

  switch (word_type(x[0]))
  {
    case word_type::signed_int_number:
    {
      int val;
      dc.decode(x[0], val);
      return c(val);
    }

    case word_type::unsigned_int_number:
    {
      unsigned val;
      dc.decode(x[0], val);
      return c(val);
    }

    case word_type::float_number:
    {
      float val;
      dc.decode(x[0], val);
      return c(val);
    }

    default:
      raise(pl, term("type_error", term("number"), dc.decode_object(x)));
  }
}


template <typename ...Args>
object
make_term(interpreter &pl, Args &&...args)
{
  object buf;
  tape_writer tape {std::back_inserter(buf), pl.symbols()};
  tape.operator<<(std::forward<Args>(args)...);
  return buf;
}


word_t
predicate_indicator(interpreter &pl, object_iterator x);


void
transfer_symbols(dictionary &from, dictionary &to, object &obj);


/**
 * @brief Recover variable names in the object
 * @details Substitute nonterminals that are bound to variables from @p ns with
 * the later.
 */
void
recover_variables(runtime &rt, const varnamespace &ns, object &obj);