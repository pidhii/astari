#pragma once

#include "pl/obj/object.hpp"
#include "pl/core/runtime.hpp"


std::strong_ordering
compare(runtime &rt, object_iterator lhs, object_iterator rhs);

inline std::strong_ordering
compare(runtime &rt, object_view lhs, object_view rhs)
{ return compare(rt, lhs.begin(), rhs.end()); }