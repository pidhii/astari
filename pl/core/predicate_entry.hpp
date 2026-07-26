#pragma once

#include "pl/obj/object.hpp"


struct predicate_entry { object sign, body; size_t nvars; };

predicate_entry
prepare_predicate(object_view signobj, object_view bodyobj);