#pragma once

#include "pl/core/interpreter.hpp"

#include <deque>
#include <list>
#include <stack>


class lib_breadthfirst {
  using _state = std::pair<runtime, continuation>;

  struct _tree {
    barrier root_cp;
    std::deque<_state> sprouts;
  };

  static void
  _lock_heap(runtime &rt, barrier *root_cp)
  {
    barrier *cp = rt.query()->cp;
    while (cp != root_cp)
    {
      if (cp->noreclaim)
        break; // rest was already marked (TODO: find a way to assure this)
      cp->noreclaim = true;
      cp = cp->prev;
    }
  }

  std::stack<_tree, std::list<_tree>> m_trees;

  public:
  lib_breadthfirst(interpreter &pl);
};
