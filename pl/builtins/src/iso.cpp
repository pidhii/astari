#include "iso.hpp"


iso::iso(interpreter &pl, unsigned libs)
: io {pl}
{
  if (libs & basic)
    iso_basic(pl);
  if (libs & type_testing)
    iso_type_testing(pl);
  if (libs & term_comparison)
    iso_term_comparison(pl);
  if (libs & writing_terms)
    iso_writing_terms(io, pl);
  if (libs & writing_characters)
    iso_writing_characters(io, pl);
  if (libs & arithmetics)
    iso_arithmetics(pl);
  if (libs & term_creation_and_decomposition)
    iso_term_creation_and_decomposition(pl);
  if (libs & throwcatch)
    iso_throwcatch(pl);
  if (libs & all_solutions)
    iso_all_solutions(pl);
  if (libs & clause_creation_and_destruction)
    iso_clause_creation_and_destruction(pl);
}