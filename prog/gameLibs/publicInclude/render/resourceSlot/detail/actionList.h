//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/variant.h>
#include <generic/dag_relocatableFixedVector.h>

namespace resource_slot
{

struct Create;
struct Update;
struct Read;

namespace detail
{
inline constexpr int EXPECTED_MAX_DECLARATIONS = 2;

typedef eastl::variant<eastl::monostate, Create, Update, Read> SlotAction;
typedef dag::RelocatableFixedVector<SlotAction, EXPECTED_MAX_DECLARATIONS> ActionList;
} // namespace detail

} // namespace resource_slot