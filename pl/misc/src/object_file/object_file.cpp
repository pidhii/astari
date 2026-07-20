#include "object_file.hpp"


void
object_file::write(std::ostream &out)
{ write_v0(out); }


void
object_file::read(std::istream &in, object_allocator &alloc)
{ read_v0(in, alloc); }
