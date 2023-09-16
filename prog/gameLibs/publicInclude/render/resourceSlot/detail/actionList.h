#pragma once

#include <EASTL/variant.h>
#include <dag/dag_vector.h>

namespace resource_slot
{

struct Create;
struct Update;
struct Read;

namespace detail
{
typedef eastl::variant<eastl::monostate, Create, Update, Read> SlotAction;
typedef dag::Vector<SlotAction> ActionList;
} // namespace detail

} // namespace resource_slot