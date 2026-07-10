#pragma once

#include "../obj/object.hpp"

#include <cassert>


class basic_encoder {
  public:
  word_t
  encode(void *blob)
  {
    assert((word_t(blob) & 0b11) == 0);
    return reinterpret_cast<word_t>(blob);
  }

  word_t
  encode(const term_header &sh)
  {
    assert(sh.id <= 0xffffffffffffULL);
    assert(sh.arity <= 0xff);
    const word_t result =
        (word_t(sh.id) << 16) | (word_t(sh.arity) << 8) | word_t(0b01);
    return result;
  }

  word_t
  encode(int32_t x)
  {
    const word_t xword = static_cast<uint32_t>(x);
    return (xword << 32) | word_t(0x01 << 8) | word_t(0b10);
  }

  word_t
  encode(uint32_t x)
  {
    const word_t xword = static_cast<uint32_t>(x);
    return (xword << 32) | word_t(0x02 << 8) | word_t(0b10);
  }

  word_t
  encode(float x)
  {
    const word_t xword = *reinterpret_cast<uint32_t*>(&x);
    return (xword << 32) | word_t(0x03 << 8) | word_t(0b10);
  }

  word_t
  encode(nonterminal nonterm)
  {
    assert(nonterm.id <= 0xffffffffffffffULL);
    return (word_t(nonterm.id) << 8) | word_t(0b11);
  }
};
