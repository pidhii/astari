#pragma once

#include "../../utl/state_saver.hpp"
#include "../dictionary.hpp"
#include "../misc/display.hpp"
#include "../parse/object_parser.hpp"
#include "../parse/syntax_parser.hpp"
#include "match.hpp"
#include "runtime.hpp"


class interpreter: public runtime {
  enum meta_symbol {
    op_and,
    op_or,
    op_if,
    op_fail,
  };

  public:
  interpreter()
  {
    auto require = [this] (std::string_view name, meta_symbol op) {
      if (m_symdict[name] != op)
        throw std::logic_error {"failed to register symbol"};
    };
    require("", op_and);
    require("or", op_or);
    require("if", op_if);
    require("fail", op_fail);
  }

  dictionary &
  symbols() noexcept
  { return m_symdict; }

  void
  add_predicate(std::string_view sign, std::string_view body)
  {
    dictionary vardict;
    object_parser parser {m_symdict, vardict};
    object signobj = parser.parse_object(sign);
    object bodyobj = parser.parse_object(body);
    assert(not signobj.empty());
    assert(not bodyobj.empty());
    assert(word_type(signobj[0]) == word_type::structure);
    // arxt::insert(&m_predicates, signobj, bodyobj);
    m_predicates.emplace(signobj[0], std::make_pair(signobj, bodyobj));
  }

  void
  add_predicate(std::string_view sign)
  {
    dictionary vardict;
    object_parser parser {m_symdict, vardict};
    object signobj = parser.parse_object(sign);
    assert(not signobj.empty());
    assert(word_type(signobj[0]) == word_type::structure);
    // arxt::insert(&m_predicates, signobj, bodyobj);
    m_predicates.emplace(signobj[0], std::make_pair(signobj, object_view()));
  }


  std::pair<object, dictionary>
  parse_expr(std::string_view expr)
  {
    dictionary vardict;
    object_parser p {m_symdict, vardict};
    object obj = p.parse_object(expr);
    return {std::move(obj), std::move(vardict)};
  }

  void
  operator << (std::string input)
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

  using solution = std::unordered_map<std::string_view, object>;

  template <typename Cont>
  void
  make_true(std::string_view exprstr, const Cont &cont)
  {
    dictionary vardict;
    varnamespace ns;

    const object obj = object_parser(m_symdict, vardict).parse_object(exprstr);
    const object_view expr = adopt(ns, obj);

    make_true(expr, [&](runtime &rt) {
      basic_decoder dc;
      solution sol;
      for (const auto [nsid, rtid] : ns)
      {
        const std::string_view varname = vardict[nsid];
        if (const auto varval = rt.dereference(rtid))
          sol[varname] = rt.reconstruct(*varval);
        else
          sol[varname] = {};
      }
      cont(sol);
    });
  }

  using continuation = std::function<void(runtime&)>;

  void
  make_true(object_view expr, const continuation &cont)
  { _make_true(*this, expr, cont); }

  private:
  void
  _make_true(runtime &rt, object_view e, const continuation &cont)
  {
    switch (word_type(e[0]))
    {
      case word_type::structure:
      {
        basic_decoder dc;
        term_header hdr;
        dc.decode(e[0], hdr);
        switch (hdr.id)
        {
          case op_and:
            _make_true__and(rt, hdr.arity, e.begin() + 1, cont);
            return;

          case op_or:
            _make_true__or(rt, hdr.arity, e.begin() + 1, cont);
            return;

          case op_if:
            _make_true__if(rt, e.begin() + 1, cont);
            return;

          case op_fail:
            return;

          default: // predicate
            _make_true__predicate(rt, e, cont);
            return;
        }
      }

      default:
        cont(rt);
        return;
    }
  }

  void
  _make_true__and(runtime &rt, size_t i, object_iterator eit,
                  const continuation &cont)
  {
    basic_decoder dc;
    if (i > 0)
    {
      const object_view e = dc.decode_object(eit);
      _make_true(rt, e, [this, i, eit, cont](runtime &rt) {
        _make_true__and(rt, i - 1, eit, cont);
      });
    }
    else // i == 0
      cont(rt);
  }

  void
  _make_true__or(runtime &rt, size_t i, object_iterator eit,
                 const continuation &cont)
  {
    basic_decoder dc;
    // (opt) no need to lock RT state before the last / single clause
    assert(i >= 1);
    while (i-- > 1)
    { 
      state_saver _ {rt};
      _make_true(rt, dc.decode_object(eit), cont);
    }
    _make_true(rt, dc.decode_object(eit), cont);
  }

  void
  _make_true__if(runtime &rt, object_iterator eit,  const continuation &cont)
  {
    basic_decoder dc;

    word_t *condp = allocator().allocate(1);
    *condp = 0;

    const object_view econd = dc.decode_object(eit);
    const object_view ethen = dc.decode_object(eit);
    const continuation thencont = [this, condp, ethen, cont] (runtime &rt) {
      *condp = 1;
      _make_true(rt, ethen, cont);
    };

    {
      state_saver _ {rt};
      _make_true(rt, econd, thencont);
    }

    if (*condp == 0)
    {
      const object_view eelse = dc.decode_object(eit);
      _make_true(rt, eelse, cont);
    }
  }

  void
  _make_true__predicate(runtime &rt, object_view e, const continuation &cont)
  {
    basic_decoder dc;
    const size_t n = m_predicates.bucket(e[0]);
    for (auto it = m_predicates.begin(n); it != m_predicates.end(n); ++it)
    { // TODO: (opt) no need to lock RT state before the last / single option
      state_saver _ {rt};
      varnamespace ns;
      const auto &[sign, body] = it->second;;
      const object_view predsign = rt.adopt(ns, sign);
      matcher match {rt, dc};
      if (match(e, predsign))
      {
        if (not body.empty())
        {
          const object_view predbody = rt.adopt(ns, body);
          _make_true(rt, predbody, cont);
        }
        else
          cont(rt);
      }
    }
  }

  private:
  // arxt::radixhash_node<word_t, object> m_predicates;
  std::unordered_multimap<word_t, std::pair<object, object>> m_predicates;
  dictionary m_symdict;
}; // class interpreter
