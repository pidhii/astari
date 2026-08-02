#pragma once

#include "pl/core/interpreter.hpp"
#include "pl/misc/object_allocator.hpp"

#include <list>


class lib_tabulate {
  public:
  lib_tabulate(interpreter &pl)
  {
    // tabulate/1
    pl.add_meta_op("tabulate", [this, &pl](runtime &rt, size_t argc,
                                           object_iterator argv,
                                           const continuation &cont) {
      assert(argc == 1);
      basic_decoder dc;
      const object_view goal = rt.reduce(dc.decode_object(argv));

      const object_view goalview = _snapshot(rt, goal);

      if (auto it = m_table.find(goalview); it != m_table.end())
      {
        _unsnapshot(goalview);
        if (it->second.is_building)
          return;

        for (const object &variant : it->second.solutions)
        {
          barrier cp;
          rt.push_choice_point(&cp);
          const object_view g = rt.adopt_hp(variant);
          [[maybe_unused]] const bool ok = rt.match(goal, g);
          assert(ok);
          cont(rt, 0, 0, 0, 0);
          if (rt.uwuc(&cp))
            break;
        }
        return;
      }

#define VER 1
#if VER == 1
      barrier buildcp;
      rt.push_choice_point(&buildcp);
      {
        table_entry &ent = m_table[goalview];
        ent.is_building = true;
        ent.build_cp = &buildcp;
      }
      std::list<runtime> todo;
      pl.make_true(rt, goal, continuation::from_lambda([&](CONT_ARGS) {
        table_entry &entry = m_table[goalview];
        entry.solutions.push_back(rt.reconstruct(goal));
        rt.lock_heap_exc(&buildcp);
        todo.emplace_back(rt);
      }));
      m_table[goalview].is_building = false;

      for (runtime &rt : todo)
        cont(rt, 0, 0, 0, 0);

      rt.unwind(&buildcp);
#else
      m_table[goalview].is_building = true;
      pl.make_true(rt, goal, [&](runtime &rt) {
        table_entry &entry = m_table[goalview];
        entry.solutions.push_back(rt.reconstruct(goal));
        cont(rt);
      });
      m_table[goalview].is_building = false;

#endif
    });
  }

  private:
  object_view
  _snapshot(runtime &rt, object_view obj)
  {
    object recobj = rt.reconstruct(obj);
    normalize(recobj, recobj.data());
    word_t *p = m_cache.allocate(recobj.size());
    std::copy(recobj.begin(), recobj.end(), p);
    return {p, recobj.size()};
  }

  void
  _unsnapshot(object_view obj)
  {
    m_cache.unallocate(obj);
  }


  private:
  struct table_entry {
    bool is_building;
    barrier *build_cp;
    std::vector<object> solutions;
  };
  std::unordered_map<object_view, table_entry> m_table;
  object_allocator m_cache;
};