#include "interpreter.hpp"

#include "pl/misc/display.hpp"
#include "pl/parse/syntax_parser.hpp"

#include <iostream>


void
interpreter::operator << (std::string input)
{
  std::istringstream inputstream {std::move(input)};

  std::vector<token> tokens;
  lexer().tokenize(inputstream, std::back_inserter(tokens));

  dictionary vardict;
  syntax_parser stxparser {m_symdict, vardict};
  load_default_grammar(m_symdict, stxparser);
  stxparser.load(tokens.begin(), tokens.end());
  const token result = stxparser.parse();
  switch (result.type)
  {
    case predicate:
    {
      const object_view resobj = std::get<object>(result.val);
      auto it = resobj.begin();
      basic_decoder dc;
      const object_view sign = dc.decode_object(it);
      const object_view body = dc.decode_object(it);
      m_predicates.emplace(sign[0], std::make_pair(sign, body));
      break;
    }

    case statement:
    {
      const object_view sign = std::get<object>(result.val);
      m_predicates.emplace(sign[0], std::make_pair(sign, object_view {}));
      break;
    }

    case obj:
    {
      varnamespace ns;
      const object_view obj = std::get<object>(result.val);
      const object_view expr = adopt(ns, obj);
      bool printed_yes = false;
      make_true(expr, [&](runtime &rt) {
        if (not printed_yes)
        {
          std::cout << "yes" << std::endl;
          printed_yes = true;
        }
        std::cout << "=> ";
        basic_decoder dc;
        bool isfirst = true;
        for (const auto [nsid, rtid] : ns)
        {
          if (not isfirst)
            std::cout << ", ";
          isfirst = false;
          const std::string_view varname = vardict[nsid];
          if (const auto varval = rt.dereference(rtid))
            std::cout << varname << " = "
                      << dump_object(m_symdict, rt.reconstruct(*varval));
          else
            std::cout << varname << " is unbound"
                      << dump_object(m_symdict, rt.reconstruct(*varval));
        }
        std::cout << std::endl;
      });
      break;
    }
  }
}
