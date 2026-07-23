#pragma once

#include "pl/misc/object_allocator.hpp"
#include "pl/obj/object.hpp"
#include "pvector/pvector.hpp"
#include "ualloc/ualloc.hpp"
#include "utl/rooted_forest.hpp"

#include "radixtrees/pradix256dense.hpp"

#include <optional>
#include <unordered_map>


using varnamespace = std::unordered_map<size_t, size_t>;


struct barrier {
  size_t varbar;
  size_t *uwbar;
  barrier *prev;
  word_t *hpbar;
  uint8_t cut;
  uint8_t noreclaim;
};

#define TERM_HEAP_SIZE (5 * (2 << 20))
static constexpr size_t unwind_heap_length = 2 << 20;
extern size_t unwind_heap[];
extern size_t *unwind_p;
extern barrier *choice_point;
extern word_t term_heap[];
extern word_t *heap_p;


class runtime: public object_allocator {
  public:
  runtime() = default;
  [[deprecated("avoid this")]] runtime(runtime &other) = default;
  [[deprecated("avoid this")]] runtime(const runtime &other) = default;
  [[deprecated("avoid this")]] runtime& operator = (runtime &other) = default;
  [[deprecated("avoid this")]] runtime& operator = (const runtime &other) = default;

  object_view
  adopt_g(varnamespace &ns, object_view in);

  object_view
  adopt_g(object_view in)
  { varnamespace ns; return adopt_g(ns, in); }

  object_view
  adopt_hp(varnamespace &ns, object_view in);

  object_view
  adopt_hp(object_view in)
  { varnamespace ns; return adopt_hp(ns, in); }

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
      *unwind_p++ = imut;
  }

  void
  assign_uw(size_t varid, object_iterator value, barrier bar);

  void
  push_choice_point(barrier *bar) const noexcept;

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

  [[nodiscard]] bool
  bound(size_t lhsid, size_t rhsid) noexcept
  { return m_dsf.find(lhsid).second == m_dsf.find(rhsid).second; }

  private:
  void
  _adopt(varnamespace &ns, object_view in, word_t *out);

  template <typename OutputIter>
  void
  _reconstruct(object_iterator in, OutputIter out, size_t n);

  private:
  pidhii::pradix256dense<object_iterator> m_assignments;
  template <typename T>
  using pvector = pidhii::pvector<T, 8, pidhii::static_uniform_allocator<T>>;
  rooted_forest<pvector> m_dsf;
};


void
normalize(object_view in, word_t *out);
