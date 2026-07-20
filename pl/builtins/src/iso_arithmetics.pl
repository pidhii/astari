
Result is Expr :-
  number(Expr)  -> Result = Expr;
  Expr = X + Y  -> Lhs is X, Rhs is Y, sum(Lhs, Rhs, Result);
  Expr = X - Y  -> Lhs is X, Rhs is Y, sub(Lhs, Rhs, Result);
  Expr = X * Y  -> Lhs is X, Rhs is Y, prod(Lhs, Rhs, Result);
  Expr = X / Y  -> Lhs is X, Rhs is Y, fdiv(Lhs, Rhs, Result);
  Expr = X // Y -> Lhs is X, Rhs is Y, idiv(Lhs, Rhs, Result);
  Expr = -X     -> Val is X, sub(0, Val, Result);
  Expr = +X     -> Result is X;
  throw(type_error(evaluable, Expr)).

X =:= Y :- Lhs is X, Rhs is Y, numeq(Lhs, Rhs).
X =\= Y :- Lhs is X, Rhs is Y, numne(Lhs, Rhs).
X < Y   :- Lhs is X, Rhs is Y, numlt(Lhs, Rhs).
X > Y   :- Lhs is X, Rhs is Y, numgt(Lhs, Rhs).
X =< Y  :- Lhs is X, Rhs is Y, numle(Lhs, Rhs).
X >= Y  :- Lhs is X, Rhs is Y, numge(Lhs, Rhs).