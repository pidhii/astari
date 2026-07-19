#pragma once

#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"

#include <istream>


class interpreter;

class lexer {
  public:
  std::pair<object, size_t>
  tokens(interpreter &pl, dictionary &vardict, std::istream &in);

  bool
  is_operator(std::string_view name);

  bool
  is_symbol(std::string_view name);

  private:
  word_t
  _read_elt(interpreter &pl, dictionary &vardict, std::istream &in,
            bool &quote) const;
};