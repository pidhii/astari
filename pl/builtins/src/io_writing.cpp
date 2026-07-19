#include "pl/builtins/iso.hpp"
#include "pl/coding/basic_decoder.hpp"
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
  pl.add_meta_op("write_term", [&](runtime &rt, int argc,
                                     object_iterator argv,
                                     const continuation &cont) {
    assert_arity(pl, "write_term", argc, 2, 3);

    basic_decoder dc;
    const object s = argc == 2 ? object {io.current_output}
                               : rt.reconstruct(dc.decode_object(argv));
    const object term = rt.reconstruct(dc.decode_object(argv));
    [[maybe_unused]] const object opts = rt.reconstruct(dc.decode_object(argv));
    dump_object(io.symbols, term, io.get_output(s));
    TAILCALL cont(rt);
  });

  pl << R"(
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


