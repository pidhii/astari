#include "iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/core/interpreter.hpp"


void
iso_basic(interpreter &pl)
{
  pl.load_objfile(PLO_PATH_iso_basic);

  pl.add_meta_op("call", [&](runtime &rt, size_t argc, object_iterator argv,
                             continuation cont) {
    assert(argc >= 1);
    basic_decoder dc;
    basic_encoder ec;

    // Merge all the arguments into one clause
    word_t *p = rt.query()->heap_p;
    for (size_t i = 0; i < argc; ++i)
    {
      const object_view arg = rt.reduce(dc.decode_object(argv));
      rt.query()->heap_p = std::copy(arg.begin(), arg.end(), rt.query()->heap_p);
    }
    if (not is_term(*p))
      raise(pl, term("type_error", term("term"), dc.decode_object(p)));

    // Fix the key of the clause to account for appended arguments
    term_header hdr;
    dc.decode(*p, hdr);
    const size_t nargs = hdr.arity + argc - 1;
    *p = ec.encode(term_header(hdr.id, nargs));
    const object_view goal {p, rt.query()->heap_p};

    // Call the merged goal clause in encapsulated scope (so that it can't cut
    // the outer scope)
    barrier cp;
    // rt.push_choice_point(&cp);
    // cont = pl.make_true(rt, goal, cont);
    // rt.pop_choice_point(&cp);
    // return cont;
    rt.push_choice_point(&cp);
    rt.exhaust(pl.make_true(rt, goal, cont));
    rt.pop_choice_point(&cp);
    return FAIL;
  });

  // once/1
  pl.add_meta_op("once", [&](runtime &rt, size_t argc, object_iterator argv,
                             continuation cont) {
    assert_arity(pl, "once", argc, 1);
    basic_decoder dc;
    const object_view expr = dc.decode_object(argv);
    barrier cp;
    rt.push_choice_point(&cp);
    cont = pl.make_true(rt, expr, continuation::from_lambda([cc=std::move(cont), &cp](CONT_ARGS) mutable {
      rt.cut(&cp);
      return std::move(cc).reinterpret<void()>();
    }));
    if (rt.driveuc(&cp, cont))
      return cont;
    rt.pop_choice_point(&cp);
    return FAIL;
  });

  ////////////////////////////////////////////////////////////////////////////
  // halt/0, halt/1
  //
  pl.add_meta_op("halt", [&](runtime &rt, size_t argc, object_iterator argv,
                             continuation cont) -> continuation {
    assert_arity(pl, "halt", argc, 0, 1);
    if (argc == 1)
      number(pl, rt.reduce(argv), std::exit);
    else
      std::exit(0);
    std::terminate();
  });
}
