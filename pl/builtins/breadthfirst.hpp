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
  _lock_heap(barrier *root_cp)
  {
    barrier *cp = ::choice_point;
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
  lib_breadthfirst(interpreter &pl)
  {
    pl.add_meta_op("breadthfirst", [this](runtime &rt, int argc,
                                          object_iterator argv,
                                          const continuation &cont) {
      assert(argc == 0);

      _tree &t = m_trees.emplace();
      rt.push_choice_point(&t.root_cp);

      // First round of sprouts
      cont(rt);

      // Keep growing untill all sprouts have exhausted
      while (not t.sprouts.empty())
      {
        auto [srt, scont] = std::move(t.sprouts.front());
        t.sprouts.pop_front();
        scont(srt);
      }

      rt.pop_choice_point(&t.root_cp);
      m_trees.pop();
    });

    pl.add_meta_op("yield", [this](runtime &rt, int argc, object_iterator argv,
                                   const continuation &cont) {
      assert(argc == 0);
      _tree &t = m_trees.top();
      t.sprouts.emplace_back(rt, cont);
      _lock_heap(&t.root_cp);
    });
  }
};
