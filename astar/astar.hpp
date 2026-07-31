#pragma once

#include "pl/core/runtime.hpp"
#include "pl/core/interpreter.hpp"

#include <map>


struct state {
  runtime rt;
  continuation cont;
};

struct node {
  std::optional<state> st;
  float gn, hn;
  bool is_frontier;

  state
  drain_state()
  {
    state ret = std::move(*st);
    st.reset();
    return ret;
  }
};


class astar {
  public:
  using node_container = std::map<object, node>;
  using iterator = node_container::iterator;
  using const_iterator = node_container::const_iterator;

  size_t ninstalls = 0;
  size_t nrejects = 0;
  size_t nupdates = 0;
  size_t nrestarts = 0;

  size_t
  graph_size() const noexcept
  { return m_nodes.size(); }

  size_t
  frontier_size() const noexcept
  { return m_frontier.size(); }

  bool
  empty() const noexcept
  { return m_frontier.empty(); }

  node_container::const_iterator
  begin() const noexcept
  { return m_nodes.begin(); }

  node_container::const_iterator
  end() const noexcept
  { return m_nodes.end(); }

  iterator
  pop();

  iterator
  put(object_view name, const state &st, float gn, float hn);

  private:
  node_container m_nodes;
  std::vector<node_container::iterator> m_frontier;
};
