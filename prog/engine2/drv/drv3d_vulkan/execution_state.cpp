#include "state_field_execution_context.h"
#include "device.h"

namespace drv3d_vulkan
{

template <>
void ExecutionStateStorage::applyTo(ExecutionContext &) const
{}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void ExecutionStateStorage::clearDirty()
{
  graphics.clearDirty();
  compute.clearDirty();
  scopes.clearDirty();
#if D3D_HAS_RAY_TRACING
  raytrace.clearDirty();
#endif
}

void ExecutionStateStorage::makeDirty()
{
  graphics.makeDirty();
  compute.makeDirty();
  scopes.makeDirty();
#if D3D_HAS_RAY_TRACING
  raytrace.makeDirty();
#endif
}

void ExecutionState::interruptRenderPass(const char *why)
{
  if (!getData().scopes.isDirty())
    executionContext->insertEvent(why, 0xFF00FFFF);

  G_ASSERTF((get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>()) != InPassStateFieldType::NATIVE_PASS,
    "vulkan: native RP can't be interrupted, but trying to interrupt it due to %s, caller %s", why,
    executionContext->getCurrentCmdCaller());

  set<StateFieldGraphicsRenderPassScopeCloser, bool, BackScopeState>(true);
  set<StateFieldGraphicsConditionalRenderingScopeCloser, ConditionalRenderingState::InvalidateTag, BackScopeState>({});

  set<StateFieldGraphicsConditionalRenderingScopeOpener, ConditionalRenderingState::InvalidateTag, BackGraphicsState>({});
  set<StateFieldGraphicsRenderPassScopeOpener, bool, BackGraphicsState>(true);
}