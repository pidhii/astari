#include "pl/builtins/iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/misc/display.hpp"


//////////////////////////////////////////////////////////////////////////////
//                         Writing terms
//
// see: https://www.deransart.fr/prolog/bips.html
//
// notes
// - write_term/2 as in the reference above is ambiguous
//   a) write_term/2(S, Term)        ✗
//   b) write_term/2(Term, Options)  ✓
//

static NOINLINE continuation
_write_term(iso_io &io, runtime &rt, size_t argc, object_iterator argv,
            continuation cont)
{
  assert_arity(io.pl, "write_term__", argc, 5);
  static basic_decoder dc;
  static basic_encoder ec;

  const object s = rt.reconstruct(dc.decode_object(argv));
  const object term = rt.reconstruct(dc.decode_object(argv));
  const object quoted = rt.reconstruct(dc.decode_object(argv));
  const object ignore_ops = rt.reconstruct(dc.decode_object(argv));
  const object numbervars = rt.reconstruct(dc.decode_object(argv));
  const word_t true0 = ec.encode(term_header(io.symbols["true"], 0));
  dump_object(io.symbols, term, io.get_output(s),
              the_word(quoted[0]) == true0, the_word(ignore_ops[0]) == true0,
              the_word(numbervars[0]) == true0);
  io.get_output(s).flush();

  return cont;
}

void
iso_writing_terms(iso_io &io, interpreter &pl)
{
  // write_term__/5
  pl.add_meta_op("write_term__", [&] NOINLINE (runtime &rt, size_t argc,
                                               object_iterator argv,
                                               continuation cont) {
    return _write_term(io, rt, argc, argv, std::move(cont));
  });

  pl.load_objfile(PLO_PATH_iso_writing_terms);
}



//////////////////////////////////////////////////////////////////////////////
//                       Character Output
//
// see: https://www.deransart.fr/prolog/bips.html
//
static NOINLINE continuation
_put_code(iso_io &io, runtime &rt, size_t argc, object_iterator argv,
            continuation cont)
{
  assert_arity(io.pl, "put_code", argc, 1, 2);
  basic_decoder dc;
  {
    const object s = argc == 1 ? object {io.current_output}
                                : rt.reconstruct(dc.decode_object(argv));
    const object c = rt.reconstruct(dc.decode_object(argv));
    switch (word_type(c[0]))
    {
      case word_type::signed_int_number:
      {
        int cv;
        dc.decode(c[0], cv);
        io.get_output(s) << char(cv);
        break;
      }
      case word_type::nonterminal:
        raise(io.pl, term("instantiation_error"));

      default:
        raise(io.pl, term("representation_error", term("character")));
    }
  }
  return cont;
}

void
iso_writing_characters(iso_io &io, interpreter &pl)
{
  // put_code/1, put_code/2
  pl.add_meta_op("put_code", [&] NOINLINE (runtime &rt, size_t argc,
                                           object_iterator argv,
                                           continuation cont) {
    return _put_code(io, rt, argc, argv, std::move(cont));
  });

  pl.load_objfile(PLO_PATH_iso_writing_characters);
};


