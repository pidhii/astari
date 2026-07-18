#pragma once

#include <istream>
#include <stdexcept>


struct parse_error: std::runtime_error {
  using location = std::pair<std::istream::pos_type, std::istream::pos_type>;
  parse_error(const std::string what, location where)
  : runtime_error(what),
    where {where}
  { }

  location where;
};
