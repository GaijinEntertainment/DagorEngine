#include "state_field_execution_context.h"
#include "device.h"
#include "back_scope_state.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(BackScopeState, scopes, ExecutionStateStorage);

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void BackScopeStateStorage::clearDirty() {}

void BackScopeStateStorage::makeDirty() {}