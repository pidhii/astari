
t :-
  test(a == a),
  test(a(b) == a(b)),
  test(a \== b),
  test(a(b) \== a(c)),
  test(a @< b; b @< a),
  (
    a @< b -> L = a, R = b;
    L = b, R = a
  ),
  test(a(L) @< a(R)),
  test(a(R) @> a(L)),
  test(a(L) @=< a(R)),
  test(a(R) @>= a(L)),
  test(a(a) @=< a(a)),
  test(a(a) @>= a(a)),
  test(1 == 1),
  test(1.0 == 1.0),
  test(1 \== 1.0),
  test("a" == "a"),
  test("a" @< "b"),
  test(_ \== _),
  test((X = Y, X == Y)).

