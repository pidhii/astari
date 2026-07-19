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
void
iso_writing_terms(iso_io &io, interpreter &pl)
{
  // write_term/2, write_term/3
  pl.add_meta_op("write_term__", [&](runtime &rt, int argc,
                                     object_iterator argv,
                                     const continuation &cont) {
    assert_arity(pl, "write_term__", argc, 5);

    basic_decoder dc;
    basic_encoder ec;
    const object s = rt.reconstruct(dc.decode_object(argv));
    const object term = rt.reconstruct(dc.decode_object(argv));
    const object quoted = rt.reconstruct(dc.decode_object(argv));
    const object ignore_ops = rt.reconstruct(dc.decode_object(argv));
    const object numbervars = rt.reconstruct(dc.decode_object(argv));
    const word_t true0 = ec.encode(term_header(pl.symbols()["true"], 0));
    dump_object(io.symbols, term, io.get_output(s), quoted[0] == true0,
                ignore_ops[0] == true0, numbervars[0] == true0);
    TAILCALL cont(rt);
  });

  // TODO: remove custom member/2 after adding a list library
  pl << R"(
    iso_io_member__(X, [X|_]).
    iso_io_member__(X, [_|T]) :-
      iso_io_member__(X, T).

    write_term(S, Term, Options) :-
      (iso_io_member__(quoted(Quoted), Options) -> true; Quoted = false),
      (iso_io_member__(ignore_ops(IgnoreOps), Options) -> true; IgnoreOps = false),
      (iso_io_member__(numbervars(Numbervars), Options) -> true; Numbervars = false),
      write_term__(S, Term, Quoted, IgnoreOps, Numbervars).

    write_term(Term, Options) :-
      current_output(S),
      write_term(S, Term, Options).

    write(Term) :-
      current_output(S),
      write_term(S, Term, [numbervars(true)]).

    writeq(Term) :-
      current_output(S),
      write_term(S, Term, [quoted(true), numbervars(true)]). 

    writeq(S,Term) :-
      write_term(S, Term, [quoted(true), numbervars(true)]). 

    write_canonical(T) :-
      current_output(S),
      write_term(S, Term, [quoted(true), ignore_ops(true)]). 
  
    write_canonical(S,T) :-
      write_term(S, Term, [quoted(true), ignore_ops(true)]). 
  )";
}



//////////////////////////////////////////////////////////////////////////////
//                       Character Output
//
// see: https://www.deransart.fr/prolog/bips.html
//
void
iso_writing_characters(iso_io &io, interpreter &pl)
{
  // put_code/1, put_code/2
  pl.add_meta_op("put_code", [&](runtime &rt, int argc, object_iterator argv,
                                 const continuation &cont) {
    assert_arity(pl, "put_code", argc, 1, 2);
    basic_decoder dc;
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
        TAILCALL cont(rt);
      }
      case word_type::nonterminal:
        pl.raise(term("instantiation_error"));

      default:
        pl.raise(term("representation_error", term("character")));
    }
  });
  pl << R"(
    put_char(S, Char) :-
      char_code(Char, Code),
      put_code(S, Code).

    put_char(Code) :-
      current_output(S),
      put_char(S, Char). 

    nl(S) :-
      put_code(S, 10).

    nl :-
      current_output(S),
      nl(S).
  )";
};


