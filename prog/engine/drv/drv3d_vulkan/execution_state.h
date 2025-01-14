// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// defines full draw/dispatch ready state that can be applied to execution context

#include "util/tracked_state.h"
#include "compute_state.h"
#include "graphics_state2.h"
#include "state_field_execution_context.h"
#include "back_scope_state.h"
#include "pipeline/stage_state_base.h"

namespace drv3d_vulkan
{

struct ExecutionStateStorage
{
  BackComputeState compute;
  BackGraphicsState graphics;
  StateFieldActiveExecutionStage activeExecutionStage;
  BackScopeState scopes;
  PipelineStageStateBase stageState[STAGE_MAX_EXT];

  void invalidateResBinds()
  {
    for (PipelineStageStateBase &stage : stageState)
      stage.invalidateState();
  }

  void reset() { invalidateResBinds(); }
  void dumpLog() const { debug("ExecutionStateStorage end"); }

  template <typename T>
  T &ref();
  template <typename T>
  T &refRO() const;
  template <typename T>
  void applyTo(T &) const;
  template <typename T>
  void transit(T &) const
  {}
  void makeDirty();
  void clearDirty();
};

class ExecutionState
  : public TrackedState<ExecutionStateStorage, StateFieldActiveExecutionStage, BackScopeState, BackComputeState, BackGraphicsState>
{
  ExecutionContext *executionContext;

public:
  ExecutionContext &getExecutionContext() { return *executionContext; }
  void setExecutionContext(ExecutionContext *ctx) { executionContext = ctx; }
  PipelineStageStateBase &getResBinds(ShaderStage stage) { return getData().stageState[stage]; }
  PipelineStageStateBase &getResBinds(ExtendedShaderStage stage)
  {
    switch (stage)
    {
      case ExtendedShaderStage::CS: return getData().stageState[ShaderStage::STAGE_CS];
      case ExtendedShaderStage::TC: return getData().stageState[ShaderStage::STAGE_VS];
      case ExtendedShaderStage::TE: return getData().stageState[ShaderStage::STAGE_VS];
      case ExtendedShaderStage::GS: return getData().stageState[ShaderStage::STAGE_VS];

      case ExtendedShaderStage::PS: return getData().stageState[ShaderStage::STAGE_PS];
      case ExtendedShaderStage::VS: return getData().stageState[ShaderStage::STAGE_VS];
      default: break;
    }
    G_ASSERTF(0, "vulkan: unkown extended shader stage %u:%s", (uint32_t)stage, formatExtendedShaderStage(stage));
    return getData().stageState[ShaderStage::STAGE_VS];
  }

  // interrupt without immediate apply!
  void interruptRenderPass(const char *reason);
};

} // namespace drv3d_vulkan
