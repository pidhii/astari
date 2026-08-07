#include "iso.hpp"


static NOINLINE continuation
_atom_concat(interpreter &pl, runtime &rt, size_t argc, object_iterator argv,
             continuation cont)
{
  assert_arity(pl, "atom_concat", argc, 3);
  basic_decoder dc;
  basic_encoder ec;
  const object_view first = rt.reduce(dc.decode_object(argv));
  const object_view secnd = rt.reduce(dc.decode_object(argv));
  const object_view result = rt.reduce(dc.decode_object(argv));
  if (not is_term_n(first[0], 0))
    raise(pl, term("type_error", term("atom"), first));
  if (not is_term_n(secnd[0], 0))
    raise(pl, term("type_error", term("atom"), secnd));
  const term_header firsthdr = dc.decode_term_header(first[0]);
  const term_header secndhdr = dc.decode_term_header(secnd[0]);
  const std::string_view firstname = pl.symbols()[firsthdr.id];
  const std::string_view secndname = pl.symbols()[secndhdr.id];
  term_header resulthdr;
  resulthdr.id =
      pl.symbols()[std::string(firstname) + std::string(secndname)];
  resulthdr.arity = 0;
  const word_t resultterm = ec.encode(resulthdr);
  const object_view term = rt.adopt_hp({&resultterm, 1});
  if (rt.match(term, result))
    return cont;
  else
    return FAIL;
}

static NOINLINE continuation
_gensym(interpreter &pl, runtime &rt, size_t argc, object_iterator argv,
        continuation cont)
{
  static size_t counter = 0;

  assert_arity(pl, "gensym", argc, 2);
  basic_decoder dc;
  basic_encoder ec;
  const object_view base = rt.reduce(dc.decode_object(argv));
  const object_view result = rt.reduce(dc.decode_object(argv));
  if (not is_term_n(base[0], 0))
    raise(pl, term("type_error", term("atom"), base));
  const term_header basehdr = dc.decode_term_header(base[0]);
  const std::string_view basename = pl.symbols()[basehdr.id];
  term_header resulthdr;
  resulthdr.id =
      pl.symbols()[std::string(basename) + std::to_string(counter++)];
  resulthdr.arity = 0;
  const word_t resultterm = ec.encode(resulthdr);
  const object_view term = rt.adopt_hp({&resultterm, 1});
  if (rt.match(term, result))
    return cont;
  else
    return FAIL;
}


void
iso_atom_processing(interpreter &pl)
{
  // atom_concat/3
  pl.add_meta_op("atom_concat", [&](runtime &rt, size_t argc, object_iterator argv,
             continuation cont) {
    return _atom_concat(pl, rt, argc, argv, std::move(cont));
  });

  // gensym/2
  pl.add_meta_op("gensym", [&](runtime &rt, size_t argc, object_iterator argv,
             continuation cont) {
    return _gensym(pl, rt, argc, argv, std::move(cont));
  });
}