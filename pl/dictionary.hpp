#pragma once

#include "pl/coding/basic_decoder.hpp"
#include "pl/coding/basic_encoder.hpp"
#include "pl/obj/object.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>


class dictionary {
  public:
  uint64_t
  operator [] (std::string_view str)
  {
    if (str == "_")
      return m_cnt++;

    const auto [it, emplaced] = m_ids.emplace(str, m_cnt);
    if (emplaced)
    {
      m_names[m_cnt] = it->first;
      m_cnt++;
    }
    return it->second;
  }

  std::string_view
  operator [] (uint64_t id) const
  {
    const auto it = m_names.find(id);
    if (it != m_names.end())
      return it->second;
    else
      return "_";
  }

  dictionary
  subscope() const
  {
    dictionary result;
    result.m_cnt = m_cnt;
    return result;
  }

  void
  clear() noexcept
  { m_ids.clear(); m_names.clear(); }

  private:
  uint64_t m_cnt {0};
  std::unordered_map<std::string, uint64_t> m_ids;
  std::unordered_map<uint64_t, std::string_view> m_names;

  friend struct object_file;
};


// TODO: move to cpp file
static void
transfer_symbols(dictionary &from, dictionary &to, object &obj)
{
  basic_encoder ec;
  basic_decoder dc;

  for (word_t &word : obj)
  {
    if (is_term(word))
    {
      term_header hdr;
      dc.decode(word, hdr);
      const size_t newid = to[from[hdr.id]];
      word = ec.encode(term_header(newid, hdr.arity));
    }
  }
}
