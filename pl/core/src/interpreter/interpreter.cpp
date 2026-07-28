#include "interpreter.hpp"

#include "pl/coding/basic_decoder.hpp"
#include "pl/misc/term_utils.hpp"
#include "pl/obj/object.hpp"
#include "pl/parse/prolog_parser.hpp"
#include "utl/resolve_path.hpp"

#include <filesystem>
#include <stdexcept>


#define TERM_HEAP_SIZE (5 * (2 << 20))
#define UNWIND_HEAP_SIZE (2 << 20)


interpreter::interpreter()
: m_unwind_heap {new size_t[UNWIND_HEAP_SIZE]},
  m_term_heap {new word_t[TERM_HEAP_SIZE]}
{
  auto require = [this] (std::string_view name, meta_symbol op) {
    if (m_symdict[name] != op)
      throw std::logic_error {"failed to register symbol"};
  };
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//                  THESE REQUIRES MUST BE IN SYNC WITH THE ORDER
//                          OF ENUMS IN THE HEADER
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  require(",",    op_and);
  require(";",    op_or);
  require("if",   op_if);
  require("fail", op_fail);
  require("!",    op_cut);
  require("+",    op_plus);
  require("-",    op_minus);
  require("*",    op_mul);
  require("/",    op_div);
  require("//",   op_divdiv);
  require("sqrt", op_sqrt);
  require(":-",   op_penis);

  m_query = reinterpret_cast<query_state *>(
      allocate((sizeof(query_state) + sizeof(word_t) - 1) / sizeof(word_t)));
  m_query->unwind_p = m_unwind_heap.get();
  m_query->heap_p   = m_term_heap.get();
  m_query->heap_e   = m_term_heap.get() + TERM_HEAP_SIZE;
  m_query->cp       = nullptr;
}


bool
interpreter::has_meta_op(size_t id) const noexcept
{ return m_metaops.contains(id); }


bool
interpreter::has_predicate(size_t id) const noexcept
{
  // Check static predicates
  for (const auto &[w, _] : m_predicates)
  {
    term_header hdr;
    basic_decoder().decode(w, hdr);
    if (hdr.id == id)
      return true;
  }
  // Check dynamic predicates
  for (const auto &[w, _] : m_dyndb)
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


bool
interpreter::is_dynamic(word_t signkey) const noexcept
{
  const size_t k = signkey & term_mask;
  return m_dynamic_names.contains(k);
}

bool
interpreter::is_static(word_t signkey) const noexcept
{ return not is_dynamic(signkey); }


void
interpreter::dynamic(word_t signkey)
{
  basic_decoder dc;
  const size_t id = dc.decode_term_header(signkey).id;
  if (has_meta_op(id) or m_predicates.contains(signkey))
    throw std::runtime_error {"change of predicate qualifier (dynamic)"};
  m_dynamic_names.emplace(signkey);
}

void
interpreter::asserta(object_view signobj, object_view bodyobj)
{
  basic_decoder dc;

  assert(not signobj.empty());
  const size_t k = signobj[0] & term_mask;
  if (has_meta_op(dc.decode_term_header(k).id))
    throw std::runtime_error {"can't shadow meta-op with a predicate"};

  if (is_static(signobj[0]))
  {
    predicate_entry pred = prepare_predicate(signobj, bodyobj);
    std::vector<predicate_entry> &variants = m_predicates[k];
    variants.emplace(variants.begin(), std::move(pred));
  }
  else if (is_dynamic(signobj[0]))
    asserta_dyn(signobj, bodyobj);
  else
    assert(not "unreachable code");
}


void
interpreter::assertz(object_view signobj, object_view bodyobj)
{
  basic_decoder dc;

  assert(not signobj.empty());
  const size_t k = signobj[0] & term_mask;
  if (has_meta_op(dc.decode_term_header(k).id))
    throw std::runtime_error {"can't shadow meta-op with a predicate"};

  if (is_static(signobj[0]))
  {
    predicate_entry pred = prepare_predicate(signobj, bodyobj);
    std::vector<predicate_entry> &variants = m_predicates[k];
    variants.emplace_back(std::move(pred));
  }
  else if (is_dynamic(signobj[0]))
    assertz_dyn(signobj, bodyobj);
  else
    assert(not "unreachable code");
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
          resolve_path(p, m_importdirs.begin(), m_importdirs.end());
      if (not m_imports.emplace(fullpath).second)
        return; // File was already loaded

      if (fullpath.extension() == ".pl")
        return load_file(fullpath.c_str());
      else if (fullpath.extension() == ".plo")
        return load_objfile(fullpath.c_str());
      else
        assert(not "unreachable code" );
    }
    catch (...)
    { continue; }
  }

  throw std::runtime_error {
      std::format("failed to resolve library ({})", path_)};
}

void
interpreter::make_true(const dictionary &vardict, object_view expr,
                       const std::function<void(const solution &)> &cont,
                       bool recover_vars)
{
  varnamespace ns;
  barrier cp;

  push_choice_point(&cp);
  object_view adexpr = adopt_hp(ns, expr);
  try
  {
    make_true(*this, adexpr, [&](runtime &rt) {
      basic_decoder dc;
      solution sol;
      for (const auto [nsid, rtid] : ns)
      {
        const std::string_view varname = vardict[nsid];
        if (varname == "_")
          continue;
        if (const auto varval = rt.dereference(rtid))
        {
          object obj = rt.reconstruct(dc.decode_object(*varval));
          if (recover_vars)
            recover_variables(rt, ns, obj);
          sol[varname] = std::move(obj);
        }
        else
          sol[varname] = { };
      }
      cont(sol);
    });
  }
  catch (...)
  {
    m_query->cp = &cp; // hard reset the choice point list
    unwind(&cp);
    throw;
  }
  unwind(&cp);
}


void
interpreter::make_true(std::string_view expr,
                       const std::function<void(const solution &)> &cont)
{
  prolog_parser p;
  dictionary vardict;
  const object e = p.parse_expr(m_symdict, vardict, expr);
  make_true(vardict, e, cont, false);
}


void
interpreter::eval(object_view obj, const dictionary &vardict)
{
  std::cout << "[eval] " << dump_object(m_symdict, obj) << std::endl;
  make_true(vardict, obj, [this] (const solution &ans) {
    std::cout << "yes";
    bool isfirst = true;
    for (const auto &[name, val] : ans)
    {
      if (isfirst) std::cout << ": ";
      else         std::cout << ", ";
      isfirst = false;
      if (not val.empty())
        std::cout << name << " = " << dump_object(m_symdict, val);
      else
        std::cout << name << " is unbound";
    }
    std::cout << std::endl;
  });
}


void
interpreter::eval(std::string_view text)
{
  prolog_parser p;
  dictionary vardict;
  const object expr = p.parse_expr(m_symdict, vardict, text);
  eval(expr, vardict);
}

