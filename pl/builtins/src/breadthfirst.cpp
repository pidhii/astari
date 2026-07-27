#include "breadthfirst.hpp"

#include "pl/misc/term_utils.hpp"


lib_breadthfirst::lib_breadthfirst(interpreter &pl)
{
  basic_encoder ec;

  pl.dynamic(ec.encode(term_header(pl.symbols()["yield"], 0)));

  pl.add_meta_op("breadthfirst", [&](runtime &rt, size_t argc,
                                        object_iterator argv,
                                        const continuation &cont) {
    assert(argc == 1);
    basic_decoder dc;
    const object_view goal = dc.decode_object(argv);

    if (m_trees.size() > 1)
      throw std::runtime_error {"nested breadthfirst (unimplemented)"};

    _tree &t = m_trees.emplace();
    rt.push_choice_point(&t.root_cp);

    const object yield_sign = make_term(pl, term("yield"));
    const object yield_body = make_term(pl, term("$yield0"));
    runtime::recovery recov = rt.asserta_dyn(yield_sign, yield_body);

    // Wrap `cont` into an exit handle (exit from the breadthfirst-ed clause)
    // that will retract the yield/0
    const auto exitcont = [&](runtime &rt) {
      const auto it = rt.variants_begin(yield_sign[0]);
      assert(std::next(it) == rt.variants_end());
      runtime::recovery recov = rt.retract_dyn(yield_sign[0], it);
      cont(rt);
      rt.recover(std::move(recov));
    };

    // First round of sprouts
    pl.make_true(rt, goal, exitcont);

    // Keep growing until all sprouts have exhausted (or until a cut)
    while (not t.sprouts.empty() and not t.root_cp.cut)
    {
      auto [srt, scont] = std::move(t.sprouts.front());
      t.sprouts.pop_front();
      scont(srt);
    }

    rt.recover(std::move(recov));
    rt.pop_choice_point(&t.root_cp);
    m_trees.pop();
  });

  pl.add_meta_op("$yield0", [this](runtime &rt, size_t argc,
                                   object_iterator argv,
                                   const continuation &cont) {
    assert(argc == 0);
    _tree &t = m_trees.top();
    t.sprouts.emplace_back(rt, cont);
    _lock_heap(rt, &t.root_cp);
  });


}