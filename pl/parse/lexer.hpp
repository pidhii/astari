#pragma once

#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"

#include <istream>


class interpreter;

class lexer {
  public:
  std::pair<object, size_t>
  tokens(interpreter &pl, dictionary &vardict, std::istream &in);

  private:
  word_t
  _read_elt(interpreter &pl, dictionary &vardict, std::istream &in,
            bool &quote) const;
};