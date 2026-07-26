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

  // Exceptions
  iso_throwcatch(pl);

  // All solutions
  iso_all_solutions(pl);

  // Clause Creation and Destruction
#define DEFINE_ASSERT(name, insert, recover)                                   \
  pl.add_meta_op(name, [&](runtime &rt, int argc, object_iterator argv,        \
                           const continuation &cont) {                         \
    assert_arity(pl, name, argc, 1);                                           \
    basic_decoder dc;                                                          \
    const object clause = rt.reconstruct(dc.decode_object(argv));              \
    object_view sign, body;                                                    \
    if (is_term(clause[0], op_penis, 2))                                       \
    {                                                                          \
      object_iterator it = clause.data() + 1;                                  \
      sign = dc.decode_object(it);                                             \
      body = dc.decode_object(it);                                             \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      sign = clause;                                                           \
      body = {};                                                               \
    }                                                                          \
    if (not pl.is_dynamic(sign))                                               \
      pl.raise(term("permission_error", term("modify"),                        \
                    term("static_procedure"), clause));                        \
    insert;                                                                    \
    try                                                                        \
    {                                                                          \
      cont(rt);                                                                \
    }                                                                          \
    catch (...)                                                                \
    {                                                                          \
      recover;                                                                 \
      throw;                                                                   \
    }                                                                          \
    recover;                                                                   \
  });
  DEFINE_ASSERT("asserta", auto save = rt.asserta_dyn(sign, body),
                rt.recover(std::move(save)));
  DEFINE_ASSERT("assertz", auto save = rt.assertz_dyn(sign, body),
                rt.recover(std::move(save)));

  pl.add_meta_op("retract", [&](runtime &rt, int argc, object_iterator argv,
                             const continuation &cont) {
    assert_arity(pl, "retract", argc, 1);
    basic_decoder dc;
    const object_view clause = dc.decode_object(rt.reduce(argv));
    const word_t key = the_word(clause[0]);
    for (auto it = rt.variants_begin(key); it != rt.variants_end(); ++it)
    {
      barrier cp;
      rt.push_choice_point(&cp);
      const object_view itclause = rt.adopt_clause_hp(it);
      if (rt.match(clause, itclause))
      {
        auto save = rt.retract_dyn(key, it);
        try { cont(rt); } catch (...) { rt.recover(std::move(save)); throw; }
        rt.recover(std::move(save));
      }
      if (rt.uwuc(&cp))
        return;
    }
  });

}