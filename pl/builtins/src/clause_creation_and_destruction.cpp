#include "iso.hpp"

#include "utl/state_saver.hpp"


void
iso_clause_creation_and_destruction(interpreter &pl)
{
#define DEFINE_ASSERT(name, insert, recover)                                   \
  pl.add_meta_op(name, [&](runtime &rt, size_t argc, object_iterator argv,     \
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
    if (not pl.is_dynamic(sign[0]))                                            \
      raise(pl, term("permission_error", term("modify"),                       \
                     term("static_procedure"), clause));                       \
    insert;                                                                    \
    try                                                                        \
    {                                                                          \
      cont(rt, 0, 0, 0, 0);                                                    \
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

  pl.add_meta_op("retract", [&](runtime &rt, size_t argc, object_iterator argv,
                                continuation &cont) {
    assert_arity(pl, "retract", argc, 1);
    basic_decoder dc;
    const object_view clause = dc.decode_object(rt.reduce(argv));
    const word_t key = is_term(clause[0], op_penis, 2) ? the_word(clause[1])
                                                       : the_word(clause[0]);
    if (not pl.is_dynamic(key))
      raise(pl, term("permission_error", term("modify"),
                     term("static_procedure"), clause));
    for (auto it = rt.variants_begin(key); it != rt.variants_end(); ++it)
    {
      barrier cp;
      rt.push_choice_point(&cp);
      state_saver _ {cont};
      const object_view itclause = rt.adopt_clause_hp(it);
      if (rt.match(clause, itclause))
      {
        auto save = rt.retract_dyn(key, it);
        try { cont(rt, 0, 0, 0, 0); }
        catch (...) { rt.recover(std::move(save)); throw; }
        rt.recover(std::move(save));
      }
      if (rt.uwuc(&cp))
        return;
    }
  });

  // TODO:
  // - handle static predicates
  // - handle nonterminal arguments
  pl.add_meta_op("clause", [&](runtime &rt, size_t argc, object_iterator argv,
                               continuation &cont) {
    assert_arity(pl, "clause", argc, 2);
    basic_decoder dc;
    const object_view head = rt.reduce(dc.decode_object(argv));
    const object_view body = rt.reduce(dc.decode_object(argv));
    const object_view true0 = rt.adopt_hp(make_term(pl, term("true")));
    if (is_term(head[0]))
    {
      const word_t key = the_word(head[0]);
      if (pl.is_dynamic(key))
      {
        for (auto it = rt.variants_begin(key); it != rt.variants_end(); ++it)
        {
          barrier cp;
          rt.push_choice_point(&cp);
          state_saver _ {cont};

          const object_view ithead = rt.adopt_hp(rt.variant_sign(it));
          const object_view itbody = rt.variant_body(it).empty()
                                         ? true0
                                         : rt.adopt_hp(rt.variant_body(it));
          if (rt.match(head, ithead) and rt.match(body, itbody))
            cont(rt, 0, 0, 0, 0);
          if (rt.uwuc(&cp))
            return;
        }
      }
      else
        raise(pl, term("unimplemented", term("clause", rt.reconstruct(head),
                                             rt.reconstruct(body))));
    }
    else
      raise(pl, term("unimplemented", term("clause", rt.reconstruct(head),
                                           rt.reconstruct(body))));
  });
}