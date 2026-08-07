#pragma once

#include <filesystem>


struct pwd_saver {
  pwd_saver(const std::filesystem::path &pwd): m_pwd {pwd} { }
  pwd_saver(): pwd_saver(std::filesystem::current_path()) { }
  pwd_saver(const pwd_saver &) = delete;
  pwd_saver(pwd_saver &&other): m_pwd {other.m_pwd} { other.m_pwd.clear(); }

  ~pwd_saver()
  {
    if (not m_pwd.empty())
      std::filesystem::current_path(m_pwd);
  }

  void
  release() noexcept
  { m_pwd.clear(); }

  private:
  std::filesystem::path m_pwd;
};


static pwd_saver
cd(const std::filesystem::path &dir)
{
  pwd_saver pwd {std::filesystem::current_path()};
  std::filesystem::current_path(dir);
  return pwd;
}