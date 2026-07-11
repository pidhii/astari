#include "pl/core/interpreter.hpp"
#include "pl/misc/display.hpp"

#include <iostream>


int
main()
{
  // Test interpreter basics
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

  // Test meta operators
  {
    interpreter prolog;

    prolog.add_meta_op("print", [&prolog](runtime &rt, size_t argc,
                                          object_iterator argv,
                                          const continuation &cont) {
      while (argc--)
      {
        const object_view x = basic_decoder().decode_object(argv);
        dump_object(prolog.symbols(), rt.reconstruct(x), std::cout);
        std::cout << " ";
      }
      std::cout << std::endl;
    });

    // ---
    prolog << "print(helloWorld, 1, cons(2, 3))";
  }

  // Test state saving
  {
    interpreter prolog;

    prolog.add_meta_op("print", [&prolog](runtime &rt, size_t argc,
                                          object_iterator argv,
                                          const continuation &cont) {
      while (argc--)
      {
        const object_view x = basic_decoder().decode_object(argv);
        dump_object(prolog.symbols(), rt.reconstruct(x), std::cout);
        std::cout << " ";
      }
      std::cout << std::endl;
      cont(rt);
    });

    runtime save_rt;
    continuation save_cont;
    prolog.add_meta_op("save",
                       [&save_rt, &save_cont](runtime &rt, size_t argc,
                                              object_iterator argv,
                                              const continuation &cont) {
                         std::cout << "SAVE" << std::endl;
                         save_rt = rt;
                         save_cont = cont;
                       });

    // ---
    prolog << R"(
      test() :-
        print(beforeSave),
        save(),
        print(afterSave).
    )";

    std::cout << "START" << std::endl;
    prolog << "test";
    std::cout << "CONTINUE" << std::endl;
    save_cont(save_rt);
    std::cout << "FINISH" << std::endl;
  };
}
