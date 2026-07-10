#pragma once

#include "../obj/object.hpp"


class basic_decoder {
  public:
  void
  decode(word_t word, void *&blob)
  { blob = reinterpret_cast<void*>(word); }

  void
  decode(word_t word, term_header &t)
  { t.id = word >> 16; t.arity = (word >> 8) & 0xFF; }

  void
  decode(word_t word, int32_t &x)
  { x = static_cast<int32_t>(static_cast<uint32_t>(word >> 32)); }

  void
  decode(word_t word, uint32_t &x)
  { x = static_cast<uint32_t>(word >> 32); }

  void
  decode(word_t word, float &x)
  {
    const uint32_t tmp = static_cast<uint32_t>(word >> 32);
    x = *reinterpret_cast<const float*>(&tmp);
  }
  void
  decode(word_t word, nonterminal &x)
  { x.id = word >> 8; }

  object_view
  decode_object(object_iterator &it)
  {
    const object_iterator objstart = it;
    if (word_type(*it) == word_type::structure)
    {
      term_header hdr;
      decode(*it++, hdr);
      for (size_t i = 0; i < hdr.arity; ++i)
        decode_object(it);
    }
    else
      it += 1;
    return object_view {objstart, it};
  }

  object_view
  decode_object(const object_iterator &it_)
  {
    object_iterator it = it_;
    return decode_object(it);
  }

  size_t
  count_objects(object_iterator it, object_iterator end)
  {
    size_t n = 0;
    while (it != end)
    {
      decode_object(it);
      n++;
    }
    return n;
  }
};
