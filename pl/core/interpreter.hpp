#pragma once

#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/tape_writer.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/obj/object.hpp"

#include <format>
#include <functional>
#include <iostream>
#include <sstream>
#include <set>


using continuation = std::function<void(runtime&)>;

using meta_op_handle =
    std::function<void(runtime &, size_t, object_iterator, continuation &)>;

#ifdef __clang__
# warning "Won't ensure tail-calls with clang. Your stack is may evaporate."
# define TAILCALL return
#elif ASTARI_DEBUG
# define TAILCALL return
#else
# define TAILCALL [[gnu::musttail]] return
#endif

class exception: public std::exception {
  public:
  exception(std::string_view msg, object_view term)
  : m_term {term}, m_what {msg}
  { }
  
  const char *
  what() const noexcept override
  { return m_what.c_str(); }

  object_view
  term() const noexcept
  { return m_term; }

  private:
  object m_term;
  std::string m_what;
};


class interpreter: public runtime {
  enum meta_symbol {
    op_and,
    op_or,
    op_if,
    op_fail,
  };

  public:
  interpreter();

  void
  debug() const
  {
    basic_decoder dc;
    for (const auto &[w, variants] : m_predicates)
    {
      term_header hdr;
      dc.decode(w, hdr);
      std::cerr << std::format("  have {}/{}:", m_symdict[hdr.id], hdr.arity)
                << std::endl;
      for (const auto &[s, b] : variants)
      {
        if (b.empty())
          std::clog << "  - " << dump(s) << "." << std::endl;
        else
          std::clog << "  - " << dump(s) << " :- " << dump(b) << std::endl;
      }
    }
    for (const auto &[id, _] : m_metaops)
      std::cerr << std::format("  have {}/*:", m_symdict[id]) << std::endl;
  }

  dictionary &
  symbols() noexcept
  { return m_symdict; }

  template <typename ...Args>
  object_view
  make_term(Args &&...args)
  {
    object buf;
    tape_writer tape {std::back_inserter(buf), symbols()};
    tape.operator<<(std::forward<Args>(args)...);
    word_t *p = allocate(buf.size());
    std::copy(buf.begin(), buf.end(), p);
    return {p, buf.size()};
  }

  template <typename ...Args>
  object
  make_term_noalloc(Args &&...args)
  {
    object buf;
    tape_writer tape {std::back_inserter(buf), symbols()};
    tape.operator<<(std::forward<Args>(args)...);
    return buf;
  }

  void
  add_predicate(object_view sign, object_view body);

  void
  add_predicate(object_view sign);

  void
  add_meta_op(std::string_view name, const meta_op_handle &handle);

  bool
  has_meta_op(size_t id) const noexcept;

  bool
  has_predicate(size_t id) const noexcept;

  bool
  has(size_t id) const noexcept;

  void
  operator << (std::string_view str)
  { std::istringstream ss {str.data(), std::ios_base::binary}; load(ss); }

  /**
   * @name Script loading primitives
   * @{
   */
  /**
   * @brief Load script from a file
   * @details Parse all statements in the file and @ref interpret them.
   * @param path File path.
   */
  void
  load_file(std::string_view path);

  /**
   * @brief Load object file
   * @details Get all statements from the file and @ref interpret them.
   * @param path File path.
   */
  void
  load_objfile(std::string_view path);

  /**
   * @brief Load script from a stream
   * @details Parse all statements in the stream and @ref interpret them.
   * @param in Input stream.
   */
  void
  load(std::istream &in);
  /** @} */

  /**
   * @name Importing libraries
   * @{
   */
  void
  ensure_loaded(std::string_view path);

  void
  import_directory(std::string_view path)
  { m_impordirs.emplace(path); }
  /** @} */

  void
  eval(object_view obj, const dictionary &vardict);

  void
  eval(std::string_view expr);

  void
  interpret(object_view stmt, const dictionary &vardict = {});

  std::string
  dump(object_view obj) const
  { return dump_object(m_symdict, obj); }

  using solution = std::unordered_map<std::string_view, object>;

  void
  make_true(object_view expr, const continuation &cont)
  {
    runtime save = *this;
    make_true(*this, expr, cont);
    static_cast<runtime&>(*this) = save;
  }

  void
  make_true(runtime &rt, object_view expr, continuation &cont)
  { _make_true(rt, 0, expr.begin(), cont); }

  void
  make_true(runtime &rt, object_view expr, continuation cont)
  { _make_true(rt, 0, expr.begin(), cont); }

  template <typename Object>
  object
  make(Object what)
  {
    object term;
    tape_writer tape {std::back_inserter(term), m_symdict};
    tape << what;
    return term;
  }

  template <typename Object>
  [[noreturn]] void
  raise(Object what);

  template <typename Cont>
  void
  number(runtime &rt, object_iterator x, Cont &&c);

  private:
  void
  _make_true(runtime &rt, size_t _, object_iterator e, continuation &cont);

  void
  _make_true__and(runtime &rt, size_t i, object_iterator eit,
                  continuation &cont);

  void
  _make_true__or(runtime &rt, size_t i, object_iterator eit,
                 continuation &cont);

  void
  _make_true__if(runtime &rt, size_t _, object_iterator eit,
                 continuation &cont);

  void
  _make_true__predicate(runtime &rt, size_t _, object_iterator e,
                        continuation &cont);

  private:
  std::unordered_map<word_t, std::vector<std::pair<object, object>>> m_predicates;
  std::unordered_map<size_t, meta_op_handle> m_metaops;
  dictionary m_symdict;
  std::set<std::string> m_impordirs;
  std::set<std::string> m_imports;
}; // class interpreter


template <typename Object>
[[noreturn]] void
interpreter::raise(Object what)
{
  object term;
  tape_writer tape {std::back_inserter(term), m_symdict};
  tape << what;

  std::ostringstream msg;
  dump_object(m_symdict, term, msg);

  throw exception {msg.str(), term};
}


template <typename Cont>
void
interpreter::number(runtime &rt, object_iterator x, Cont &&c)
{
  basic_decoder dc;
  if (is_nonterminal(x[0]))
  {
    nonterminal var;
    dc.decode(x[0], var);
    if (auto xval = rt.dereference(var.id))
      x = xval.value();
    else
      raise(term("instantiation_error"));
  }

  switch (word_type(x[0]))
  {
    case word_type::signed_int_number:
    {
      int val;
      dc.decode(x[0], val);
      c(val);
      return;
    }

    case word_type::unsigned_int_number:
    {
      unsigned val;
      dc.decode(x[0], val);
      c(val);
      return;
    }

    case word_type::float_number:
    {
      float val;
      dc.decode(x[0], val);
      c(val);
      return;
    }

    default:
      raise(term("type_error", term("number"), dc.decode_object(x)));
      return;
  }
}

static object
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


static std::pair<object, size_t>
unmake_list(interpreter &pl, runtime &rt, object_iterator it)
{
  basic_encoder ec;
  basic_decoder dc;
  const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));
  const word_t cons2 = ec.encode(term_header(pl.symbols()["cons"], 2));

  size_t n = 0;
  object buf;
  for (it = rt.reduce(it); *it != nil0; it = rt.reduce(it))
  {
    assert(*it++ == cons2);
    buf += rt.reduce(dc.decode_object(it));
    n++;
  }

  return {buf, n};
}


static object
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


static std::pair<object, size_t>
unmake_list(interpreter &pl, object_iterator it)
{
  basic_encoder ec;
  basic_decoder dc;
  const word_t nil0 = ec.encode(term_header(pl.symbols()["nil"], 0));
  const word_t cons2 = ec.encode(term_header(pl.symbols()["cons"], 2));

  size_t n = 0;
  object buf;
  while (*it != nil0)
  {
    assert(*it++ == cons2);
    buf += dc.decode_object(it);
    n++;
  }

  return {buf, n};
}
