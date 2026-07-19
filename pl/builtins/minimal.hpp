#pragma once

#include "iso.hpp"


static void
minimal_predicates(interpreter &pl)
{
  pl.add_predicate(pl.make_term(term("true")));
  pl.add_predicate(pl.make_term(term("=", var("X"), var("X"))));


  dictionary vardict;
  const auto var = [&](const auto &name) {
    return nonterminal(vardict[name]);
  };
  // \+ Goal :- Goal -> fail; true.
  pl.add_predicate(
      pl.make_term(term("\\+", var("Goal"))),
      pl.make_term(term("if", var("Goal"), term("fail"), term("true"))));

  vardict.clear();
  // X \= Y -> X = Y -> fail; true.
  pl.add_predicate(pl.make_term(term("\\=", var("X"), var("Y"))),
                   pl.make_term(term("if", term("=", var("X"), var("Y")),
                                     term("fail"), term("true"))));


  iso_type_testing(pl); // Doesn't use parser
  iso_term_comparison(pl); // Doesn't use parser
  iso_term_creation_and_decomposition(pl); // Doesn't use parser

  // once/1
  pl.add_meta_op("once", [&](runtime &rt, int argc, object_iterator argv,
                             const continuation &cont) {
    assert_arity(pl, "once", argc, 1);
    runtime contrt;
    struct cut { };
    try
    {
      basic_decoder dc;
      const object_view expr = dc.decode_object(argv);
      pl.make_true(rt, expr, [&contrt](runtime &rt) { contrt = rt; throw cut {}; });
    }
    catch (cut)
    { TAILCALL cont(contrt); }
  });
}