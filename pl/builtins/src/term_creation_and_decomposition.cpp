#include "iso.hpp"


void
iso_term_creation_and_decomposition(interpreter &pl)
{
  // =../2
  pl.add_meta_op("=..", [&](runtime &rt, int argc, object_iterator argv,
                              const continuation &cont) {
    basic_encoder ec;
    basic_decoder dc;

    const word_t nil = ec.encode(term_header(pl.symbols()["nil"], 0));
    const word_t cons = ec.encode(term_header(pl.symbols()["cons"], 2));

    const auto isnull = [&](object_iterator x) -> bool {
      return (rt.reduce(x)[0] & term_mask) == nil;
    };

    assert(argc == 2);
    const object_view result = rt.reduce(dc.decode_object(argv));
    const object_view rhs = dc.decode_object(argv);

    if (is_nonterminal(result[0]))
    {
      object_iterator l = rt.reduce(rhs.begin());
      assert((l[0] & term_mask) == cons);

      size_t lsize = 0;
      object buf;
      while (not isnull(l))
      {
        if ((l[0] & term_mask) != cons)
          pl.raise(term("type_error", term("list"), rhs));

        const object_view car = dc.decode_object(l + 1);
        const object_view cdr = dc.decode_object(car.end());
        buf += rt.reduce(car);
        lsize += 1;
        l = rt.reduce(cdr.begin());
      }

      assert(is_term_n(buf[0], 0));
      term_header hdr;
      dc.decode(buf[0], hdr);
      buf[0] = ec.encode(term_header(hdr.id, lsize - 1));

      word_t *p = rt.allocate(buf.size());
      std::copy(buf.begin(), buf.end(), p);
      if (rt.match(result, {p, buf.size()}))
        TAILCALL cont(rt);
      else
        rt.unallocate(buf.size());
    }
    else if (is_term(result[0]))
    {
      term_header hdr;
      dc.decode(result[0], hdr);
      const object arglist = make_list(pl, rt, hdr.arity, result.begin() + 1);
      object list;
      list += cons;
      list += ec.encode(term_header(hdr.id, 0));
      list += arglist;
      word_t *p = rt.allocate(list.size());
      std::copy(list.begin(), list.end(), p);
      if (rt.match({p, list.size()}, rhs))
        TAILCALL cont(rt);
      else
        rt.unallocate({p, list.size()});
    }
  });

  // functor/3
  pl.add_meta_op("functor", [&](runtime &rt, int argc, object_iterator argv,
                                const continuation &cont) {
    assert(argc >= 2);
    basic_decoder dc;
    basic_encoder ec;
    const object_view term = rt.reduce(dc.decode_object(argv));
    if (is_term(term))
    {
      term_header hdr;
      dc.decode(term[0], hdr);
      word_t *termname = rt.allocate(1);
      word_t *termarity = rt.allocate(1);
      *termname = ec.encode(term_header(hdr.id, 0));
      *termarity = ec.encode(int(hdr.arity));
      const object_view name = dc.decode_object(argv);
      const object_view arity = dc.decode_object(argv);
      if (rt.match(name, {termname, 1}) and rt.match(arity, {termarity, 1}))
        TAILCALL cont(rt);
      else
        rt.unallocate(2);
    }
    else
      pl.raise(::term("instantiation_error"));
  });
}