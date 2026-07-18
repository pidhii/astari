#pragma once

#include "pl/core/interpreter.hpp"
#include "utl/state_saver.hpp"

#include <list>


class lib_tabulate {
  public:
  lib_tabulate(interpreter &pl)
  {
    // tabulate/1
    pl.add_meta_op("tabulate", [this, &pl](runtime &rt, int argc,
                                           object_iterator argv,
                                           const continuation &cont) {
      assert(argc == 1);
      basic_decoder dc;
      basic_encoder ec;
      const object_view goal = rt.reduce(dc.decode_object(argv));

      const object_view goalview = _snapshot(rt, goal);

      if (auto it = m_table.find(goalview); it != m_table.end())
      {
        _unsnapshot(goalview);
        if (it->second.is_building)
          return pl.make_true(rt, goal, cont);

        // if (not it->second.solutions.empty())
        // {
        //   std::cerr << std::format("hit {} [table size = {}]", pl.dump(goalview),
        //                           it->second.solutions.size())
        //             << std::endl;
        // }
        for (const object &variant : it->second.solutions)
        {
          state_saver _ {rt};
          const object_view g = rt.adopt(variant);
          [[maybe_unused]] const bool ok = rt.match(goal, g);
          assert(ok);
          cont(rt);
        }
        return;
      }

      // std::cerr << "miss " << pl.dump(goalview) << std::endl;
      m_table[goalview].is_building = true;
      std::list<runtime> todo;
      pl.make_true(rt, goal, [&](runtime &rt) {
        table_entry &entry = m_table[goalview];
        entry.solutions.push_back(rt.reconstruct(goal));
        todo.emplace_back(rt);
      });
      m_table[goalview].is_building = false;

      for (runtime &rt : todo)
        cont(rt);
    });
  }

  private:
  object_view
  _snapshot(runtime &rt, object_view obj)
  {
    state_saver _ {m_cache};
    return m_cache.adopt(rt.reconstruct(obj));
  }

  void
  _unsnapshot(object_view snapshot)
  {
    state_saver _ {m_cache};
    return m_cache.unallocate(obj);
  }

  private:
  struct table_entry {
    bool is_building;
    std::vector<object> solutions;
  };
  std::unordered_map<object_view, table_entry> m_table;
  runtime m_cache;
};