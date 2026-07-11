#include "syntax_parser.hpp"


token
syntax_parser::parse()
{
  for (auto &g : m_grammars)
  {
    if (g->assoc > 0)
    {
      token_iterator s = m_data.end() - g->size;
      token_iterator e = m_data.end();
      while (s >= m_data.begin())
      {
        token result;
        if (g->apply(s, result))
        {
          m_data.erase(s, e);
          m_data.insert(s, result);
          return parse();
        }
        s--;
        e--;
      }
    }
    else
    {
      token_iterator s = m_data.begin();
      token_iterator e = m_data.begin() + g->size;
      while (e <= m_data.end())
      {
        token result;
        if (g->apply(s, result))
        {
          m_data.erase(s, e);
          m_data.insert(s, result);
          return parse();
        }
        s++;
        e++;
      }
    }
  }

  if (m_data.size() != 1)
    throw std::runtime_error {"parsing did not terminate"};
  token result = std::move(m_data[0]);
  m_data.clear();
  return result;
}
