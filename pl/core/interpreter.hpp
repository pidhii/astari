#pragma once

#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/dictionary.hpp"
#include "pl/parse/object_parser.hpp"

#include <functional>


using continuation = std::function<void(runtime&)>;

using meta_op_handle =
    std::function<void(runtime &, size_t, object_iterator, const continuation &)>;

class interpreter: public runtime {
  enum meta_symbol {
    op_and,
    op_or,
    op_if,
    op_fail,
  };

  public:
  interpreter();
  
  dictionary &
  symbols() noexcept
  { return m_symdict; }

  void
  add_predicate(std::string_view sign, std::string_view body);
  
  void
  add_predicate(std::string_view sign);

  void
  add_meta_op(std::string_view name, const meta_op_handle &handle);

  [[deprecated]] std::pair<object, dictionary>
  parse_expr(std::string_view expr)
  {
    dictionary vardict;
    object_parser p {m_symdict, vardict};
    object obj = p.parse_object(expr);
    return {std::move(obj), std::move(vardict)};
  }

  void
  operator << (std::string input);
  
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

  void
  make_true(object_view expr, const continuation &cont)
  { _make_true(*this, expr, cont); }

  private:
  void
  _make_true(runtime &rt, object_view e, const continuation &cont);

  void
  _make_true__and(runtime &rt, size_t i, object_iterator eit,
                  const continuation &cont);

  void
  _make_true__or(runtime &rt, size_t i, object_iterator eit,
                 const continuation &cont);

  void
  _make_true__if(runtime &rt, object_iterator eit,  const continuation &cont);
  
  void
  _make_true__predicate(runtime &rt, object_view e, const continuation &cont);

  private:
  // arxt::radixhash_node<word_t, object> m_predicates;
  std::unordered_multimap<word_t, std::pair<object, object>> m_predicates;
  std::unordered_map<size_t, meta_op_handle> m_metaops;
  dictionary m_symdict;
}; // class interpreter
