#include "interpreter.hpp"

#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/parse/syntax_parser.hpp"

#include <iostream>
#include <fstream>


#define ERROR(fmt, ...)                                                        \
  throw std::runtime_error { std::format(fmt, ##__VA_ARGS__) }


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
  const token stmt = stxparser.parse();
  _interpret(stmt, vardict);
}


void
interpreter::load_file(std::string_view path)
{
  std::ifstream file {path.data(), std::ios_base::binary};
  if (not file)
    ERROR("failed to open file for reading ({})", path);

  const tokstream tokens = lexer().tokenize(file);

  auto it = tokens.tokens.begin();
  while (it != tokens.tokens.end())
  {
    // Find statement boundary
    const auto dot = std::find(it, tokens.tokens.end(), token {'.', "."});
    if (dot == tokens.tokens.end())
      ERROR("unterminated syntax ({})", path);

    // Parse and interpret one statement
    dictionary vardict;
    syntax_parser stxparser {m_symdict, vardict};
    load_default_grammar(m_symdict, stxparser);
    stxparser.load(it, dot + 1);
    const token stmt = stxparser.parse();
    _interpret(stmt, vardict);

    // Shift offset iterator past the interpreted statement
    it = dot + 1;
  }
}


void
interpreter::eval(object_view obj, const dictionary &vardict)
{
  varnamespace ns;
  const object_view expr = adopt(ns, obj);
  make_true(expr, [this, ns, vardict](runtime &rt) {
    std::cout << "yes";
    basic_decoder dc;
    bool isfirst = true;
    for (const auto [nsid, rtid] : ns)
    {
      if (isfirst) std::cout << ": ";
      else         std::cout << ", ";
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
}


void
interpreter::_interpret(const token &stmt, const dictionary &vardict)
{
  switch (stmt.type)
  {
    case predicate:
    {
      const object_view resobj = std::get<object>(stmt.val);
      auto it = resobj.begin();
      basic_decoder dc;
      const object_view sign = dc.decode_object(it);
      const object_view body = dc.decode_object(it);
      m_predicates.emplace(sign[0], std::make_pair(sign, body));
      break;
    }

    case statement:
    {
      const object_view sign = std::get<object>(stmt.val);
      m_predicates.emplace(sign[0], std::make_pair(sign, object_view {}));
      break;
    }

    case directive:
    {
      basic_decoder dc;
      const object_view term = std::get<object>(stmt.val);
      if (not is_term(term[0]))
        ERROR("invalid directive ({})", dump_object(m_symdict, term));
      term_header hdr;
      dc.decode(term[0], hdr);

      // initialization/1
      if (m_symdict[hdr.id] == "initialization" and hdr.arity == 1)
      {
        const object_view goal = dc.decode_object(term.begin() + 1);
        eval(goal, vardict);
        break;
      }

      ERROR("invalid directive ({}/{})", m_symdict[hdr.id], hdr.arity);
    }

    case obj:
    {
      varnamespace ns;
      const object_view obj = std::get<object>(stmt.val);
      eval(obj, vardict);
      break;
    }
  }
}
