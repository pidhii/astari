#pragma once

#include <cassert>
#include <deque>
#include <type_traits>
#include <utility>


#ifdef __clang__
# define TAILCALL return
#elif defined(__GNUC__) || defined(__GNUG__)
# define TAILCALL [[gnu::musttail]] return
#else
# define TAILCALL return
#endif


/**
 * @brief Utility object for resolving recursive deletions
 */
struct push_deleter {
  static push_deleter &
  instance()
  {
    static push_deleter self;
    return self;
  }

  void
  push_delete(void *p, void (*dtor)(void *))
  {
    m_queue.emplace_back(p, dtor);
    if (m_queue.size() > 1)
      return; // You were not the one to kick off this avalanche
    while (not m_queue.empty())
    {
      auto [p, dtor] = m_queue.front();
      dtor(p);
      m_queue.pop_front();
    }
  }

  private:
  push_deleter() = default;
  push_deleter(const push_deleter &) = delete;
  push_deleter(push_deleter &&) = delete;
  void operator = (const push_deleter &) = delete;
  void operator = (push_deleter &&) = delete;

  private:
  std::deque<std::pair<void *, void (*)(void *)>> m_queue;
};


template <typename Sig>
struct tcfunction;

template <typename RetT, typename ...ArgsT>
struct tcfunction<RetT(ArgsT...)> {
  tcfunction() = default;

  tcfunction(const tcfunction &other)
  : m_clos {other._copy_clos()},
    m_func {other.m_func},
    m_dtor {other.m_dtor},
    m_copy {other.m_copy}
  { }

  tcfunction(tcfunction &&other)
  : m_clos {other.m_clos},
    m_func {other.m_func},
    m_dtor {other.m_dtor},
    m_copy {other.m_copy}
  { other.m_clos = nullptr; }

  tcfunction &
  operator = (const tcfunction &other)
  {
    void *clos = other._copy_clos();
    if (m_clos)
      m_dtor(m_clos);
    m_clos = clos;
    m_func = other.m_func;
    m_dtor = other.m_dtor;
    m_copy = other.m_copy;
    return *this;
  }

  tcfunction &
  operator = (tcfunction &&other)
  {
    if (this == &other)
      return *this;
    if (m_clos)
      m_dtor(m_clos);
    m_clos = other.m_clos;
    m_func = other.m_func;
    m_dtor = other.m_dtor;
    m_copy = other.m_copy;
    other.m_clos = nullptr;
    return *this;
  }

  template <typename Lambda>
  static tcfunction
  from_lambda(Lambda &&lambda)
  {
    using lambda_type = std::remove_cvref_t<Lambda>;

    tcfunction cont;
    cont.m_clos = new lambda_type {std::forward<Lambda>(lambda)};
    cont.m_func = [](void *p, ArgsT ...args) -> RetT {
      lambda_type *self = reinterpret_cast<lambda_type*>(p);
      TAILCALL (*self)(args...);
    };
    cont.m_dtor = [](void *p) {
      lambda_type *self = reinterpret_cast<lambda_type *>(p);
      delete self;
    };
    cont.m_copy = [](void *p) -> void * {
      lambda_type *self = reinterpret_cast<lambda_type *>(p);
      lambda_type *copy = new lambda_type {*self};
      return copy;
    };
    return cont;
  }

  ~tcfunction()
  {
    if (m_clos)
    {
      push_deleter::instance().push_delete(m_clos, m_dtor);
      m_clos = nullptr;
    }
    // if (m_clos)
    //   m_dtor(m_clos);
  }

  operator bool () const noexcept
  { return m_clos != nullptr; }

  RetT
  operator () (ArgsT ...args) const noexcept
  {
    assert(m_clos != nullptr);
    return m_func(m_clos, args...);
  }

  RetT
  call_tc(ArgsT... args) const
  {
    assert(m_clos != nullptr);
    TAILCALL m_func(m_clos, args...);
  }

  private:
  void*
  _copy_clos() const noexcept
  {
    if (m_clos and m_copy)
      return m_copy(m_clos);
    else
      return nullptr;
  }

  private:
  void *m_clos = nullptr;
  RetT (*m_func)(void *, ArgsT...) = nullptr;
  void (*m_dtor)(void *) = nullptr;
  void *(*m_copy)(void *) = nullptr;
};

#undef TAILCALL