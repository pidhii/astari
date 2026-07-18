#pragma once

#include "pl/core/interpreter.hpp"
#include "pl/parse/lexer.hpp"


class lib_parsing {
  public:
  lib_parsing(interpreter &pl)
  {
    // tokens/2
    pl.add_meta_op("tokens", [&pl](runtime &rt, int argc, object_iterator argv,
                                   const continuation &cont) {
      assert(argc == 2);
      basic_decoder dc;
      basic_encoder ec;
      const object_view string = rt.reduce(dc.decode_object(argv));
      const object_view tokens = rt.reduce(dc.decode_object(argv));
      assert(is_blob(string[0]));

      dictionary vardict;
      const object list = _tokenize(pl, vardict, ::string(string[0]));
      const object_view pobj = rt.adopt(list);
      if (rt.match(pobj, tokens))
        cont(rt);
      else
        rt.unallocate(pobj);
    });
  }

  private:
  static object
  _tokenize(interpreter &pl, dictionary &vardict, std::string_view text)
  {
    std::istringstream stream {std::string(text)};
    const auto [elts, nelts] = lexer().tokens(pl, vardict, stream);
    const object result = make_list(pl, nelts, object_view(elts).begin());
    // std::clog << "tokenlist: " << pl.dump(result) << std::endl;
    return result;
  }
};