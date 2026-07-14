
t :-
  test(var(_)),
  test(nonvar(a(_, _))),
  test((X = 1, nonvar(X))),
  test(atom(a)),
  test(atomic(a)),
  test(atomic(1)),
  test(atomic(1.0)),
  test(atomic("a")),
  test(compound(a(b))),
  test(string("a")),
  test(integer(1)),
  test(float(1.0)),
  test(number(1)),
  test(number(1.0)).

