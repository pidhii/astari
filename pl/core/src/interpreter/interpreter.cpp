#include "interpreter.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/obj/object.hpp"
#include "utl/resolve_path.hpp"

#include <filesystem>
#include <stdexcept>


interpreter::interpreter()
{
  auto require = [this] (std::string_view name, meta_symbol op) {
    if (m_symdict[name] != op)
      throw std::logic_error {"failed to register symbol"};
  };
  require(",", op_and);
  require(";", op_or);
  require("if", op_if);
  require("fail", op_fail);
}


bool
interpreter::has_meta_op(size_t id) const noexcept
{ return m_metaops.contains(id); }


bool
interpreter::has_predicate(size_t id) const noexcept
{
  for (const auto &[w, _] : m_predicates)
  {
    term_header hdr;
    basic_decoder().decode(w, hdr);
    if (hdr.id == id)
      return true;
  }
  return false;
}


bool
interpreter::has(size_t id) const noexcept
{ return has_predicate(id) or has_meta_op(id); }


void
interpreter::add_predicate(object_view signobj, object_view bodyobj)
{
  // if (bodyobj.empty())
  //   std::clog << "[add_predicate] add predicate: " << dump(signobj) << "." << std::endl;
  // else
  //   std::clog << "[add_predicate] add predicate: " << dump(signobj) << " :- "
  //             << dump(bodyobj) << std::endl;
  assert(not signobj.empty());
  if (not is_term(signobj[0]))
    throw std::runtime_error {"invalid predicate signature: " + dump(signobj)};
  const size_t id = basic_decoder().decode_term_header(signobj[0]).id;
  if (has_meta_op(id))
  {
    throw std::runtime_error {std::format(
        "predicate name already used for meta operator ({})", m_symdict[id])};
  }
  m_predicates[signobj[0]].emplace_back(signobj, bodyobj);
}


void
interpreter::add_predicate(object_view signobj)
{
  // std::clog << "[add_predicate] add statement: " << dump(signobj) << std::endl;
  add_predicate(signobj, {});
}


void
interpreter::add_meta_op(std::string_view name, const meta_op_handle &handle)
{
  const size_t id = m_symdict[name];
  if (has(id))
  {
    throw std::runtime_error {std::format(
        "duplicate names for meta operators are not allowed ({})", name)};
  }
  m_metaops.emplace(id, handle);
}


void
interpreter::ensure_loaded(std::string_view path_)
{
  namespace fs = std::filesystem;

  const fs::path path {path_};

  std::vector<fs::path> searchnames;
  if (path.extension() == ".plo" or path.extension() == "pl")
    searchnames.push_back(path);
  else
  {
    searchnames.emplace_back(path.string() + ".plo");
    searchnames.emplace_back(path.string() + ".pl");
  }

  for (const fs::path &p : searchnames)
  {
    try
    {
      const fs::path fullpath =
          resolve_path(p, m_impordirs.begin(), m_impordirs.end());
      if (not m_imports.emplace(fullpath).second)
        return; // File was already loaded

      if (fullpath.extension() == ".pl")
        load_file(fullpath.c_str());
      else if (fullpath.extension() == ".plo")
        load_objfile(fullpath.c_str());
      else
        assert(not "unreachable code" );
    }
    catch (...)
    { continue; }
  }

  throw std::runtime_error {
      std::format("failed to resolve library ({})", path_)};
}