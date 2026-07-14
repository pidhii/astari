
t :-
  test_is(  3 is 1 + 2 ),
  test_is( -1 is 1 - 2 ),
  test_is(  6 is 2 * 3 ),
  test_is(0.5 is 1 / 2 ),
  test_is(  0 is 1 // 2).

t :-
  test(2 =:= 2.0 + 0),
  test(1 =\= 1.1 + 0),
  test(1 < 2.0 + 0),
  test(2 > 1.0 + 0),
  test(1 >= 1.0 + 0),
  test(1 >= 0.0 + 0),
  test(1 =< 1.0 + 0),
  test(1 =< 2.0 + 0).

