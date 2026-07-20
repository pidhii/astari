#include "match.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/core/interpreter.hpp"


bool
matcher::_match(object_iterator lhs, object_iterator rhs, memory &mem)
{
  // Special case when both are nonterminals (includes optimization)
  if (word_type(*lhs) == word_type::nonterminal and
      word_type(*rhs) == word_type::nonterminal)
  {
    nonterminal lhsvar, rhsvar;
    m_decoder.decode(*lhs, lhsvar);
    m_decoder.decode(*rhs, rhsvar);
    // bound + bound => dereference both and match results
    // bound + unbound => bind
    // unbound + unbound => bind
    const auto lhsobj = m_runtime.dereference(lhsvar.id);
    const auto rhsobj = m_runtime.dereference(rhsvar.id);
    if (lhsobj and rhsobj)
    {
      if (not m_memory.emplace(lhs, rhs).second)
        return true;

      auto lhs = *lhsobj;
      auto rhs = *rhsobj;
      [[clang::musttail]] return _match(lhs, rhs, mem);
    }
    else
      return m_runtime.bind(lhsvar.id, rhsvar.id);
  }

  // Special cases with a single nonterminal
  assert(not(word_type(*lhs) == word_type::nonterminal and
             word_type(*rhs) == word_type::nonterminal));
  if (word_type(*lhs) == word_type::nonterminal)
  {
    nonterminal var;
    m_decoder.decode(*lhs, var);
    if (auto lhsobj = m_runtime.dereference(var.id))
    {
      if (not m_memory.emplace(lhs, rhs).second)
        return true;

      auto lhs = *lhsobj;
      [[clang::musttail]] return _match(lhs, rhs, mem);
    }
    else
    {
      const object_view obj = m_decoder.decode_object(rhs);
      assert(not obj.empty());
      m_runtime.assign(var.id, obj.begin());
      return true;
    }
  }
  else if (word_type(*rhs) == word_type::nonterminal)
  {
    // "swap" LHS with RHS and go into the branch above
    [[clang::musttail]] return _match(rhs, lhs, mem);
  }

  // Structural equality
  // TODO: optimize
  if (word_type(*lhs) == word_type(*rhs))
  {
    switch (word_type(*lhs))
    {
      case word_type::blob:
      {
        if (blob_tag(*lhs) != blob_tag(*rhs))
          return false;
        switch (blob_tag(*lhs))
        {
          case blob_tag::string:
            return string(*lhs) == string(*rhs);
          default:
            throw std::runtime_error {"invalid blob tag"};
        }
      }

      case word_type::signed_int_number:
      case word_type::unsigned_int_number:
      case word_type::float_number:
        return *lhs == *rhs;
      
      case word_type::structure:
      {
        term_header lhs_header, rhs_header;
        m_decoder.decode(*lhs++, lhs_header);
        m_decoder.decode(*rhs++, rhs_header);
        if (lhs_header.id != rhs_header.id or
            lhs_header.arity != rhs_header.arity)
          return false;

        if (lhs_header.arity == 0)
          return true;

        size_t n = lhs_header.arity;
        while (n-- > 1)
        {
          if (not _match(lhs, rhs, mem))
            return false;
          basic_decoder dc;
          dc.decode_object(lhs); // call for side-effects
          dc.decode_object(rhs); // call for side-effects
        }
        TAILCALL _match(lhs, rhs, mem);
      }
      
      case word_type::nonterminal:
        // Should not reach here due to earlier handling
        assert(not "unreachable code");
        return false;
    }
  }
  else
    return false;
  
  return false;
}
