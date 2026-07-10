#include "pl/core/interpreter.hpp"


int
main()
{
  {
    interpreter prolog;

    // ---
    prolog << "parent(bob, charly).";
    prolog << "parent(bob, mell).";
    //
    prolog << "(parent(bob, Child), parent(bob, OtherChild))";

    // ---
    prolog << "reverse(Xs, Ys) :- reverse(Xs, nil, Ys, Ys).";
    prolog << "reverse(nil, Ys, Ys, nil).";
    prolog << "reverse(cons(X, Xs), Rs, Ys, cons(_, Bound)) :-"
              "  reverse(Xs, cons(X, Rs), Ys, Bound).         ";
    //
    prolog << "reverse(cons(1, cons(2, nil)), Reverse)";
  }
}
