#include "display.hpp"

#include "pl/coding/basic_decoder.hpp"

#include <format>
#include <sstream>


template <typename InIter>
void
_dump_object(const dictionary &dict, InIter &it, std::ostream &os)
{
  basic_decoder dc;
  
  switch (word_type(*it))
  {
    case word_type::blob:
    {
      void *blob;
      dc.decode(*it++, blob);
      os << std::format("blob(0x{:x})", reinterpret_cast<word_t>(blob));
      break;
    }
    
    case word_type::structure:
    {
      term_header hdr;
      dc.decode(*it++, hdr);
      os << dict[hdr.id];
      if (hdr.arity > 0)
      {
        os << "(";
        for (size_t i = 0; i < hdr.arity; ++i)
        {
          if (i > 0) os << ", ";
          _dump_object(dict, it, os);
        }
        os << ")";
      }
      break;
    }
    
    case word_type::signed_int_number:
    {
      int32_t x;
      dc.decode(*it++, x);
      os << x;
      break;
    }
    
    case word_type::unsigned_int_number:
    {
      uint32_t x;
      dc.decode(*it++, x);
      os << x << "u";
      break;
    }
    
    case word_type::float_number:
    {
      float x;
      dc.decode(*it++, x);
      os << x << "f";
      break;
    }
    
    case word_type::nonterminal:
    {
      nonterminal x;
      dc.decode(*it++, x);
      os << std::format("@{}", size_t(x.id));
      break;
    }
  }
}

void
dump_object(const dictionary &dict, object_view obj, std::ostream &os)
{
  object_view::iterator it = obj.begin();
  _dump_object(dict, it, os);
}

std::string
dump_object(const dictionary &dict, object_view obj)
{
  std::ostringstream os;
  object_view::iterator it = obj.begin();
  _dump_object(dict, it, os);
  return os.str();
}

