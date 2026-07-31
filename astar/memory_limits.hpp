#pragma once

#include "pl/core/runtime.hpp"


static bool
memory_limit_crossed(const runtime &rt, ssize_t minremwords,
                     const char *c_stack_limit)
{
  // Check term stack
  const ssize_t remwords = rt.query()->heap_e - rt.query()->heap_p;
  if (remwords < minremwords)
    return true;

  // Check C stack
  void *sp;
  if ((char*)&sp < c_stack_limit)
    return true;

  return false;
}