#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>


class dictionary {
  public:
  uint64_t
  operator [] (std::string_view str)
  {
    const int newid = m_ids.size();
    const auto [it, emplaced] = m_ids.emplace(str, newid);
    if (emplaced)
      m_names[newid] = it->first;
    return it->second;
  }

  std::string_view
  operator [] (uint64_t id) const
  { return m_names.at(id); }

  void
  clear() noexcept
  { m_ids.clear(); m_names.clear(); }

  private:
  std::unordered_map<std::string, uint64_t> m_ids;
  std::unordered_map<uint64_t, std::string_view> m_names;
};
