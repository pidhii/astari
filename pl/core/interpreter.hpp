/**
 * @file interpreter.hpp
 * @brief Interpreter implementation
 *
 * @details
 * This is the main public header of the *astari* Prolog engine. It defines
 * @ref interpreter -- a self-contained, embeddable Prolog interpreter -- along
 * with the small set of supporting types that make up the C++/Prolog
 * boundary:
 * - @ref continuation -- a Prolog *success continuation* (what to do next
 *   after a goal succeeds);
 * - @ref meta_op_handle -- the signature of a native (C++) predicate hook,
 *   a.k.a. *meta-op*;
 * - @ref meta_symbol -- IDs of symbols that the interpreter treats specially
 *   (control constructs);
 * - @ref exception -- the C++ exception type used to carry a Prolog error
 *   term across `throw`/`catch`.
 */
#pragma once

#include "runtime.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/dictionary.hpp"
#include "pl/misc/display.hpp"
#include "pl/misc/object_file.hpp"
#include "pl/obj/object.hpp"
#include "utl/tcfunction.hpp"

#include <format>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>


#ifdef __clang__
# warning "Won't ensure tail-calls with clang. Your stack may evaporate."
// # define TAILCALL [[clang::musttail]] return
#define TAILCALL return
#elif ASTARI_DEBUG
# define TAILCALL return
#else
/**
 * @ingroup core
 * @brief Marks a `return`-statement as requiring a guaranteed tail-call
 * @details The interpreter's evaluation model (@ref interpreter::make_true
 * and friends) relies on proper tail-calls to avoid growing the native stack
 * for arbitrarily long/deep Prolog derivations (e.g. tail-recursive
 * predicates, long conjunctions, disjunctions). On GCC this expands to
 * `[[gnu::musttail]] return`, which causes a *compile error* if the compiler
 * cannot guarantee the tail-call -- this is intentional, as a silently
 * missed tail-call would otherwise blow the native stack at runtime for
 * deep recursion. On clang, or other LLVM-based compiler, the limitations on
 * tail-calls are to strict and (as of now) are not met by the code.
 */
# define TAILCALL [[gnu::musttail]] return
#endif

#ifdef __clang__
# define NOINLINE
#else
# define NOINLINE __attribute__((noinline))
#endif

#define DONE (tcfunction<void()> {})
#define FAIL (continuation {})

/**
 * @ingroup core
 * @brief Signature of a native (C++) predicate implementation, a.k.a. *meta-op*
 * @details Registered via @ref interpreter::add_meta_op. A meta-op behaves
 * like a Prolog predicate but its body is implemented in C++ instead of
 * Prolog. It receives:
 * - the current @ref runtime *sprout*;
 * - the arity with which it was called;
 * - an iterator to the (encoded) argument list;
 * - the @ref continuation to invoke for every solution.
 * @see @ref interpreter::add_meta_op
 */
using meta_op_handle =
    tcfunction<continuation(runtime &, size_t, object_iterator, continuation)>;
// std::function<void(runtime &, size_t, object_iterator, continuation &)>;


/**
 * @ingroup core
 * @brief IDs of select subset of frequently occuring symbols for fast dispatch
 *
 * @details These are the control constructs and arithmetic operators that
 * @ref interpreter::_make_true and the arithmetic evaluator recognize
 * directly rather than looking them up in dictionary. The @ref interpreter
 * constructor registers the corresponding names (`,`, `;`,
 * `if`, `fail`, `!`, `+`, `-`, `*`, `/`, `//`, `:-`) in the symbol
 * dictionary and asserts that they map to exactly these enumerators.
 * This is rather a low-level implementation detail; user code rarely needs
 * this.
 */
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//                  THESE ENUMS MUST BE IN SYNC WITH THE ORDER OF
//                  REQUIRES IN THE CONSTRUCTOR OF INTERPRETER
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
  op_sqrt,
  op_penis,
};


/**
 * @ingroup core
 * @brief C++ exception carrying a Prolog error term
 * @details Thrown by @ref raise (see `pl/misc/term_utils.hpp`) to bridge
 * Prolog-level `throw/1` errors (`instantiation_error`, `type_error/2`,
 * etc.) as well as internal interpreter faults into the native C++
 * exception mechanism. `catch/3` (see `pl/builtins/src/throwcatch.cpp`)
 * catches instances of this type and re-injects @ref exception::term into
 * the Prolog computation for unification against the catcher pattern.
 *
 * @code{.cpp}
 * try
 * {
 *   pl.eval("X is 1 + foo");
 * }
 * catch (const exception &exn)
 * {
 *   std::cerr << "Prolog error: " << exn.what() << std::endl;
 *   // exn.term() holds the raw error term, e.g. type_error(evaluable, foo)
 * }
 * @endcode
 */
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
 *
 * @details
 * `interpreter` is the top-level, embeddable Prolog engine of *astari*. A
 * single instance owns:
 * - the **predicate database**, split into *static* predicates (fixed set
 *   of clauses, indexed by `object_view`, immutable during runtime)
 *   and *dynamic* predicates (mutable at runtime via `assert*`/`retract`,
 *   see @ref runtime's *Dynamic database interface*);
 * - the **meta-op table**, i.e. native C++ predicate implementations
 *   registered with @ref add_meta_op;
 * - the **symbol dictionary** (@ref symbols), mapping atom/functor names to
 *   compact integer IDs used everywhere internally (@ref dictionary);
 * - the **memory pools**: a term heaps for allocation of backtrackable
 *   and persistent (non-backtrackable) data and an unwind heap used to record
 *   undo information for backtracking (see @ref runtime, @ref barrier,
 *   @ref query_state);
 * - bookkeeping for **library resolution** (@ref import_directory,
 *   @ref ensure_loaded) so that Prolog source files can `:- ensure_loaded(...)`
 *   each other similarly to other Prolog systems.
 *
 * `interpreter` privately inherits from @ref runtime, meaning every
 * `interpreter` is a root/initial *sprout* of a query, while also owning
 *  everything a `runtime` needs to actually be dereferenced against (predicate
 *  tables, symbol dictionary, memory).
 *
 * @warning `interpreter` instances are **not** thread-safe: a single
 * instance (and the `runtime` *sprouts* derived from it) must not be driven
 * concurrently from multiple native threads. Cooperative concurrency within
 * a single thread (e.g. multiple *sprouts* of the same query, as used by
 * `lib_breadthfirst`) is the intended way of expressing concurrent search.
 */
class interpreter: private runtime {
  public:
  /**
   * @brief Construct a fresh interpreter
   *
   * @details Allocates the term heap and unwind heap (see @ref query_state),
   * initializes an empty database, and registers the small set of symbols with
   * special meaning to the engine (see @ref meta_symbol) -- notably `,`, `;`,
   * `if`, `fail`, `!`, the arithmetic operators, and `:-`. Constructor does
   * not register any predicates itself. The database of a new interpreter
   * instance is completely empty.
   */
  interpreter();

  /**
   * @brief (DEUBG) Dump the current static predicate and meta-op database to
   *        `stderr`
   */
  void
  debug() const
  {
    basic_decoder dc;
    for (const auto &[w, variants] : m_predicates)
    {
      term_header hdr;
      dc.decode(w, hdr);
      std::cerr << std::format("  have {}/{}", m_symdict[hdr.id], hdr.arity)
                << std::endl;
      // for (const auto &pred : variants)
      // {
      //   if (pred.body.empty())
      //     std::clog << "  - " << dump(pred.sign) << "." << std::endl;
      //   else
      //     std::clog << "  - " << dump(pred.sign) << " :- " << dump(pred.body)
      //               << std::endl;
      // }
    }
    for (const auto &[id, _] : m_metaops)
      std::cerr << std::format("  have {}/*:", m_symdict[id]) << std::endl;
  }

  size_t
  heap_size() const noexcept
  { return m_query->heap_e - m_term_heap.get(); }

  ssize_t
  heap_remsize() const noexcept
  { return m_query->heap_e - m_query->heap_p; }


  /**
   * @brief Access the global (non-backtrackable) memory allocator
   *
   * @details Returns the @ref object_allocator that `interpreter` inherits
   * (transitively, via @ref runtime). Useful for allocating persistent
   * objects when setting up the interpreter like precompiled constant terms or
   * strings.
   */
  object_allocator &
  global_memory()
  { return *this; }

  /**
   * @brief Access the interpreter's symbol dictionary
   *
   * @details Maps atom/functor names to/from the compact integer IDs used
   * everywhere internally to represent terms (see @ref dictionary,
   * @ref term_header). Builtin libraries and user code alike use this to look
   * up or intern names, e.g. `pl.symbols()["foo"]` returnes the ID for atom
   * `foo`. It is permitted to add new names to the symbols dictionary at any
   * moment, which happens automatically upon referencing such name.
   */
  dictionary &
  symbols() noexcept
  { return m_symdict; }

  /**
   * @name Database interface
   * @{
   */
  /**
   * @brief Check if there exists a predicate or meta-op with a given name
   * @param name ID of a name-symbol.
   *
   * @details Check static, dynamic, and *meta-ops*-databases for predicates with
   * names matching @p name.
   */
  bool
  has(size_t name) const noexcept;

  /**
   * @brief Check if there exists a predicate with a given name
   * @param name ID of a name-symbol.
   *
   * @details Check static and dynamic databases for the given predicate.
   * This procedure does not check **_meta-ops_**. Use @ref interpreter::has to
   * account for *meta-ops* as well.
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
   * @param sign Signature/head of a predicate clause.
   * @param body[opt] Body of a predicate clause, or empty view which is
   * equivalent to a body consisting of a single `true`-statement.
   * @throw std::runtime_error - if predicate definition would conflict with
   * existing *meta-op*.
   *
   * @details Addition of the clause @p sign :- @p body to the database BEFORE
   * all other clauses for the predicate associated to @p sign. The procedure
   * automatically determines whether the predicate is static or dynamic and
   * puts it in the appropriate table.
   *
   * @note By default all predicates have *static* qualifiers, until explicitly
   * declared *dynamic*. Addition of a predicate without a prior *dynamic*
   * declaration locks the predicate towards *static* database.
   */
  void
  asserta(object_view sign, object_view body = {});

  /**
   * @brief Add a predicate
   * @param sign Signature/head of a predicate clause.
   * @param body[opt] Body of a predicate clause, or empty view which is
   * equivalent to a body consisting of a single `true`-statement.
   * @throw std::runtime_error - if predicate definition would conflict with
   * existing *meta-op*.
   *
   * @details Addition of the clause @p sign :- @p body to the database AFTER
   * all other clauses for the predicate associated to @p sign. The procedure
   * automatically determines whether the predicate is static or dynamic and
   * puts it in the appropriate table.
   *
   * @note By default all predicates have *static* qualifiers, until explicitly
   * declared *dynamic*. Addition of a predicate without a prior *dynamic*
   * declaration locks the predicate towards *static* database.
   */
  void
  assertz(object_view sign, object_view body = {});

  /**
   * @brief Add a *meta-op* (hooks to define predicates in C++)
   * @param name Name of the predicate to hook (its arity is not fixed here;
   * the handler receives the actual arity used at each call site).
   * @param handle Native implementation, see @ref meta_op_handle.
   * @throw std::runtime_error - if name is associated with any other predicate
   * or *meta-op*.
   *
   * @details This is the primary extension mechanism of the interpreter: all
   * builtin libraries are implemented on top of it. See @ref meta_op_handle for
   * the handler signature.
   */
  template <typename F>
  void
  add_meta_op(std::string_view name, F &&handle)
  {
    const size_t id = m_symdict[name];
    if (has(id))
    {
      throw std::runtime_error {std::format(
          "duplicate names for meta operators are not allowed ({})", name)};
    }
    m_metaops.emplace(id, meta_op_handle::from_lambda(handle));
  }
  /** @} */

  /**
   * @name Script loading primitives
   * @{
   */
  /**
   * @brief Load script from a file
   * @param path File path.
   * @throw std::runtime_error - if failed to open the file under the given
   * @p path.
   *
   * @details Parse all statements in the file and @ref interpret them.
   */
  void
  load_file(std::string_view path);

  /**
   * @brief Load object file
   * @param path File path.
   * @throw std::runtime_error - if failed to open the file under the given
   * @p path.
   *
   * @details Get all statements from the file and @ref interpret them.
   *
   * @see `pl/misc/object_file.hpp`, `pl/coding/\*` for the on-disk format of
   * pre-compiled `.plo` object files.
   */
  void
  load_objfile(std::string_view path);

  void
  load_objfile(const object_file &path);

  /**
   * @brief Load script from a stream
   * @param in Input stream.
   *
   * @details Parse all statements in the stream and @ref interpret them.
   */
  void
  load(std::istream &in);

  /**
   * @brief Load script from a text string
   * @param text String with Prolog statements.
   *
   * @details Parse all statements in the string and @ref interpret them.
   * @code{.cpp}
   * pl << "foo(1). foo(2). foo(3).";
   * pl.eval("foo(X)"); // -> X = 1 ; X = 2 ; X = 3
   * @endcode
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
   * @param path A path in filesystem (can be either absolute or relative).
   *
   * @details Adds a prefix @p path to the prefix resolution list at the
   * position with highest priority.
   *
   * @see @ref interpreter::ensure_loaded
   */
  void
  import_directory(std::string_view path);

  /**
   * @brief Require a script or objects loaded into interpreter
   * @param path Library name (see detailed description for exact interpretation
   * of parameter)
   * @throw std::runtime_error - if failed to resolve the path.
   *
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
   *
   * This is what implements the `:- ensure_loaded(lists).` directive at the
   * Prolog level (see @ref interpret).
   *
   * @code{.cpp}
   * pl.import_directory("./mylibs");
   * pl.ensure_loaded("lists"); // resolves to ./mylibs/lists.plo or .pl
   * @endcode
   */
  void
  ensure_loaded(std::string_view path);
  /** @} */

  /**
   * @brief Evaluate query, printing every solution
   * @param obj Encoded goal term.
   * @param vardict Dictionary mapping variable names to the nonterminal IDs
   * used inside @p obj.
   *
   * @details Runs @p obj as a goal (its free variables being named
   * according to @p vardict) and prints, for each solution found, the
   * bindings of every named variable (or "is unbound" if a variable
   * remained unbound). Also prints the query itself before running it. This
   * is what @ref eval(std::string_view) uses after parsing; prefer that
   * overload unless you already have a parsed/encoded goal and its variable
   * dictionary (e.g. obtained from @ref prolog_parser).
   */
  void
  eval(object_view obj, const dictionary &vardict);

  /**
   * @brief Parse and evaluate a query, printing every solution
   * @param expr Prolog goal, e.g. `"member(X, [1,2,3])"`.
   *
   * @details Parses @p expr as a Prolog goal, then behaves like
   * @ref eval(object_view, const dictionary&): prints the goal, runs it via
   * @ref make_true, and prints each solution's variable bindings to
   * `stdout`. This is the simplest way to "just run a query" when embedding
   * the interpreter, e.g. from a REPL (see `apps/astari-pl.cpp`) or for
   * quick scripting/testing.
   * @code{.cpp}
   * pl.eval("X is 2 + 2");
   * // [eval] X is 2+2
   * // yes: X = 4
   * @endcode
   */
  void
  eval(std::string_view expr);

  /**
   * @brief Interpret a single parsed top-level statement
   * @param p Parser instance that produced @p stmt (needed to resolve
   * operator declarations, i.e. `:- op(Prec, Type, Name)`).
   * @param stmt Encoded top-level statement to interpret.
   * @throw std::runtime_error - if @p stmt is not a recognizable
   * directive/rule/fact.
   *
   * @details Dispatches and applies a single top-level term (a directive
   * `:- Goal`, a rule `Head :- Body`, or a bare fact `Head`). Directives such
   * as `ensure_loaded/1`, `import_directory/1`, `dynamic/1` and `op/3` are
   * executed immediately; rules and facts are added to the database
   * via @ref assertz. Used internally by @ref load / @ref load_file /
   * @ref load_objfile to process an entire script one statement at a time;
   * user code normally does not need to call this directly.
   *
   * @note See a list of supported directives in "pl/README.md". Passing an
   * unsupported directive will cause a throw (with appropriate message).
   */
  void
  interpret(prolog_parser &p, object_view stmt);

  /**
   * @brief Render a term to a human-readable string
   * @param obj Encoded term to render.
   * @return Textual representation of @p obj, e.g. `"foo(1,bar,[a,b])"`.
   *
   * @details Convenience wrapper around @ref dump_object using this
   * interpreter's own @ref symbols dictionary to resolve names.
   */
  std::string
  dump(object_view obj) const
  { return dump_object(m_symdict, obj); }

  /**
   * @brief A single query solution: variable name -> bound value
   *
   * @details Produced by the entry-point- @ref make_true variant.
   * A variable maps to an empty `object` if it appears in
   * the goal but remains unbound at the point of the solution. Variables
   * named `"_"` are never included.
   */
  using solution = std::unordered_map<std::string_view, object>;

  /**
   * @brief Find every solution of a goal, invoking a callback for each
   * @param vardict Maps the variable names appearing in @p expr to the
   * nonterminal IDs used to encode them (as produced by, e.g.,
   * @ref prolog_parser::parse_expr).
   * @param expr Encoded goal to solve.
   * @param cont Invoked once per solution with a @ref solution snapshot of
   * all named variable bindings. You can throw from @p cont to abort the search
   * early; but any exception propagates out of `make_true` after choice points
   * are properly unwound.
   * @param recover_vars If `true`, variables that remain bound to other
   * (still-unbound) query variables are rewritten back to reference those
   * variable names (via @ref recover_variables) instead of being reported
   * as fresh internal nonterminals. Useful when the solution needs to be
   * re-displayed/re-parsed in terms of the original variable names.
   *
   * @details This is the primary programmatic entry point for querying the
   * interpreter (as opposed to @ref eval, which just prints results).
   * After this call returns, the interpreter's state is as if the query had
   * never run, aside from any explicit, non-backtracked side effects such as
   * I/O or sabotajes by user code in *meta-ops*.
   *
   * @code{.cpp}
   * dictionary vd;
   * prolog_parser p;
   * const object goal = p.parse_expr(pl.symbols(), vd, "append(X, Y, [1,2,3])");
   *
   * pl.make_true(vd, goal, [&](const interpreter::solution &sol) {
   *   std::cout << "X = " << pl.dump(sol.at("X"))
   *             << ", Y = " << pl.dump(sol.at("Y")) << std::endl;
   * });
   * @endcode
   */
  void
  make_true(const dictionary &vardict, object_view expr,
            const std::function<void(const solution &)> &cont,
            bool recover_vars = false);

  /**
   * @brief Parse and find every solution of a goal, invoking a callback for
   * each
   * @param expr Goal clause, e.g. `"append(X, Y, [1,2,3])"`.
   * @param cont Invoked once per solution with a @ref solution snapshot of
   * all named variable bindings (keyed by the variable names as written in
   * @p expr).
   *
   * @details A parsing convenience wrapper around the
   * `(vardict, expr, cont, recover_vars)` overload above: it parses @p expr
   * with a fresh @ref prolog_parser and solves the resulting goal.
   *
   * @code{.cpp}
   * pl.make_true("member(X, [a, b, c])", [&](const interpreter::solution &sol) {
   *   std::cout << "X = " << pl.dump(sol.at("X")) << std::endl;
   * });
   * // prints: X = a
   * //         X = b
   * //         X = c
   * @endcode
   */
  void
  make_true(std::string_view expr,
            const std::function<void(const solution &)> &cont);

  void
  make_true(std::string_view expr);

  /**
   * @brief Low-level goal evaluation entry point 
   *
   * @details Drives @p expr to its success against the given @p rt *sprout*,
   * invoking @p cont on every solution, without establishing its own choice
   * point or unwinding afterwards -- the caller is fully responsible for
   * choice-point/heap management. This is a building block used by native
   * meta-ops (see @ref meta_op_handle) to recursively invoke sub-goals, e.g.
   * implementing `once/1`:
   * @code{.cpp}
   * pl.add_meta_op("once", [&pl](runtime &rt, size_t argc, object_iterator argv,
   *                              continuation &cont) {
   *   basic_decoder dc;
   *   const object_view goal = dc.decode_object(argv);
   *   barrier cp;
   *   rt.push_choice_point(&cp);
   *   pl.make_true(rt, goal, [cont, &cp](runtime &rt) { rt.cut(&cp); cont(rt); });
   *   rt.pop_choice_point(&cp);
   * });
   * @endcode
   */
  [[nodiscard]] continuation
  make_true(runtime &rt, object_view expr, continuation cont)
  { return _make_true(rt, 0, expr.begin(), m_query->cp, std::move(cont)); }

  private:
  // void
  // _exhaust(runtime &rt, continuation cont)
  // {
  //   while (cont)
  //     cont = cont(rt, 0, 0, 0, 0).reinterpret<continuation::signature>();
  // }

  [[nodiscard]] continuation
  _make_true(runtime &rt, size_t _, object_iterator e, barrier *clause,
             const continuation& cont);

  [[nodiscard]] continuation
  _make_true__and(runtime &rt, size_t i, object_iterator eit,
                  barrier *clause, const continuation &cont);

  [[nodiscard]] continuation
  _make_true__or(runtime &rt, size_t i, object_iterator eit,
                 barrier *clause, const continuation &cont);

  [[nodiscard]] continuation
  _make_true__if(runtime &rt, size_t _, object_iterator eit,
                 barrier *clause, const continuation &cont);

  [[nodiscard]] continuation
  _make_true__predicate(runtime &rt, size_t _, object_iterator e,
                        barrier *clause, const continuation &cont);

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
