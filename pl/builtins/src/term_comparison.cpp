#include "iso.hpp"

#include "pl/core/compare.hpp"
#include "pl/obj/object.hpp"


void
iso_term_comparison(interpreter &pl)
{
#define DEFINE_CMP(name, op)                                                   \
  pl.add_meta_op(name, [&](runtime &rt, int argc, object_iterator argv,        \
                           const continuation &cont) {                         \
    assert_arity(name, argc, 2);                                               \
    basic_decoder dc;                                                          \
    const object_iterator lhs = argv;                                          \
    dc.decode_object(argv);                                                    \
    const object_iterator rhs = argv;                                          \
    if (compare(rt, lhs, rhs) op 0)                                            \
      cont(rt);                                                                \
  });

  DEFINE_CMP("@=<", <=)
  DEFINE_CMP("@<", <)
  DEFINE_CMP("@>=", >=)
  DEFINE_CMP("@>", >)
  DEFINE_CMP("==", ==)
  DEFINE_CMP("\\==", !=)
}