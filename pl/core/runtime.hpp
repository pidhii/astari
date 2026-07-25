/**
 * @file runtime.hpp
 * @brief Interpreter runtime
 */
#pragma once

#include "pl/misc/object_allocator.hpp"
#include "pl/obj/object.hpp"
#include "pvector/pvector.hpp"
#include "ualloc/ualloc.hpp"
#include "utl/rooted_forest.hpp"

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
   * @brief Cut all choice points since after the given one
   * @param tgt The oldest choice point to be cut.
   */
  void
  cut(barrier *tgt);

  /**
   * @brief Pop the choice point
   */
  void
  pop_choice_point(barrier *cp);

  /**
   * @brief UnWind-Unless-Cut
   * @details Checks if @p cp was cut and calls either @ref unwind (if not cut),
   * or @ref pop_choice_point (if cut). Meant for use in loops setting choice
   * points, i.e:
   * @code{.cpp}
   * for (...)
   * {
   *   rt.push_choice_point(&cp);
   *   ...
   *   if (rt.uwuc(&cp))
   *     return;
   * }
   * @endcode
   * @return `true` if @p cp was cut, `false` otherwise.
   */
  [[nodiscard]] bool
  uwuc(barrier *cp);

  /**
   * @brief Test if two variables are bound/unified
   */
  [[nodiscard]] bool
  bound(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.find(lhsid).second == m_dsf.find(rhsid).second; }

  query_state *
  query() const noexcept
  { return m_query; }

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
