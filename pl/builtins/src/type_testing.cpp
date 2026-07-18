#include "iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"


void
iso_type_testing(interpreter &pl)
{
  // var/1
  pl.add_meta_op("var", [&](runtime &rt, int argc, object_iterator argv,
                            const continuation &cont) {
    assert_arity(pl, "var", argc, 1);
    basic_decoder dc;
    object_view x = rt.reduce(dc.decode_object(argv));
    if (is_nonterminal(x[0]))
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

  dictionary v;
  auto var = [&] (std::string_view name) { return nonterminal(v[name]); };

  // nonvar/1
  //   nonvar(X) :- if(var(X), fail, true).
  pl.add_predicate(pl.make_term(term("nonvar", var("X"))),
                   pl.make_term(term("if", term("var", var("X")), term("fail"),
                                     term("true"))));

  // number/1
  //   number(X) :- integer(X); float(X).
  pl.add_predicate(pl.make_term(term("number", var("X"))),
                   pl.make_term(term(";", term("integer", var("X")),
                                     term("float", var("X")))));

  // atomic/1
  //   atomic(X) :- atom(X); number(X); string(X).
  pl.add_predicate(pl.make_term(term("atomic", var("X"))),
                   pl.make_term(term(";", term("atom", var("X")),
                                     term("number", var("X")),
                                     term("string", var("X")))));

  // compound/1
  //   compound(X) :- if(atomic(X), fail, nonvar(X)).
  pl.add_predicate(pl.make_term(term("compound", var("X"))),
                   pl.make_term(term("if", term("atomic", var("X")),
                                     term("fail"), term("nonvar", var("X")))));
}
