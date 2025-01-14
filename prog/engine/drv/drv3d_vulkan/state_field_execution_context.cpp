// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "state_field_execution_context.h"
#include "back_scope_state.h"
#include "execution_state.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldActiveExecutionStage, activeExecutionStage, ExecutionStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldExecutionStageCloser, stageCloser, BackScopeStateStorage);

template <>
void StateFieldExecutionStageCloser::dumpLog(const BackScopeStateStorage &) const
{
  debug("stageCloser: last transit %s -> %s", formatActiveExecutionStage(data.from), formatActiveExecutionStage(data.to));
}

template <>
void StateFieldExecutionStageCloser::applyTo(BackScopeStateStorage &, ExecutionContext &) const
{}

template <>
void StateFieldActiveExecutionStage::applyTo(ExecutionStateStorage &state, ExecutionContext &)
{
  auto from = oldData;
  auto to = data;
  oldData = data;

  // graphics to graphics is synced with render passes
  // also access to render targets is handled with image state tracking
  if (ActiveExecutionStage::GRAPHICS == to && ActiveExecutionStage::GRAPHICS == from)
  {
    if (state.graphics.changed<StateFieldGraphicsRenderPassEarlyScopeOpener>())
    {
      state.graphics.makeDirty();
      state.compute.makeDirty();
    }
    return;
  }

  switch (data)
  {
    case ActiveExecutionStage::COMPUTE:
      state.compute.disableApply(false);
      state.graphics.disableApply(true);
      break;
    case ActiveExecutionStage::GRAPHICS:
      // needed to properly restart render pass after interrupt if it was not changed since interruption
      // state.graphics.makeFieldDirty<StateFieldGraphicsRenderPassScopeOpener>();
      // state.graphics.makeFieldDirty<StateFieldGraphicsRenderPassEarlyScopeOpener>();
      state.graphics.makeDirty();
      state.graphics.disableApply(false);
      state.compute.makeDirty();
      state.compute.disableApply(true);
      break;
    case ActiveExecutionStage::FRAME_BEGIN:
    case ActiveExecutionStage::CUSTOM:
      state.compute.disableApply(true);
      state.graphics.disableApply(true);
      break;
    default: G_ASSERTF(false, "vulkan: wrong active execution stage selected"); return;
  }

  // any scopes must be closed on stage change
  state.scopes.set<StateFieldExecutionStageCloser, StateFieldExecutionStageCloser::Transit>({from, to});
  state.scopes.set<StateFieldGraphicsNativeRenderPassScopeCloser, ActiveExecutionStage>(to);
  state.scopes.set<StateFieldGraphicsRenderPassScopeCloser, ActiveExecutionStage>(to);
  state.scopes.set<StateFieldGraphicsConditionalRenderingScopeCloser, ActiveExecutionStage>(to);
  state.scopes.set<StateFieldGraphicsQueryScopeCloser, ActiveExecutionStage>(to);
}

template <>
void StateFieldActiveExecutionStage::dumpLog(const ExecutionStateStorage &) const
{
  debug("activeExecutionStage: %s", formatActiveExecutionStage(data));
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;
