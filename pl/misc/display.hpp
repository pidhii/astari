#pragma once

#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"

#include <ostream>


void
dump_object(const dictionary &dict, object_view obj, std::ostream &os);

std::string
dump_object(const dictionary &dict, object_view obj);

