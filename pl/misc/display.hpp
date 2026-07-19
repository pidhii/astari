#pragma once

#include "pl/dictionary.hpp"
#include "pl/obj/object.hpp"

#include <ostream>


void
dump_object(const dictionary &dict, object_view obj, std::ostream &os,
            bool quoted = false, bool ignore_ops = false,
            bool numbervars = false);

std::string
dump_object(const dictionary &dict, object_view obj, bool quoted = false,
            bool ignore_ops = false, bool numbervars = false);
