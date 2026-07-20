#pragma once

#include "pl/dictionary.hpp"
#include "pl/misc/object_allocator.hpp"
#include "pl/obj/object.hpp"

#include <vector>
#include <istream>
#include <ostream>
#include <iostream>


struct object_file {
  dictionary symbols;
  std::vector<object> objects;

  void
  write(std::ostream &out);

  void
  read(std::istream &in, object_allocator &alloc);
  
  void
  write_v0(std::ostream &out);

  void
  read_v0(std::istream &in, object_allocator &alloc);
  
};

