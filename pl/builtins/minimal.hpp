#pragma once

#include "iso.hpp"


static void
minimal_predicates(interpreter &pl)
{
  pl.add_predicate(pl.make_term(term("true")));
  pl.add_predicate(pl.make_term(term("=", var("X"), var("X"))));

  iso_type_testing(pl); // Doesn't use parser
  iso_term_comparison(pl); // Doesn't use parser
  iso_term_creation_and_decomposition(pl); // Doesn't use parser

  // once/1
  pl.add_meta_op("once", [&](runtime &rt, int argc, object_iterator argv,
                              const continuation &cont) {
    assert_arity(pl, "once", argc, 1);
    struct cut { };
    try
    {
      basic_decoder dc;
      const object_view expr = dc.decode_object(argv);
      pl.make_true(rt, expr, [cont](runtime &rt) {
        cont(rt);
        throw cut {};
      });
    }
    catch (cut) { }
  });
}