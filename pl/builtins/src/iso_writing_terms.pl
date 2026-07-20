
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