#pragma once

#include "iso.hpp"

#include "pl/misc/term_utils.hpp"


static void
minimal_predicates(interpreter &pl)
{
  pl.assertz(make_term(pl, term("true")));
  pl.assertz(make_term(pl, term("false")), make_term(pl, term("fail")));
  pl.assertz(make_term(pl, term("=", var("X"), var("X"))));


  dictionary vardict;
  const auto var = [&](const auto &name) { return nonterminal(vardict[name]); };
  // \+ Goal :- Goal -> fail; true.
  pl.assertz(make_term(pl, term("\\+", var("Goal"))),
             make_term(pl, term("if", var("Goal"), term("fail"), term("true"))));

  vardict.clear();
  // X \= Y -> X = Y -> fail; true.
  pl.assertz(make_term(pl, term("\\=", var("X"), var("Y"))),
             make_term(pl, term("if", term("=", var("X"), var("Y")),
                                term("fail"), term("true"))));

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

  iso_type_testing(pl);                    // Doesn't use parser
  iso_term_comparison(pl);                 // Doesn't use parser
  iso_term_creation_and_decomposition(pl); // Doesn't use parser
  iso_throwcatch(pl);                      // Doesn't use parser
  iso_clause_creation_and_destruction(pl); // Doesn't use parser
  iso_arithmetics(pl);                     // Doesn't use parser
}