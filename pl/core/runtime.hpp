/**
 * @file runtime.hpp
 * @brief Interpreter runtime
 */
#pragma once

#include "predicate_entry.hpp"

#include "pl/dictionary.hpp"
#include "pl/misc/object_allocator.hpp"
#include "pl/obj/object.hpp"
#include "pvector/pvector.hpp"
#include "ualloc/ualloc.hpp"
#include "utl/persistent_database.hpp"
#include "utl/rooted_forest.hpp"
#include "utl/tcfunction.hpp"

#include <optional>
#include <unordered_map>


/**
 * @ingroup core
 * @brief Variables namespace
 * @details Mapping from *local* IDs to *global* IDs:
 * - *local* IDs refer to nonterminals inside a privately owned object;
 * - *global* IDs refer to runtime variables (subject to bindings and
 *   assignments).
 */
using varnamespace = std::unordered_map<size_t, size_t>;


/**
 * @ingroup core
 * @brief Choice point record
 * @details Choice points form a (singly-) linked list that is managed
 * by a @ref runtime through methods like @ref runtime::push_choice_point and
 * etc. The initial choice point is to be set by @ref interpreter. See
 * documentation of related methods in @ref runtime for more info.
 */
struct barrier {
  size_t varbar;
  size_t *uwbar;
  barrier *prev;
  word_t *hpbar;
  uint8_t cut;
  uint8_t noreclaim;
};


/**
 * @ingroup core
 * @brief Prolog *success continuation*
 * @details A continuation is invoked with the current @ref runtime *sprout*
 * every time the goal it was attached to succeeds. Returning from a
 * continuation without throwing means "and now try to find another
 * solution" -- backtracking into whatever choice points remain further up
 * the call chain. This is the standard *continuation-passing style* used
 * throughout the interpreter to implement conjunction, disjunction,
 * if-then-else and predicate calls without growing the native C++ call
 * stack (tail-calls are used internally, see @ref TAILCALL).
 */
// class continuation {
//   private:
//   void *m_clos;
//   void (*m_func)(void *,runtime &, size_t, object_iterator, continuation &);
// };
// using continuation =
//     std::function<void(runtime &, size_t, object_iterator, barrier *, void *)>;
class runtime;
#define CONT_ARGS runtime &rt, size_t, object_iterator, barrier *, void*
using continuation = tcfunction<tcfunction<void()>(CONT_ARGS)>;


/**
 * @ingroup core
 * @brief Query state
 * @details Interpreter state/registers shared by *sprouts* (@ref runtime
 * instances) of a query.
 */
struct query_state {
  size_t *unwind_p; /**< @brief Retractable heap to record information for backtracking */
  word_t *heap_p; /**< @brief Retractable heap for data allocations during the query */
  word_t *heap_e; /**< @brief End of the data heap */
  barrier *cp; /**< @brief Latest choice point */
};


/**
 * @ingroup core
 * @brief Prolog runtime interface
 *
 * @details
 * This class implements the evaluation environment along with operation
 * primitives to implement Prolog @ref interpeter. Contents-wise, runtime
 * represents a *sprout* or *branch* of a query. Multiple such *sprouts* can
 * coexist simultaniously and ran asynchronously.
 *
 * **Valid State**  
 * Valid instances of runtime must copy-constructed from either
 * @ref interpreter instances or other valid runtime instances.
 * Default-constructed instance of runtime must not be used untill initialized.
 * The runtime instance will become initialized as soon as it is assigned with
 * (either rvalue or lvalue reference of) other initialized runtime.
 */
class runtime: public object_allocator {
  public:
  /** @cond DETAILS */
  using dynamic_database_type =
      persistent_database<word_t, predicate_entry,
                          pidhii::static_uniform_allocator>;
  struct recovery { word_t key; dynamic_database_type::recovery entrecov; };
  using dyn_variant_iterator = dynamic_database_type::const_iterator;
  /** @endcond  */

  void
  print_dynamic_database(dictionary &symbols) const;

  /**
   * @name Object relocation and linking
   * @details Relocate objects from local storage onto interpreter-managed
   * memory pools and "link" them into the runtime.
   * @{
   */
  /**
   * @brief Relocate and link an object via a static (global) memory section
   * @details Uses memory pool inherited from @ref object_allocator. This memory
   * is retained and unchanged for the lifetime of the parent @ref interpreter
   * instance.
   */
  object_view
  adopt_g(varnamespace &ns, object_view in);

  object_view
  adopt_g(object_view in)
  { static varnamespace ns; ns.clear(); return adopt_g(ns, in); }

  object_view
  adopt_hp(varnamespace &ns, object_view in);

  object_view
  adopt_hp(object_view in)
  { static varnamespace ns; ns.clear(); return adopt_hp(ns, in); }

  object_view
  adopt_hp_n(size_t base, object_view in);

  object_view
  adopt_clause_hp(dyn_variant_iterator it);
  /** @} */

  size_t
  n_vars() const noexcept
  { return m_dsf.size(); }

  void
  make_n_vars(size_t n) noexcept
  { m_dsf.make_n_sets(n); }

  object
  reconstruct(object_iterator in);

  object
  reconstruct(object_view in);

  void
  reconstruct(object_iterator in, word_t *out);

  std::optional<object_iterator>
  dereferencer(size_t &varid);

  std::optional<object_iterator>
  dereference(size_t varid)
  { return dereferencer(varid); }

  object_iterator
  reduce(object_iterator x);

  object_view
  reduce(object_view x);

  bool
  match(object_view lhs, object_view rhs);

  void
  bind(size_t lhsid, size_t rhsid) noexcept
  { m_dsf.join(lhsid, rhsid); }

  void
  assign(size_t varid, object_iterator value);

  void
  bind_uw(size_t lhsid, size_t rhsid, barrier bar) noexcept
  {
    const size_t imut = m_dsf.join(lhsid, rhsid);
    assert(imut != -1ull);
    if (imut < bar.varbar)
      *m_query->unwind_p++ = imut;
  }

  void
  assign_uw(size_t varid, object_iterator value, barrier bar);

  /**
   * @brief Create a choice point
   * @details Create a choice point and push it onto the global stack. Bindings
   * made after establishement of the choice point can be undone using
   * @ref unwind. Foreign parties will have access to the stack of choice
   * points, and can thus use it as means of communication, e.g.: request a
   * *cut* or lock the heap memory.
   * @note Handling requests for *cut* is a responsibility of the owner of the
   * given choice point.
   * parties will have access to this choice point and may use it as means of
   */
  void
  push_choice_point(barrier *cp) const noexcept;

  /**
   * @brief Unwind variable bindings, reclaim memory and pop the choice point
   * @details The main backtracking routine. Consider using a much cheaper
   * @ref pop_choice_point instead unless another goal is to be executed
   * explicitly right after and unwinding is required. It is preferable to have
   * few large unwindings versus many small ones.
   * @note Memory reclamation might be prevented by the foreign party, e.g. a
   * continuation suspended and currently awaiting for reentrace.
   */
  void
  unwind(barrier *cp);

  /**
   * @brief Cut all choice points since after the given one (inclusive)
   * @param tgt The oldest choice point to be cut.
   */
  void
  cut(barrier *tgt);

  /**
   * @brief Cut all choice points since after the given one (exclusive)
   * @param tgt The latest choice point to keep uncut.
   */
  void
  cut_exc(barrier *tgt);

  void
  lock_heap_exc(barrier *tgt)
  {
    barrier *cp = query()->cp;
    while (cp != tgt)
    {
      if (cp->noreclaim)
        break; // rest was already marked (TODO: find a way to assure this)
      cp->noreclaim = true;
      cp = cp->prev;
    }
  }

  /**
   * @brief Pop the choice point
   */
  void
  pop_choice_point(barrier *cp);

  /**
   * @brief Drive Until Cut
   * @return `true` if @p cp was cut, `false` otherwise.
   */
  [[nodiscard]] bool
  driveuc(barrier *cp, continuation &cc)
  {
    while (true)
    {
      if (cp->cut)
      {
        pop_choice_point(cp);
        return true;
      }
      if (cc)
        cc = cc(*this, 0, 0, 0, 0).reinterpret<continuation::signature>();
      else
        return false;
    }
  }

  void
  exhaust(continuation cc)
  {
    while (cc)
      cc = cc(*this, 0, 0, 0, 0).reinterpret<continuation::signature>();
  }

  /**
   * @brief Test if two variables are bound/unified
   */
  [[nodiscard]] bool
  bound(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.find(lhsid).second == m_dsf.find(rhsid).second; }

  query_state *
  query() const noexcept
  { return m_query; }

  /**
   * @name Dynamic database interface
   * @{
   */
  /**
   * @brief Add a predicate to dynamic database
   * @details Addition of the clause @p sign :- @p body to the database BEFORE
   * all other clauses for the predicate associated to @p sign.
   * @return Recovery object for a table associated with @p sign.
   * @see @ref runtime::retract
   * @warning It is a responsibility of a caller to verify that the predicate is
   * dynamic (e.g., using @ref interpreter::is_dynamic).
   */
  recovery
  asserta_dyn(object_view sign, object_view body = {});

  /**
   * @brief Add a predicate to dynamic database
   * @details Addition of the clause @p sign :- @p body to the database AFTER
   * all other clauses for the predicate associated to @p sign. All other
   * database references become invalidated untill the added predicate is
   * removed from the database with @ref retract.
   * @return Recovery object for a table associated with @p sign.
   * @see @ref runtime::retract
   * @warning It is a responsibility of a caller to verify that the predicate is
   * dynamic (e.g., using @ref interpreter::is_dynamic).
   */
  recovery
  assertz_dyn(object_view sign, object_view body = {});

  /**
   * @brief Iterator over associated predicates
   * @return <implementation-details>
   * @see @ref variant_sign, @ref variant_body, @ref adopt_clause_hp
   */
  dyn_variant_iterator
  variants_begin(word_t signkey)
  { return m_dyndb.begin(signkey); }

  /**
   * @brief Sentinel for the predicates iterator
   * @return <implementation-details>
   */
  dyn_variant_iterator
  variants_end()
  { return m_dyndb.end(0); }

  /**
   * @brief Access the signature of a predicate clause pointed-to by @p it
   * @return Clause signature.
   */
  static object_view
  variant_sign(dyn_variant_iterator it)
  { return it->sign; }

  /**
   * @brief Access body of the predicate clause pointed-to by @p it
   * @return Clause body, if present; otherwise an empty view if clause
   * resembles a statement (no body).
   */
  static object_view
  variant_body(dyn_variant_iterator it)
  { return it->body; }

  recovery
  retract_dyn(word_t signkey, dyn_variant_iterator it);

  /**
   * @brief Recover a table for dynamic predicate.
   * @details Rollback the state of a table that was previously altered (e.g.,
   * @ref assert[a,z]_dyn, @ref retract_dyn). Single recovery object must only be
   * used once. This function complements @ref unwind in that it implements a
   * fast (but necessarily explicit) backtracking for a *single-sprout*
   * execution mode whilest respecting the shared states in a *multiple-sprouts*
   * regime.
   * @code{.cpp}
   * auto save = rt.asserta_dyn(clause_sign, clause_body);
   * // ... proceeed with query ...
   * rt.recover(save); // "unwind"
   * @endcode
   */
  void
  recover(recovery &&recovery);
  /** @} */


  private:
  void
  _adopt(varnamespace &ns, object_view in, word_t *out);

  void
  _adopt_n(size_t base, object_view in, word_t *out);

  template <typename OutputIter>
  void
  _reconstruct(object_iterator in, OutputIter out, size_t n);

  private:
  template <typename T>
  using pvector = pidhii::pvector<T, 8, pidhii::static_uniform_allocator<T>>;
  rooted_forest<pvector> m_dsf;

  protected:
  query_state *m_query;
  dynamic_database_type m_dyndb;
};


/**
 * @ingroup core
 * @brief Project an object onto its equivalence class
 * @details Rename variables to base 0 and zero any cached fields. Projections
 * of two *equivalent* objects (that are not recursive) are *equal*.
 * @return Number of nonterminals (equal to the largest id of inserted
 * nonterminal plus one).
 */
size_t
normalize(object_view in, word_t *out);

/**
 * @ingroup core
 * @brief Reentrant version of @ref normalize
 * @details To normalize multiple objects, thread a common @p ns through the
 * calls and use the return value of a previous call as @p base for the next
 * call, i.e.:
 * @code{.cpp}
 * varnamespace ns;
 * size_t base = 0;
 * base = normalize_r(ain, aout, ns, base);
 * base = normalize_r(bin, bout, ns, base);
 * ...
 * @endcode
 */
size_t
normalize_r(object_view in, word_t *out, varnamespace &ns, size_t base = 0);
