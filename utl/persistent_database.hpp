#pragma once

#include <cassert>
#include <memory>
#include <stdexcept>
#include <unordered_map>


template <typename Key, typename T, template <typename> typename Alloc>
class persistent_database {
  struct cons_node {
    cons_node(const T &val, std::shared_ptr<cons_node> next)
    : value {val}, next {next}
    { }

    T value;
    std::shared_ptr<cons_node> next;
  };
  using cons_list = std::shared_ptr<cons_node>;
  using table_value_type = cons_list;
  using table_type = std::unordered_map<Key, cons_list>;

  public:
  persistent_database(Alloc<T> alloc = Alloc<T> {})
  : m_db {std::allocate_shared<table_type>(m_alloc)},
    m_alloc {alloc}
  { }

  struct const_iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = ssize_t;
    using reference = const T &;
    using pointer = const T *;

    explicit const_iterator(cons_list list): m_list {list.get()} { }
    const_iterator() = default;
    const_iterator(const const_iterator &) = default;
    const_iterator & operator = (const const_iterator &) = default;

    bool
    operator == (const const_iterator &other) const noexcept
    { return m_list == other.m_list; }

    bool
    operator != (const const_iterator &other) const noexcept
    { return m_list != other.m_list; }

    reference
    operator * () const noexcept
    { return m_list->value; }

    pointer
    operator -> () const noexcept
    { return &m_list->value; }

    const_iterator &
    operator ++ () noexcept
    { m_list = m_list->next.get(); return *this; }

    const_iterator
    operator ++ (int) noexcept
    { const_iterator tmp { m_list }; m_list = m_list->next.get(); return tmp; }

    operator const cons_node * () const noexcept
    { return m_list; }

    private:
    const cons_node *m_list;
  };
  static_assert(std::forward_iterator<const_iterator>);

  [[nodiscard]] const_iterator
  begin(const Key &key) const noexcept
  {
    assert(m_db != nullptr);
    if (const auto dbit = m_db->find(key); dbit != m_db->end())
      return const_iterator {dbit->second};
    else
      return const_iterator {nullptr};
  }

  [[nodiscard, gnu::pure]] const_iterator
  end(const Key &_) const noexcept
  { return const_iterator {nullptr}; }

  [[nodiscard]] table_type::const_iterator
  begin() const noexcept
  {
    assert(m_db != nullptr);
    return m_db->begin();
  }

  [[nodiscard]] table_type::const_iterator
  end() const noexcept
  {
    assert(m_db != nullptr);
    return m_db->end();
  }

  struct recovery {
    cons_list old_list;
    void detach() { old_list = nullptr; }
  };

  recovery
  push_front(const Key &key, const T &val)
  {
    assert(m_db != nullptr);
    if (m_db.use_count() > 1) // copy-on-write
      m_db = std::allocate_shared<table_type>(m_alloc, *m_db);

    // in-place update
    assert(m_db.use_count() == 1);
    cons_list &list = m_db->emplace(key, nullptr).first->second;
    recovery recov {list};
    list = _cons(val, list);
    return recov;
  }

  recovery
  push_back(const Key &key, const T &val)
  {
    assert(m_db != nullptr);
    if (m_db.use_count() > 1) // copy-on-write
      m_db = std::allocate_shared<table_type>(m_alloc, *m_db);

    // in-place update
    assert(m_db.use_count() == 1);
    cons_list &list = m_db->emplace(key, nullptr).first->second;
    const recovery recov {list};
    list = _append(val, list);
    return recov;
  }

  recovery
  erase(const Key &key, const_iterator rmit)
  {
    assert(m_db != nullptr);
    if (m_db.use_count() > 1)
      m_db = std::allocate_shared<table_type>(m_alloc, *m_db);

    const auto dbit = m_db->find(key);
    if (dbit == m_db->end())
      throw std::out_of_range {"persistent_database::erase: no such key"};

    cons_list &list = dbit->second;
    const recovery recov {list};
    list = _erase(rmit, list);
    return recov;
  }

  void
  rollback(const Key &key, recovery &&recov)
  {
    assert(m_db != nullptr);
    if (m_db.use_count() > 1) // copy-on-write
      m_db = std::allocate_shared<table_type>(m_alloc, *m_db);

    // in-place update
    assert(m_db.use_count() == 1);
    m_db->at(key) = std::move(recov.old_list);
  }

  private:
  static bool
  _null(const cons_list &list) noexcept
  { return list == nullptr; }

  cons_list
  _cons(const T &val, const cons_list &list) const
  { return std::allocate_shared<cons_node>(m_alloc, val, list); }

  cons_list
  _append(const T &val, const cons_list &list) const
  {
    if (_null(list))
      return _cons(val, list);
    else
    {
      cons_list tail = _append(val, list->next);
      return _cons(list->value, tail);
    }
  }

  cons_list
  _erase(const cons_node *x, const cons_list &list) const
  {
    if (x == list.get())
      return list->next;
    else
    {
      cons_list tail = _erase(x, list->next);
      return _cons(list->value, tail);
    }
  }

  private:
  std::shared_ptr<table_type> m_db;
  Alloc<cons_node> m_alloc;
};