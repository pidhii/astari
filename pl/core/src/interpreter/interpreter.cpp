#include "interpreter.hpp"
#include "pl/coding/basic_decoder.hpp"


interpreter::interpreter()
{
  auto require = [this] (std::string_view name, meta_symbol op) {
    if (m_symdict[name] != op)
      throw std::logic_error {"failed to register symbol"};
  };
  require("", op_and);
  require("or", op_or);
  require("if", op_if);
  require("fail", op_fail);
}


bool
interpreter::has_meta_op(size_t id) const noexcept
{ return m_metaops.contains(id); }


bool
interpreter::has_predicate(size_t id) const noexcept
{
  for (const auto [w, _] : m_predicates)
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
interpreter::add_predicate(std::string_view sign, std::string_view body)
{
  dictionary vardict;
  object_parser parser {m_symdict, vardict};
  object signobj = parser.parse_object(sign);
  object bodyobj = parser.parse_object(body);
  assert(not signobj.empty());
  assert(not bodyobj.empty());
  assert(word_type(signobj[0]) == word_type::structure);
  const size_t id = basic_decoder().decode_term_header(signobj[0]).id;
  if (has_meta_op(id))
  {
    throw std::runtime_error {std::format(
        "predicate name already used for meta operator ({})", m_symdict[id])};
  }
  m_predicates.emplace(signobj[0], std::make_pair(signobj, bodyobj));
}


void
interpreter::add_predicate(std::string_view sign)
{
  dictionary vardict;
  object_parser parser {m_symdict, vardict};
  object signobj = parser.parse_object(sign);
  assert(not signobj.empty());
  assert(word_type(signobj[0]) == word_type::structure);
  const size_t id = basic_decoder().decode_term_header(signobj[0]).id;
  if (has_meta_op(id))
  {
    throw std::runtime_error {std::format(
        "predicate name already used for meta operator ({})", m_symdict[id])};
  }
  m_predicates.emplace(signobj[0], std::make_pair(signobj, object_view()));
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
