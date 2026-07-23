#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>


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


static constexpr word_t magic_mask = word_t(0b11111100);
static constexpr word_t term_mask = ~magic_mask;

struct term_header_layout { word_t tag:2, reserved:6, arity:8, id:48; };
static_assert(sizeof(term_header_layout) == sizeof(word_t));

struct nonterminal_layout { word_t tag:2, reserved:6, id:56; };
static_assert(sizeof(nonterminal_layout) == sizeof(word_t));

[[nodiscard, gnu::pure]] constexpr inline word_t
the_word(word_t w)
{
  const word_t o[2] = {w, w & term_mask};
  return o[(w & 0b11ull) == 0b01ull];
}

[[nodiscard, gnu::pure]] constexpr inline word_t
word_magic(word_t w)
{ return (w & magic_mask) >> 2; }

[[nodiscard, gnu::pure]] constexpr inline word_t
add_magic(word_t w, word_t magic)
{ return w | ((magic << 2) & magic_mask); }

[[nodiscard, gnu::pure]] constexpr inline word_t
as_magic(word_t magic)
{ return ((magic << 2) & magic_mask); }



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


[[gnu::pure]] inline std::string_view
to_string(word_type type)
{
  switch (type)
  {
    case word_type::blob: return "blob";
    case word_type::structure: return "structure";
    case word_type::signed_int_number: return "signed_int_number";
    case word_type::unsigned_int_number: return "unsigned_int_number";
    case word_type::float_number: return "float_number";
    case word_type::nonterminal: return "nonterminal";
    default: throw std::runtime_error {"invalid word type"};
  }
}


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

[[nodiscard, gnu::pure]] inline bool
is_nonterminal(word_t word)
{ return (word & 0b11) == 0b11; }

[[nodiscard, gnu::pure]] inline bool
is_term(word_t word)
{ return (word & 0b11) == 0b01; }

[[nodiscard, gnu::pure]] inline bool
is_blob(word_t word)
{ return (word & 0b11) == 0b00; }


using object = std::basic_string<word_t>;
using object_view = std::basic_string_view<word_t>;
using object_iterator = object_view::const_iterator;

inline bool
is_term(object_view obj)
{ return not obj.empty() and is_term(obj[0]); }



enum class blob_tag: word_t {
  string,
};

struct blob { blob_tag tag; };
struct string_data: blob { size_t size; char data[]; };

inline blob_tag
blob_tag(word_t word)
{ return reinterpret_cast<blob*>(word)->tag; }

inline std::string_view
string(word_t word)
{
  const string_data *p = reinterpret_cast<string_data*>(word);
  return {p->data, p->size};
}


namespace std {
template <>
struct hash<object_view> {
  size_t
  operator () (object_view x) const noexcept
  {
    std::hash<std::string_view> hsh;
    return hsh({(const char*)x.data(), x.size() * sizeof(word_t)});
  }
};

template <>
struct hash<object> {
  size_t
  operator () (const object &x) const noexcept
  {
    std::hash<std::string_view> hsh;
    return hsh({(const char*)x.data(), x.size() * sizeof(word_t)});
  }
};
}

static void
prune(object &obj)
{
  for (word_t &w : obj)
    w = the_word(w);
}
