#pragma once

#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <render/resourceSlot/detail/actionList.h>

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

[[nodiscard]] NodeHandleWithSlotsAccess register_access(const char *name, const char *source_location, ActionList &&action_list,
  AccessCallback &&declaration_callback, unsigned storage_id);

} // namespace detail

} // namespace resource_slot