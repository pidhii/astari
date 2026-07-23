#include "iso.hpp"

#include "utl/state_saver.hpp"


void
iso_throwcatch(interpreter &pl)
{
  // throw/1
  // pl.add_meta_op("throw", [&](runtime &rt, int argc, object_iterator argv,
  //                             const continuation &cont) {
  //   assert_arity(pl, "throw", argc, 1);
  //   basic_decoder dc;
  //   pl.raise(rt.reconstruct(dc.decode_object(argv)));
  // });

  // catch/3
  // pl.add_meta_op("catch", [&](runtime &rt, int argc, object_iterator argv,
  //                             const continuation &cont) {
  //   assert_arity(pl, "catch", argc, 3);
  //   basic_decoder dc;
  //   const object_view goal = dc.decode_object(argv);
  //   const object_view catcher = dc.decode_object(argv);
  //   const object_view handler = dc.decode_object(argv);
  //   struct pass { exception exn; };
  //   try
  //   {
  //     state_saver _ {rt};
  //     pl.make_true(rt, goal, [cont](runtime &rt) {
  //       try { cont(rt); }
  //       catch (const exception &exn)
  //       { throw pass {exn}; }
  //     });
  //   }
  //   catch (const pass &pass)
  //   {
  //     throw pass.exn;
  //   }
  //   catch (const exception &exn)
  //   {
  //     const object_view exnterm = rt.adopt(exn.term());
  //     if (rt.match(catcher, exnterm))
  //       TAILCALL pl.make_true(rt, handler, cont);
  //     else
  //       rt.unallocate(exnterm);
  //   }
  // });


}