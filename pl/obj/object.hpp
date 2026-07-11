#pragma once

#include <cstdint>
#include <stdexcept>


using word_t = uint64_t;

/*
  # Terminals

  Blob:
  o bits (2-0]: 00
  o bits (64-2]: address of the data descriptor

  Structure: 
  o bits (2-0]: 01
  o bits (8-2]: reserved
  o bits (16-8]: arity         # second byte
  o bits (64-16]: id           # 6 bytes

  Small numbers:
  o bits (2-0]: 10
  o bits (8-2]: reserved
  o bits (16-8]:
    + 0x01 - signed integer
    + 0x02 - unsigned integer
    + 0x03 - float
  o bits (64-32]: value


  # Non-terminals

  Variable:
  o bits (2-0]: 11
  o bits (8-2]: reserved
  o bits (64-8]: id           # 7 bytes


  # Blob data descriptor

  struct { uint64_t tag; void data[]; }

*/


struct term_header_layout { word_t tag:2, reserved:6, arity:8, id:48; };
static_assert(sizeof(term_header_layout) == sizeof(word_t));

struct nonterminal_layout { word_t tag:2, reserved:6, id:56; };
static_assert(sizeof(nonterminal_layout) == sizeof(word_t));



struct term_header { word_t id; word_t arity; };
struct nonterminal { word_t id; };

enum class word_type {
  blob,
  structure,
  signed_int_number,
  unsigned_int_number,
  float_number,
  nonterminal,
};


[[gnu::pure]] inline word_type
word_type(word_t word)
{
  switch (word & 0b11)
  {
    case 0b00: return word_type::blob;
    case 0b01: return word_type::structure;
    case 0b10:
      switch ((word >> 8) & 0xFF)
      {
        case 0x01: return word_type::signed_int_number;
        case 0x02: return word_type::unsigned_int_number;
        case 0x03: return word_type::float_number;
        default: throw std::runtime_error {"invalid numeric type"};
      }
    case 0b11: return word_type::nonterminal;
    default: throw std::runtime_error {"invalid word type"};
  }
}

inline bool
is_nonterminal(word_t word)
{ return (word & 0b11) == 0b11; }

inline bool
is_term(word_t word)
{ return (word & 0b11) == 0b01; }



using object = std::basic_string<word_t>;
using object_view = std::basic_string_view<word_t>;
using object_iterator = object_view::const_iterator;
