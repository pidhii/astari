#pragma once

#include "pl/obj/object.hpp"
#include "pl/core/runtime.hpp"


std::strong_ordering
compare(runtime &rt, object_view lhs, object_view rhs);