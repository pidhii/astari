#include "object_file.hpp"


void
object_file::write(std::ostream &out)
{
  out << "v1\n";
  write_v1(out);
}


void
object_file::read(std::istream &in, object_allocator &alloc)
{
  // Handle version 0 that had no version identifier
  if (in.peek() != 'v')
    return read_v0(in, alloc);
  
  // Handle differrent versions
  std::string version;
  std::getline(in, version);
  if (version == "v1")
    return read_v1(in, alloc);
  else
    throw std::runtime_error {"unknown prolog object-file version: " + version};
}
