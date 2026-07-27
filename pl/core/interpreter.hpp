/**
 * @file interpreter.hpp
 * @brief Interpreter implementation
 */
#pragma once

#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/obj/object.hpp"

#include <format>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>


using continuation = std::function<void(runtime&)>;

using meta_op_handle =
    std::function<void(runtime &, size_t, object_iterator, continuation &)>;

#ifdef __clang__
# warning "Won't ensure tail-calls with clang. Your stack may evaporate."
# define TAILCALL return
#elif ASTARI_DEBUG
# define TAILCALL return
#else
# define TAILCALL [[gnu::musttail]] return
#endif


enum meta_symbol {
  op_and,
  op_or,
  op_if,
  op_fail,
  op_cut,
  op_plus,
  op_minus,
  op_mul,
  op_div,
  op_divdiv,
  op_penis,
};


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


class prolog_parser;


/**
 * @ingroup core
 * @brief Prolog interpreter
 */
class interpreter: private runtime {
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
      for (const auto &pred : variants)
      {
        if (pred.body.empty())
          std::clog << "  - " << dump(pred.sign) << "." << std::endl;
        else
          std::clog << "  - " << dump(pred.sign) << " :- " << dump(pred.body)
                    << std::endl;
      }
    }
    for (const auto &[id, _] : m_metaops)
      std::cerr << std::format("  have {}/*:", m_symdict[id]) << std::endl;
  }

  object_allocator &
  global_memory()
  { return *this; }

  dictionary &
  symbols() noexcept
  { return m_symdict; }

  /**
   * @name Database interface
   * @{
   */
  /**
   * @brief Check if there exists a predicate or meta-op with a given name
   * @details Check static, dynamic, and *meta-ops* -databases for predicates with
   * names matching @p name.
   * @param name ID of a name-symbol.
   */
  bool
  has(size_t name) const noexcept;

  /**
   * @brief Check if there exists a predicate with a given name
   * @details Check static and dynamic databases for the given predicate.
   * This procedure does not check **_meta-ops_**. Use @ref interpreter::has to
   * account for *meta-ops* as well.
   * @param name ID of a name-symbol.
   */
  bool
  has_predicate(size_t name) const noexcept;

  /**
   * @brief Check if there exists a *meta-op* with a given name
   * @param name ID of a name-symbol.
   */
  bool
  has_meta_op(size_t name) const noexcept;

  /**
   * @brief Check if predicates associated to a *signature-key* are dynamic
   * @param signkey First *word* of a predicate term.
   */
  bool
  is_dynamic(word_t signkey) const noexcept;

  /**
   * @brief Check if predicates associated to a *signature-key* are static
   * @param signkey First *word* of a predicate term.
   */
  bool
  is_static(word_t signkey) const noexcept;

  /**
   * @brief Declare that predicates associated to a *signature-key* are dynamic
   * @param signkey First *word* of a predicate term.
   * @throw std::runtime_error - if declaration would alter existing qualifier.
   */
  void
  dynamic(word_t signkey);

  /**
   * @brief Add a predicate
   * @details Addition of the clause @p sign :- @p body to the database BEFORE
   * all other clauses for the predicate associated to @p sign. The procedure
   * automatically determines whether the predicate is static or dynamic and
   * puts it in the appropriate table.
   * @param sign Signature/head of a predicate clause.
   * @param body[opt] Body of a predicate clause, or empty view which is
   * equivalent to a body consisting of a single `true`-statement.
   * @throw std::runtime_error - if predicate definition would conflict with
   * existing *meta-op*.
   */
  void
  asserta(object_view sign, object_view body = {});

  /**
   * @brief Add a predicate
   * @details Addition of the clause @p sign :- @p body to the database AFTER
   * all other clauses for the predicate associated to @p sign. The procedure
   * automatically determines whether the predicate is static or dynamic and
   * puts it in the appropriate table.
   * @param sign Signature/head of a predicate clause.
   * @param body[opt] Body of a predicate clause, or empty view which is
   * equivalent to a body consisting of a single `true`-statement.
   * @throw std::runtime_error - if predicate definition would conflict with
   * existing *meta-op*.
   */
  void
  assertz(object_view sign, object_view body = {});

  /**
   * @brief Add a *meta-op* (hooks to define predicates in C++)
   * @throw std::runtime_error - if name is associated with any other predicate
   * or *meta-op*.
   */
  void
  add_meta_op(std::string_view name, const meta_op_handle &handle);
  /** @} */

  /**
   * @name Script loading primitives
   * @{
   */
  /**
   * @brief Load script from a file
   * @details Parse all statements in the file and @ref interpret them.
   * @param path File path.
   * @throw std::runtime_error - if failed to open the file under the given
   * @p path.
   */
  void
  load_file(std::string_view path);

  /**
   * @brief Load object file
   * @details Get all statements from the file and @ref interpret them.
   * @param path File path.
   * @throw std::runtime_error - if failed to open the file under the given
   * @p path.
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

  /**
   * @brief Load script from a text string
   * @details Parse all statements in the string and @ref interpret them.
   * @param text String with Prolog statements.
   */
  void
  operator << (std::string_view text)
  { std::istringstream ss {text.data(), std::ios_base::binary}; load(ss); }
  /** @} */

  /**
   * @name Importing libraries
   * @{
   */
  /**
   * @brief Add a prefix for library resolutions
   * @details Adds a prefix @p path to the prefix resolution list at the
   * position with highest priority.
   * @param path A path in filesystem (can be either absolute or relative).
   * @see @ref interpreter::ensure_loaded
   */
  void
  import_directory(std::string_view path) noexcept
  { m_importdirs.emplace(m_importdirs.begin(), path); }

  /**
   * @brief Require a script or objects loaded into interpreter
   * @details
   * 1. Resolve the @p path using *import directories* provided via
   *    interpreter::import_directory, matching with files ending with
   *    @p path + `".plo"` and @p path + `".pl"` (in this priority), unless
   *    @p path is already ending with one of these extensions. The first
   *    successful match is a *resolved file path*;
   * 2. Test if the *full path* of the *resolved file path* was already loaded
   *    with `ensure_loaded`:
   *    - *yes* -> (3);
   *    - *no* -> (4).
   * 3. Pass and return.
   * 5. Memorise the *full path* to prevent it from loading again;
   * 6. Load the file using either @ref interpreter::load_file or
   *    @ref interpreter::load_objfile, depending on the extension of the
   *    resolved path.
   * @param path Library name (see detailed description for exact interpretation
   * of parameter)
   * @throw std::runtime_error - if failed to resolve the path.
   */
  void
  ensure_loaded(std::string_view path);
  /** @} */

  void
  eval(object_view obj, const dictionary &vardict);

  void
  eval(std::string_view expr);

  void
  interpret(prolog_parser &p, object_view stmt, const dictionary &vardict = {});

  std::string
  dump(object_view obj) const
  { return dump_object(m_symdict, obj); }

  using solution = std::unordered_map<std::string_view, object>;
  void
  make_true(const dictionary &vardict, object_view expr,
            const std::function<void(const solution &)> &cont,
            bool recover_vars = false);

  /**
   * @warning Do not use this as an entry point of a query.
   */
  void
  make_true(runtime &rt, object_view expr, continuation &cont)
  { TAILCALL _make_true(rt, 0, expr.begin(), nullptr, cont); }

  /**
   * @warning Do not use this as an entry point of a query.
   */
  void
  make_true(runtime &rt, object_view expr, continuation cont)
  { _make_true(rt, 0, expr.begin(), nullptr, cont); }

  private:
  void
  _make_true(runtime &rt, size_t _, object_iterator e, barrier *clause,
             continuation &cont);

  void
  _make_true__and(runtime &rt, size_t i, object_iterator eit,
                  barrier *clause, continuation &cont);

  void
  _make_true__or(runtime &rt, size_t i, object_iterator eit,
                 barrier *clause, continuation &cont);

  void
  _make_true__if(runtime &rt, size_t _, object_iterator eit,
                 barrier *clause, continuation &cont);

  void
  _make_true__predicate(runtime &rt, size_t _, object_iterator e,
                        barrier *clause, continuation &cont);

  private:
  std::unordered_map<word_t, std::vector<predicate_entry>> m_predicates;
  std::unordered_map<size_t, meta_op_handle> m_metaops;
  dictionary m_symdict;
  std::unordered_set<size_t> m_dynamic_names;
  std::vector<std::string> m_importdirs;
  std::set<std::string> m_imports;
  std::unique_ptr<size_t[]> m_unwind_heap;
  std::unique_ptr<word_t[]> m_term_heap;
}; // class interpreter

