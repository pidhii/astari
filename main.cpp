#include "pl/core/interpreter.hpp"
#include "pl/misc/display.hpp"

#include <iostream>


int
main()
{
  {
    interpreter prolog;

    prolog.add_meta_op("print", [&prolog](runtime &rt, size_t argc,
                                          object_iterator argv,
                                          const continuation &cont) {
      while (argc--)
      {
        const object_view x = basic_decoder().decode_object(argv);
        std::cout << dump_object(prolog.symbols(), rt.reconstruct(x)) << " ";
      }
      std::cout << std::endl;
    });


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

    // ---
    prolog << "print(helloWorld, 1, cons(2, 3))";
  }
}
