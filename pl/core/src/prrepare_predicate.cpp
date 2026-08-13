#include "predicate_entry.hpp"
#include "runtime.hpp"
#include "match.hpp"

#include "pl/coding/basic_decoder.hpp"

#include <cassert>


using _occurances = std::unordered_map<size_t, size_t>;

static void
_count_occurances(object_view obj, _occurances &occurs)
{
  for (word_t w : obj)
  {
    if (is_nonterminal(w))
      occurs[w] += 1;
  }
}

// static void
// _mark_wildcards(object &obj, const _occurances &occurs)
// {
//   for (word_t &w : obj)
//   {
//     if (is_nonterminal(w))
//     {
//       assert(occurs.at(w) > 0);
//       if (occurs.at(w) == 1)
//         w = add_magic(w, wildcard);
//     }
//   }
// }

predicate_entry
prepare_predicate(object_view signobj, object_view bodyobj)
{
  basic_decoder dc;

  assert(not signobj.empty());
  if (not is_term(signobj[0]))
    throw std::runtime_error {"invalid predicate signature"};

  object sign {signobj};
  object body {bodyobj};

  // Normalize and save variable counts for fast adopt
  varnamespace ns;
  size_t base = 0;
  base = normalize_r(sign, sign.data(), ns, base);
  base = normalize_r(body, body.data(), ns, base);

  // Inject optimization hints
  _occurances occurs;
  _count_occurances(sign, occurs);
  _count_occurances(body, occurs);
  // _mark_wildcards(sign, occurs);
  // _mark_wildcards(body, occurs);

  dc.decode_object(sign.data()); // call for side-effects
  if (not body.empty())
    dc.decode_object(body.data()); // call for side-effects

  return {sign, body, base};
}

