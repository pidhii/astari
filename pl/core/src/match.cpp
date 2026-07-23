#include "match.hpp"
#include "interpreter.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/obj/object.hpp"


bool
shallow_match(object_iterator lhs, object_iterator rhs, size_t n)
{
  basic_decoder dc;

  while (n--)
  {
    switch (((*lhs & 0b11) << 2) | (*rhs & 0b11))
    {
      case 0b0000: // blob, blob
        lhs++;
        rhs++;
        break; // blobs may match

      case 0b0101: // term, term
        if (the_word(*lhs) != the_word(*rhs))
          return false; // can't match
        n += dc.decode_term_header(*lhs).arity; // proceeed to shallow match the fields
        lhs++;
        rhs++;
        break;

      case 0b1010: // number, number
        if (*lhs++ != *rhs++) // trivially comparable
          return false;
        break;

      case 0b0001: // blob, term
      case 0b0100: // term, blob
      case 0b0010: // blob, number
      case 0b1000: // number, blob
      case 0b0110: // term, number
      case 0b1001: // number, term
        return false; // can't match

      case 0b1100: // var, blob
      case 0b1110: // var, number
      case 0b0011: // blob, var
      case 0b1011: // number, var
      case 0b1111: // var, var
        lhs++;
        rhs++;
        break; // may match

      case 0b0111: // term, var
        lhs = dc.decode_object(lhs).end(); // jump over the term
        rhs++;
        break; // may match

      case 0b1101: // var, term
        rhs = dc.decode_object(rhs).end(); // jump over the term
        lhs++;
        break; // may match
    }
  }

  return true;
}


static bool
_match_var_nonvar(runtime &rt, object_iterator var, object_iterator val,
                  size_t _, matcher::memory &mem)
{
  basic_decoder dc;
  nonterminal nonterm;

  dc.decode(*var, nonterm);
  if (auto varobj = rt.dereferencer(nonterm.id))
  {
    if (not mem.emplace(var, val).second)
      return true;
    TAILCALL match(rt, *varobj, val, 1, mem);
  }
  else
  {
    rt.assign(nonterm.id, val);
    return true;
  }
}

static bool
_match_var_var(runtime &rt, object_iterator lhs, object_iterator rhs, size_t _,
               matcher::memory &mem)
{
  basic_decoder dc;
  nonterminal lhsvar, rhsvar;
  dc.decode(*lhs, lhsvar);
  dc.decode(*rhs, rhsvar);
  // bound + bound => dereference both and match results
  // bound + unbound => bind
  // unbound + unbound => bind
  const auto lhsobj = rt.dereferencer(lhsvar.id);
  const auto rhsobj = rt.dereferencer(rhsvar.id);
  if (lhsobj and rhsobj)
  {
    if (not mem.emplace(lhs, rhs).second)
      return true;
    TAILCALL match(rt, *lhsobj, *rhsobj, 1, mem);
  }
  else
  {
    rt.bind(lhsvar.id, rhsvar.id);
    return true;
  }
}

bool
match(runtime &rt, object_iterator lhs, object_iterator rhs, size_t n,
           matcher::memory &mem)
{
  basic_decoder dc;

  while (n--)
  {
    switch (((*lhs & 0b11) << 2) | (*rhs & 0b11))
    {
      case 0b0000: // blob, blob
        if (blob_tag(*lhs) != blob_tag(*rhs))
          return false;
        switch (blob_tag(*lhs))
        {
          case blob_tag::string:
            if (string(*lhs++) == string(*rhs++))
              continue;
            else
              return false;
          default:
            throw std::runtime_error {"invalid blob tag"};
        }

      case 0b0101: // term, term
        if (the_word(*lhs) != the_word(*rhs))
          return false;
        n += dc.decode_term_header(*lhs).arity; // proceeed to match the fields
        lhs++;
        rhs++;
        continue;

      case 0b1010: // number, number
        if (*lhs++ != *rhs++) // trivially comparable
          return false;
        continue;

      case 0b0001: // blob, term
      case 0b0100: // term, blob
      case 0b0010: // blob, number
      case 0b1000: // number, blob
      case 0b0110: // term, number
      case 0b1001: // number, term
        return false;

      case 0b0011: // blob, var
      case 0b1011: // number, var
        std::swap(lhs, rhs);
        // fall through
      case 0b1100: // var, blob
      case 0b1110: // var, number
        if (n == 0)
          TAILCALL _match_var_nonvar(rt, lhs, rhs, 0, mem);
        else if (_match_var_nonvar(rt, lhs++, rhs++, 0, mem))
          continue;
        else
          return false;

      case 0b0111: // term, var
        std::swap(lhs, rhs);
        // fall through
      case 0b1101: // var, term
        if (n == 0)
          TAILCALL _match_var_nonvar(rt, lhs, rhs, 0, mem);
        else if (_match_var_nonvar(rt, lhs, rhs, 0, mem))
        {
          lhs++;
          dc.decode_object(rhs); // call for side-effects
          continue;
        }
        else
          return false;

      case 0b1111: // var, var
        if (*lhs == *rhs)
        {
          lhs++;
          rhs++;
          continue;
        }
        if (n == 0)
          TAILCALL _match_var_var(rt, lhs, rhs, 0, mem);
        else if (_match_var_var(rt, lhs++, rhs++, 0, mem))
          continue;
        else
          return false;

      default:
        assert(not "unreachable code");
    }
  }

  return true;
}


static bool
_match_var_nonvar_uw(runtime &rt, object_iterator var, object_iterator val,
                     size_t _, matcher::memory &mem, barrier bar)
{
  basic_decoder dc;
  nonterminal nonterm;

  dc.decode(*var, nonterm);
  if (auto varobj = rt.dereferencer(nonterm.id))
  {
    if (not mem.emplace(var, val).second)
      return true;
    TAILCALL match_uw(rt, *varobj, val, 1, mem, bar);
  }
  else
  {
    rt.assign_uw(nonterm.id, val, bar);
    return true;
  }
}

static bool
_match_var_var_uw(runtime &rt, object_iterator lhs, object_iterator rhs,
                  size_t _, matcher::memory &mem, barrier bar)
{
  basic_decoder dc;
  nonterminal lhsvar, rhsvar;
  dc.decode(*lhs, lhsvar);
  dc.decode(*rhs, rhsvar);
  // bound + bound => dereference both and match results
  // bound + unbound => bind
  // unbound + unbound => bind
  const auto lhsobj = rt.dereferencer(lhsvar.id);
  const auto rhsobj = rt.dereferencer(rhsvar.id);
  if (lhsobj and rhsobj)
  {
    if (not mem.emplace(lhs, rhs).second)
      return true;
    TAILCALL match_uw(rt, *lhsobj, *rhsobj, 1, mem, bar);
  }
  else
  {
    rt.bind_uw(lhsvar.id, rhsvar.id, bar);
    return true;
  }
}

bool
match_uw(runtime &rt, object_iterator lhs, object_iterator rhs, size_t n,
         matcher::memory &mem, barrier bar)
{
  basic_decoder dc;

  while (n--)
  {
    switch (((*lhs & 0b11) << 2) | (*rhs & 0b11))
    {
      case 0b0000: // blob, blob
        if (blob_tag(*lhs) != blob_tag(*rhs))
          return false;
        switch (blob_tag(*lhs))
        {
          case blob_tag::string:
            if (string(*lhs++) == string(*rhs++))
              continue;
            else
              return false;
          default:
            throw std::runtime_error {"invalid blob tag"};
        }

      case 0b0101: // term, term
        if (the_word(*lhs) != the_word(*rhs))
          return false;
        n += dc.decode_term_header(*lhs).arity; // proceeed to match the fields
        lhs++;
        rhs++;
        continue;

      case 0b1010: // number, number
        if (*lhs++ != *rhs++) // trivially comparable
          return false;
        continue;

      case 0b0001: // blob, term
      case 0b0100: // term, blob
      case 0b0010: // blob, number
      case 0b1000: // number, blob
      case 0b0110: // term, number
      case 0b1001: // number, term
        return false;

      case 0b0011: // blob, var
      case 0b1011: // number, var
        std::swap(lhs, rhs);
        // fall through
      case 0b1100: // var, blob
      case 0b1110: // var, number
        if (n == 0)
          TAILCALL _match_var_nonvar_uw(rt, lhs, rhs, 0, mem, bar);
        else if (_match_var_nonvar_uw(rt, lhs++, rhs++, 0, mem, bar))
          continue;
        else
          return false;

      case 0b0111: // term, var
        std::swap(lhs, rhs);
        // fall through
      case 0b1101: // var, term
        if (n == 0)
          TAILCALL _match_var_nonvar_uw(rt, lhs, rhs, 0, mem, bar);
        else if (_match_var_nonvar_uw(rt, lhs, rhs, 0, mem, bar))
        {
          lhs++;
          dc.decode_object(rhs); // call for side-effects
          continue;
        }
        else
          return false;

      case 0b1111: // var, var
        if (*lhs == *rhs)
        {
          lhs++;
          rhs++;
          continue;
        }
        if (n == 0)
          TAILCALL _match_var_var_uw(rt, lhs, rhs, 0, mem, bar);
        else if (_match_var_var_uw(rt, lhs++, rhs++, 0, mem, bar))
          continue;
        else
          return false;

      default:
        assert(not "unreachable code");
    }
  }

  return true;
}
