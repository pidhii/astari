#pragma once

#include "pl/core/interpreter.hpp"

#include <list>


class lib_breadthfirst {
  using state = std::pair<runtime, continuation>;

  public:
  lib_breadthfirst(interpreter &pl)
  {
    pl.add_meta_op("breadthfirst", [this](runtime &rt, int argc,
                                          object_iterator argv,
                                          const continuation &cont) {
      assert(argc == 0);
      cont(rt);
      while (not m_breadth_nodes.empty())
      {
        state st = m_breadth_nodes.front();
        m_breadth_nodes.pop_front();
        st.second(st.first);
      }
    });

    pl.add_meta_op("yield", [this](runtime &rt, int argc, object_iterator argv,
                                   const continuation &cont) {
      assert(argc == 0);
      m_breadth_nodes.emplace_back(rt, cont);
    });
  }

  private:
  std::list<state> m_breadth_nodes;
};
