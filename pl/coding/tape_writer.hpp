#pragma once

#include "basic_encoder.hpp"

#include "../obj/object.hpp"
#include "../dictionary.hpp"


template <typename... Fields>
struct multiterm_proxy {
  std::string_view name;
  std::tuple<Fields...> fields;
};

struct monoterm_proxy {
  std::string_view name;
};

template <typename... Fields>
multiterm_proxy<std::remove_cvref_t<Fields>...>
term(std::string_view name, Fields &&...fields)
{ return {name, std::make_tuple(fields...)}; }

inline monoterm_proxy
term(std::string_view name)
{ return {name}; }


template <typename OutIter>
class tape_writer: public basic_encoder {
  public:
  tape_writer(OutIter it, dictionary &dict): m_dict (dict), m_oit {it} { }

  template <typename T>
  tape_writer&
  operator << (T x)
  { *m_oit++ = encode(x); return *this; }

  template <typename ...Fields>
  tape_writer&
  operator << (const multiterm_proxy<Fields...> &sh)
  {
    const uint64_t id = m_dict[sh.name];
    *m_oit++ = encode(term_header {id, sizeof...(Fields)});
    _encode_fields<0>(sh.fields);
    return *this;
  }

  tape_writer&
  operator << (const monoterm_proxy &sh)
  {
    const uint64_t id = m_dict[sh.name];
    *m_oit++ = encode(term_header {id, 0});
    return *this;
  }

  private:
  template <size_t I, typename ...Fields>
  void
  _encode_fields(const std::tuple<Fields...> &fields)
  {
    if constexpr (I < sizeof...(Fields))
    {
      *this << std::get<I>(fields);
      _encode_fields<I + 1>(fields);
    }
  }

  private:
  dictionary &m_dict;
  OutIter m_oit;
};


template <typename SeqContainer>
auto
encode_to(SeqContainer &container, dictionary &dict)
{ return tape_writer(std::back_inserter(container), dict); }
