#include "display.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/parse/lexer.hpp"

#include <format>
#include <sstream>


static void
_dump_escaped_string(std::string_view s, std::ostream &os)
{
  os << '"';
  for (const char c: s)
  {
    switch (c)
    {
      case '"': os << "\\\""; break;
      case '\\': os << "\\\\"; break;
      case '\n': os << "\\n"; break;
      case '\t': os << "\\t"; break;
      case '\r': os << "\\r"; break;
      case '\v': os << "\\v"; break;
      case '\f': os << "\\f"; break;
      case '\b': os << "\\b"; break;
      case '\a': os << "\\a"; break;
      case '\0': os << "\\0"; break;
      default: os << c; break;
    }
  }
  os << '"';
}

template <typename InIter>
void
_dump_object(const dictionary &dict, InIter &it, std::ostream &os, bool quoted,
             bool ignore_ops, bool numbervars)
{
  static lexer lex;
  static basic_decoder dc;
  
  switch (word_type(*it))
  {
    case word_type::blob:
    {
      switch (blob_tag(*it))
      {
        case blob_tag::string:
          if (quoted)
            _dump_escaped_string(string(*it++), os);
          else
            os << string(*it++);
          return;

        default:
          os << std::format("blob(0x{:x})", *it++);
          return;
      }
    }
    
    case word_type::structure:
    {
      term_header hdr;
      dc.decode(*it++, hdr);
      const std::string_view name = dict[hdr.id];

      if (name == "nil" and hdr.arity == 0)
      {
        os << "[]";
        return;
      }

      if (name == "cons" and hdr.arity == 2)
      {
        os << "[";
        _dump_object(dict, it, os, quoted, ignore_ops, numbervars);

        for (;;)
        {
          if (word_type(*it) == word_type::structure)
          {
            term_header nexthdr;
            dc.decode(*it, nexthdr);
            const std::string_view nextname = dict[nexthdr.id];

            if (nextname == "nil" and nexthdr.arity == 0)
            {
              ++it;
              break;
            }

            if (nextname == "cons" and nexthdr.arity == 2)
            {
              ++it;
              os << ", ";
              _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
              continue;
            }
          }

          // improper list: dump the remaining tail after a '|'
          os << " | ";
          _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
          break;
        }

        os << "]";
        return;
      }

      if (not ignore_ops and lex.is_operator(name) and hdr.arity > 0)
      {

        if (name == "," and hdr.arity >= 2)
        {
          os << '(';
          for (size_t i = 0; i < hdr.arity; ++i)
          {
            if (i > 0) os << ", ";
            _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
          }
          os << ')';
          return;
        }

        switch (hdr.arity)
        {
          case 1:
            os << '(' << name << ' ';
            _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
            os << ')';
            return;

          case 2:
            os << '(';
            _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
            os << ' ' << name << ' ';
            _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
            os << ')';
            return;
        }
      }

      if (lex.is_symbol(name))
        os << name;
      else
      {
        if (quoted)
          os << "'" << name << "'";
        else
          os << name;
      }

      if (hdr.arity > 0)
      {
        os << "(";
        for (size_t i = 0; i < hdr.arity; ++i)
        {
          if (i > 0) os << ", ";
          _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
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
dump_object(const dictionary &dict, object_view obj, std::ostream &os,
            bool quoted, bool ignore_ops, bool numbervars)
{
  object_view::iterator it = obj.begin();
  _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
}

std::string
dump_object(const dictionary &dict, object_view obj, bool quoted,
            bool ignore_ops, bool numbervars)
{
  std::ostringstream os;
  object_view::iterator it = obj.begin();
  _dump_object(dict, it, os, quoted, ignore_ops, numbervars);
  return os.str();
}

