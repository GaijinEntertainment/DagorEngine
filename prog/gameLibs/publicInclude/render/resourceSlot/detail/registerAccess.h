//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <render/resourceSlot/detail/actionList.h>
#include <render/daBfg/nameSpace.h>

#include <generic/dag_fixedMoveOnlyFunction.h>

namespace dabfg
{
class NodeHandle;
}

namespace resource_slot
{

struct State;

namespace detail
{
// 128 bytes should be enough for anyone. If more is required, use
// an explicit unique_ptr indirection.
inline constexpr size_t MAX_CALLBACK_SIZE = 128;
typedef dag::FixedMoveOnlyFunction<MAX_CALLBACK_SIZE, dabfg::NodeHandle(resource_slot::State)> AccessCallback;

[[nodiscard]] NodeHandleWithSlotsAccess register_access(dabfg::NameSpace ns, const char *name, ActionList &&action_list,
  AccessCallback &&declaration_callback);

} // namespace detail

} // namespace resource_slot