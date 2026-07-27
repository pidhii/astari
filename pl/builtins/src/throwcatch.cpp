#include "iso.hpp"


void
iso_throwcatch(interpreter &pl)
{
  // throw/1
  pl.add_meta_op("throw", [&](runtime &rt, int argc, object_iterator argv,
                              const continuation &cont) {
    assert_arity(pl, "throw", argc, 1);
    basic_decoder dc;
    raise(pl, rt.reconstruct(dc.decode_object(argv)));
  });

  // catch/3
  pl.add_meta_op("catch", [&](runtime &rt, int argc, object_iterator argv,
                              const continuation &cont) {
    assert_arity(pl, "catch", argc, 3);
    basic_decoder dc;
    const object_view goal = dc.decode_object(argv);
    const object_view catcher = dc.decode_object(argv);
    const object_view handler = dc.decode_object(argv);

    barrier cp;
    rt.push_choice_point(&cp);

    struct pass { exception exn; };
    try
    {
      pl.make_true(rt, goal, [cont](runtime &rt) {
        // Wrap further exceptions to distinguish them from those originating
        // from the goal clause
        try { cont(rt); }
        catch (const exception &exn)
        { throw pass {exn}; }
      });
    }
    catch (const pass &pass)
    {
      throw pass.exn;
    }
    catch (const exception &exn)
    {
      rt.query()->cp = &cp; // drop all choice points set after entering the goal
      rt.unwind(&cp);

      const object_view exnterm = rt.adopt_hp(exn.term());
      if (rt.match(catcher, exnterm))
        TAILCALL pl.make_true(rt, handler, cont);
    }
  });


}