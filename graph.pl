:- dynamic(yield/3).

graph(G) :-
  asserta(yield(EdgeLen, Node, Heuristic) :- '$graph:yield'(EdgeLen, Node, Heuristic)),
  '$graph:graph_entry',
  call(G),
  retract(yield(_, _, _) :- _).
