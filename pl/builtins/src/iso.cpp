#include "iso.hpp"


iso::iso(interpreter &pl)
: io {pl}
{
  pl.load_objfile(PLO_PATH_iso_basic);

  // once/1
  pl.add_meta_op("once", [&](runtime &rt, int argc, object_iterator argv,
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
  pl.add_meta_op("halt", [&](runtime &rt, int argc, object_iterator argv,
                             const continuation &cont) {
    assert_arity(pl, "halt", argc, 0, 1);
    if (argc == 1)
      pl.number(rt, argv, std::exit);
    else
      std::exit(0);
  });

  // Type testing
  iso_type_testing(pl);

  // Term comparison
  iso_term_comparison(pl);

  // I/O
  iso_writing_terms(io, pl);
  iso_writing_characters(io, pl);

  // Arithmetics
  iso_arithmetics(pl);

  // Term Creation and Decomposition
  iso_term_creation_and_decomposition(pl);
}