#include "iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"


void
iso_all_solutions(interpreter &pl)
{
  pl.add_meta_op("findall", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
    assert_arity(pl, "findall", argc, 3);
    basic_decoder dc;
    const object_view temp = rt.reduce(dc.decode_object(argv));
    const object_view goal = rt.reduce(dc.decode_object(argv));
    const object_view list = rt.reduce(dc.decode_object(argv));

    basic_encoder ec;
    const word_t cons2 = ec.encode(term_header(pl.symbols()["cons"], 2));
    const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));

    object_view result; 

    // Build the list of instances. Perform manual "prelinking" s.t. we can
    // copy the list onto the data heap directly, without using adopt. For this,
    // rename variables in the reconstructed instances s.t. they form a densely
    // packed immediate complement of the current set of variables.
    {
      barrier cp;
      rt.push_choice_point(&cp);

      object l;
      const size_t varbar = rt.n_vars();
      size_t varn = varbar;
      pl.make_true(rt, goal, [&] (runtime &rt) {
        object inst = rt.reconstruct(temp);
        varnamespace ns_local;
        varn = normalize_r(inst, inst.data(), ns_local, varn);
        l += cons2;
        l += inst; 
      });
      l += nil0;

      rt.unwind(&cp);

      // Relocate result onto the data heap
      result = {rt.query()->heap_p, l.size()};
      rt.query()->heap_p = std::copy(l.begin(), l.end(), rt.query()->heap_p);
      // Link new variables
      rt.make_n_vars(varn - varbar);
    }

    if (rt.match(list, result))
      TAILCALL cont(rt);
  });
}