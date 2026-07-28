#include "iso.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/misc/display.hpp"
#include "pl/misc/term_utils.hpp"


iso_io::iso_io(interpreter &pl)
: symbols {pl.symbols()},
  stdout_term {make_term(pl, term("stdout"))},
  stderr_term {make_term(pl, term("stderr"))},
  stdin_term {make_term(pl, term("stdin"))},
  current_output {stdout_term},
  current_input {stdin_term}
{
  // current_output/1
  pl.add_meta_op("current_output", [&](runtime &rt, size_t argc,
                                       object_iterator argv,
                                       const continuation &cont) {
    assert_arity(pl, "current_output", argc, 1);
    basic_decoder dc;
    const object_view x = dc.decode_object(argv);
    if (rt.match(x, current_output))
      TAILCALL cont(rt);
  });
}


std::ostream &
iso_io::get_output(object_view s)
{
  if (is_term(s))
  {
    if (the_word(s[0]) == the_word(stdout_term[0]))
      return std::cout;
    else if (the_word(s[0]) == the_word(stderr_term[0]))
      return std::cerr;
    else if (auto it = ostreams.find(the_word(s[0])); it != ostreams.end())
      return *it->second.second;
    else
      throw std::runtime_error {
          std::format("no such output stream ({})", dump_object(symbols, s))};
  }
  else
    throw std::runtime_error {
        std::format("invalid output stream ({})", dump_object(symbols, s))};
}
