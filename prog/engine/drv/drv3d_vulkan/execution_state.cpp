// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_state.h"
#include "state_field_execution_context.h"
#include "execution_context.h"

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
}

void ExecutionStateStorage::makeDirty()
{
  graphics.makeDirty();
  compute.makeDirty();
  scopes.makeDirty();
  invalidateResBinds();
}

void ExecutionState::interruptRenderPass(const char *why)
{
  if (!getData().scopes.isDirty())
    executionContext->insertEvent(why, 0xFF00FFFF);

  if (Globals::cfg.bits.fatalOnNRPSplit &&
      (get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>()) == InPassStateFieldType::NATIVE_PASS)
    DAG_FATAL("vulkan: native RP can't be interrupted, but trying to interrupt it due to %s, caller %s", why,
      executionContext->getCurrentCmdCaller());

  set<StateFieldGraphicsRenderPassScopeCloser, bool, BackScopeState>(true);
  set<StateFieldGraphicsConditionalRenderingScopeCloser, ConditionalRenderingState::InvalidateTag, BackScopeState>({});
  set<StateFieldGraphicsQueryScopeCloser, GraphicsQueryState::InvalidateTag, BackScopeState>({});

  set<StateFieldGraphicsQueryScopeOpener, GraphicsQueryState::InvalidateTag, BackGraphicsState>({});
  set<StateFieldGraphicsConditionalRenderingScopeOpener, ConditionalRenderingState::InvalidateTag, BackGraphicsState>({});
  set<StateFieldGraphicsRenderPassScopeOpener, bool, BackGraphicsState>(true);
  set<StateFieldGraphicsRenderPassEarlyScopeOpener, bool, BackGraphicsState>(true);
}