
when(Cond, Goal) :-
  Cond -> Goal; true.

infonl(What) :-
  verbose -> write(What), nl;
  true.

info(What) :-
  verbose -> write(What);
  true.


printlist(_, []) :- !.
printlist(Lvl, [X]) :- !,
  printone(Lvl, X), nl.
printlist(Lvl, [H|T]) :-
  printone(Lvl, H), write(","), nl,
  printlist(Lvl, T).

printlist(L) :-
  printlist(0, L).


printone(Lvl, asserta(CHead :- CBody)) :- !,
  indent(Lvl), write("asserta("), nl,
  indent(Lvl + 2), write(CHead), write(" :- "),
  (
    CBody =.. [','|BCs] -> nl, printlist(Lvl + 4, BCs);
    indent(Lvl + 4), write(CBody), nl
  ),
  indent(Lvl), write(")").

printone(Lvl, asserta(instance(_, _))) :- !,
  indent(Lvl), write("asserta(instance(...))").

printone(Lvl, asserta(Clause)) :- !,
  indent(Lvl), write(asserta(Clause)).

printone(Lvl, X) :-
  indent(Lvl), write(X).

indent(Lvl) :-
  Lvl =:= 0 -> true;
  write(" "), indent(Lvl - 1).
  

ensure_asserted(ClauseHead) :-
  clause(ClauseHead, true), !;
  asserta(ClauseHead).

ensure_asserted(ClauseHead, ClauseBody) :-
  clause(ClauseHead, ClauseBody), !;
  asserta(ClauseHead :- ClauseBody).


must(Goal, What) :-
  Goal, !;
  throw(transformation_error(What)).

must(G, E, A, Z) :-
  must(call(G, A, Z), E).


debug(Id, Goal) :-
  write("["), write(Id), write("] try "), write(Goal), nl,
  Goal,
  write("["), write(Id), write("] ok  "), write(Goal), nl.

debug(Goal) :-
  gensym('', Id), debug(Id, Goal).

debug(Goal, A, Z) :-
  debug(call(Goal, A, Z)).
