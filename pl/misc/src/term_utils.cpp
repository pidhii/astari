#include "term_utils.hpp"
#include "pl/core/interpreter.hpp"


object
make_list(interpreter &pl, runtime &rt, size_t n, object_iterator it)
{
  basic_encoder ec;
  basic_decoder dc;
  const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));
  const word_t cons2 = ec.encode(term_header(pl.symbols()["cons"], 2));

  object buf;
  while (n--)
  {
    buf += cons2;
    buf += rt.reduce(dc.decode_object(it));
  }
  buf += nil0;

  return buf;
}


std::pair<object, size_t>
unmake_list(interpreter &pl, runtime &rt, object_iterator it)
{
  basic_encoder ec;
  basic_decoder dc;
  const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));
  [[maybe_unused]] const word_t cons2 =
      ec.encode(term_header(pl.symbols()["cons"], 2));

  size_t n = 0;
  object buf;
  for (it = rt.reduce(it); (*it & term_mask) != nil0; it = rt.reduce(it))
  {
    assert((*it & term_mask) == cons2); it++;
    buf += rt.reduce(dc.decode_object(it));
    n++;
  }

  return {buf, n};
}


object
make_list(interpreter &pl, size_t n, object_iterator it)
{
  basic_encoder ec;
  basic_decoder dc;
  const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));
  const word_t cons2 = ec.encode(term_header(pl.symbols()["cons"], 2));

  object buf;
  while (n--)
  {
    buf += cons2;
    buf += dc.decode_object(it);
  }
  buf += nil0;

  return buf;
}


std::pair<object, size_t>
unmake_list(interpreter &pl, object_iterator it)
{
  basic_encoder ec;
  basic_decoder dc;
  const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));
  [[maybe_unused]] const word_t cons2 =
      ec.encode(term_header(pl.symbols()["cons"], 2));

  size_t n = 0;
  object buf;
  while ((*it & term_mask) != nil0)
  {
    assert((*it & term_mask) == cons2); it++;
    buf += dc.decode_object(it);
    n++;
  }

  return {buf, n};
}


word_t
strdup(object_allocator &alloc, std::string_view str)
{
  const size_t size = sizeof(string_data) + str.size();
  const size_t nwords = (size + sizeof(word_t) - 1) / sizeof(word_t);
  string_data *p = reinterpret_cast<string_data*>(alloc.allocate(nwords));
  p->tag = blob_tag::string;
  p->size = str.size();
  std::copy(str.begin(), str.end(), &p->data[0]);
  const word_t result = reinterpret_cast<word_t>(p);
  assert(word_type(result) == word_type::blob);
  return result;
}


word_t
predicate_indicator(interpreter &pl, object_iterator indicator)
{
  basic_encoder ec;
  basic_decoder dc;
  #define ATOM(name, arity) ec.encode(term_header(m_symdict[name], arity))

  const auto raise_type_error = [&]() {
    raise(pl, term("type_error", term("predicate_indicator"),
                      dc.decode_object(indicator)));
  };

  if (is_term(indicator[0], op_penis, 2))
    raise_type_error();
  object_iterator it = indicator + 1;
  const object_view name = dc.decode_object(it);
  const object_view arity = dc.decode_object(it);
  if (not is_term_n(name[0], 0) or
      word_type(arity[0]) != word_type::signed_int_number)
    raise_type_error();

  term_header hdr;
  int ar;
  dc.decode(name[0], hdr);
  dc.decode(arity[0], ar);
  return ec.encode(term_header(hdr.id, ar));
}


void
transfer_symbols(const dictionary &from, dictionary &to, object &obj)
{
  basic_encoder ec;
  basic_decoder dc;

  for (word_t &word : obj)
  {
    if (is_term(word))
    {
      term_header hdr;
      dc.decode(word, hdr);
      const size_t newid = to[from[hdr.id]];
      word = ec.encode(term_header(newid, hdr.arity));
    }
  }
}


void
recover_variables(runtime &rt, const varnamespace &ns, object &obj)
{
  // FIXME: ensure that unassociated variables dont clash with the originals
  basic_encoder ec;
  basic_decoder dc;
  for (word_t &word : obj)
  {
    if (is_nonterminal(word))
    {
      // Find if this variable is bound with an original variable (from `ns`).
      // If so, rename it to this original ID.
      nonterminal var;
      dc.decode(word, var);
      for (const auto [nsid, rtid] : ns)
      {
        if (rt.bound(var.id, rtid))
        {
          word = ec.encode(nonterminal(nsid));
          break;
        }
      }
    }
  }
}
