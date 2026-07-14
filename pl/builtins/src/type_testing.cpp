#include "iso.hpp"


void
iso_type_testing(interpreter &pl)
{
  // var/1
  pl.add_meta_op("var", [&](runtime &rt, int argc, object_iterator argv,
                            const continuation &cont) {
    assert_arity(pl, "var", argc, 1);
    if (not is_nonterminal(argv[0]))
      return;
    nonterminal var;
    basic_decoder().decode(argv[0], var);
    if (rt.dereference(var.id))
      return;
    cont(rt);
  });

  // atom/1
  pl.add_meta_op("atom", [&](runtime &rt, int argc, object_iterator argv,
                              const continuation &cont) {
    assert_arity(pl, "atom", argc, 1);
    basic_decoder dc;
    const object x = rt.reconstruct(dc.decode_object(argv));
    if (is_term(x[0]) and dc.decode_term_header(x[0]).arity == 0)
      cont(rt);
  });

  // integer/1
  pl.add_meta_op("integer", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
    assert_arity(pl, "integer", argc, 1);
    basic_decoder dc;
    const object x = rt.reconstruct(dc.decode_object(argv));
    switch (word_type(x[0]))
    {
      case word_type::signed_int_number: // fallthrough
      case word_type::unsigned_int_number: cont(rt); break;
      default: return;
    }
  });

  // float/1
  pl.add_meta_op("float", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
    assert_arity(pl, "float", argc, 1);
    basic_decoder dc;
    const object x = rt.reconstruct(dc.decode_object(argv));
    switch (word_type(x[0]))
    {
      case word_type::float_number: cont(rt); break;
      default: return;
    }
  });

  // string/1
  pl.add_meta_op("string", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
    assert_arity(pl, "string", argc, 1);
    basic_decoder dc;
    const object x = rt.reconstruct(dc.decode_object(argv));
    if (word_type(x[0]) == word_type::blob and
        blob_tag(x[0]) == blob_tag::string)
      cont(rt);
  });

  pl << R"(
    nonvar(X) :-
      var(X) -> fail; true.

    number(X) :-
      integer(X); float(X).

    atomic(X) :-
      atom(X); number(X); string(X).

    compound(X) :-
      atomic(X) -> fail; nonvar(X).
  )";
}