
maplist(_, []).
maplist(Term, [H|T]) :-
  call(Term, H),
  maplist(Term, T).

maplist(_, [], []).
maplist(Term, [H1|T1], [H2|T2]) :-
  call(Term, H1, H2),
  maplist(Term, T1, T2).

maplist(_, [], [], []).
maplist(Term, [H1|T1], [H2|T2], [H3|T3]) :-
  call(Term, H1, H2, H3),
  maplist(Term, T1, T2, T3).

maplist(_, [], [], [], []).
maplist(Term, [H1|T1], [H2|T2], [H3|T3], [H4|T4]) :-
  call(Term, H1, H2, H3, H4),
  maplist(Term, T1, T2, T3, T4).

maplist(_, [], [], [], [], []).
maplist(Term, [H1|T1], [H2|T2], [H3|T3], [H4|T4], [H5|T5]) :-
  call(Term, H1, H2, H3, H4, H5),
  maplist(Term, T1, T2, T3, T4, T5).


foldl(_, [], Acc, Acc).
foldl(Goal, [X|Xs], Acc, Fold) :-
  call(Goal, X, Acc, XFold),
  foldl(Goal, Xs, XFold, Fold).


partition(Pred, List, Included, Excluded) :-
  partition_(List, Pred, Included, Excluded).

partition_([], _, [], []).
partition_([H|T], Pred, Incl, Excl) :-
  call(Pred, H) -> Incl = [H|I], partition_(T, Pred, I, Excl);
                   Excl = [H|E], partition_(T, Pred, Incl, E).
