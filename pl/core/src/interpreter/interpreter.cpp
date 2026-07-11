#include "interpreter.hpp"


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
  // arxt::insert(&m_predicates, signobj, bodyobj);
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
  // arxt::insert(&m_predicates, signobj, bodyobj);
  m_predicates.emplace(signobj[0], std::make_pair(signobj, object_view()));
}
