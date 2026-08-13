%% source: https://www.swi-prolog.org/pldoc/doc/_SWI_/library/lists.pl

length(L, N) :-
  '$length3'(L, N, 0).

'$length3'([], N, N).
'$length3'(X, N, N0) :-
  var(X), !,
  '$genlist3'(X, N, N0).
'$length3'([_|List], N, N0) :-
  N1 is N0 + 1,
  '$length3'(List, N, N1).
'$genlist3'(X, N, N0) :-
  N =:= N0 -> X = [];
  N1 is N0 + 1,
  X = [_|List],
  '$genlist3'(List, N, N1).
  

% member/2
member(El, [H|T]) :-
  '$member3'(T, El, H).

'$member3'(_, El, El).
'$member3'([H|T], El, _) :-
  '$member3'(T, El, H).


% append/3
append([], L, L).
append([H|T], L, [H|R]) :-
  append(T, L, R).


% append/2
append(ListOfLists, List) :-
  '$append2'(ListOfLists, List).

'$append2'([], []).
'$append2'([L|Ls], As) :-
  append(L, Ws, As),
  '$append2'(Ls, Ws).


% prefix/2
prefix([], _).
prefix([E|T0], [E|T]) :-
  prefix(T0, T).


% select/3
select(X, [Head|Tail], Rest) :-
  '$select3'(Tail, Head, X, Rest).

'$select3'(Tail, Head, Head, Tail).
'$select3'([Head2|Tail], Head, X, [Head|Rest]) :-
  '$select3'(Tail, Head2, X, Rest).


% selectchk/3
selectchk(Elem, List, Rest) :-
  select(Elem, List, Rest0), !, Rest = Rest0.


% select/4
select(X, XList, Y, YList) :-
  '$select4'(XList, X, Y, YList).

'$select4'([X|List], X, Y, [Y|List]).
'$select4'([X0|XList], X, Y, [X0|YList]) :-
  '$select4'(XList, X, Y, YList).


% selectchk/4
selectchk(X, XList, Y, YList) :-
  select(X, XList, Y, YList), !.


% nextto/3
nextto(X, Y, [X,Y|_]).
nextto(X, Y, [_|Zs]) :-
  nextto(X, Y, Zs).

% permutation/2
permutation([], []).
permutation(List, [First|Perm]) :-
    select(First, List, Rest),
    permutation(Rest, Perm).
