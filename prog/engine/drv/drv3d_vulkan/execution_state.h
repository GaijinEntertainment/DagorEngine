// defines full draw/dispatch ready state that can be applied to execution context
#pragma once
#include "util/tracked_state.h"
#include "compute_state.h"
#include "graphics_state2.h"
#include "raytrace_state.h"
#include "state_field_execution_context.h"
#include "back_scope_state.h"

namespace drv3d_vulkan
{

struct ExecutionStateStorage
{
  BackComputeState compute;
  BackGraphicsState graphics;
#if D3D_HAS_RAY_TRACING
  BackRaytraceState raytrace;
#endif
  StateFieldActiveExecutionStage activeExecutionStage;
  BackScopeState scopes;

  void reset() {}
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
  : public TrackedState<ExecutionStateStorage, StateFieldActiveExecutionStage, BackScopeState, BackComputeState, BackGraphicsState
#if D3D_HAS_RAY_TRACING
      ,
      BackRaytraceState
#endif
      >
{
  ExecutionContext *executionContext;

public:
  ExecutionContext &getExecutionContext() { return *executionContext; }
  void setExecutionContext(ExecutionContext *ctx) { executionContext = ctx; }

  // interrupt without immediate apply!
  void interruptRenderPass(const char *reason);
};

} // namespace drv3d_vulkan
