#include "iso.hpp"
#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"


void
iso_all_solutions(interpreter &pl)
{
  pl.add_meta_op("findall", [&](runtime &rt, size_t argc, object_iterator argv,
                                continuation cont) {
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
    // - patch: Preserve all bindings to old variables. New variables are
    //          disentangled as before.
    
    barrier cp;
    rt.push_choice_point(&cp);

    object l;
    const size_t varbar = rt.n_vars();
    size_t varn = varbar;
    rt.exhaust(pl.make_true(rt, goal, continuation::from_lambda([&](CONT_ARGS) {
      object inst = rt.reconstruct(temp);
      varnamespace ns_local;
      for (word_t &w : inst)
      {
        if (is_nonterminal(w))
        {
          nonterminal var;
          dc.decode(w, var);
          if (var.id >= varbar)
          {
            auto [it, isnew] = ns_local.emplace(var.id, varn);
            varn += isnew;
            w = ec.encode(nonterminal(it->second));
          }
        }
      }
      l += cons2;
      l += inst; 
      return DONE;
    })));
    l += nil0;

    rt.unwind(&cp);

    // Relocate result onto the data heap
    result = {rt.query()->heap_p, l.size()};
    rt.query()->heap_p = std::copy(l.begin(), l.end(), rt.query()->heap_p);
    // Link new variables
    rt.make_n_vars(varn - varbar);
  
    if (rt.match(list, result))
      return cont;
    else
      return FAIL;
  });
}