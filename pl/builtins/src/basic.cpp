#include "iso.hpp"
#include "pl/core/interpreter.hpp"


void
iso_basic(interpreter &pl)
{
  pl.load_objfile(PLO_PATH_iso_basic);

  // once/1
  pl.add_meta_op("once", [&](runtime &rt, size_t argc, object_iterator argv,
                             const continuation &cont) {
    assert_arity(pl, "once", argc, 1);
    basic_decoder dc;
    const object_view expr = dc.decode_object(argv);
    barrier cp;
    rt.push_choice_point(&cp);
    pl.make_true(rt, expr, [cont, &cp](runtime &rt) { rt.cut(&cp); cont(rt); });
    rt.pop_choice_point(&cp); // let someone else to unwind it if needed
  });

  ////////////////////////////////////////////////////////////////////////////
  // halt/0, halt/1
  //
  pl.add_meta_op("halt", [&](runtime &rt, size_t argc, object_iterator argv,
                             const continuation &cont) {
    assert_arity(pl, "halt", argc, 0, 1);
    if (argc == 1)
      number(pl, rt.reduce(argv), std::exit);
    else
      std::exit(0);
  });
}
