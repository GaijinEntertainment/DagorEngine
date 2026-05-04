// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// globals facade-like structure for quick access on everything that is singleton/global like under backend visibility
// use with related headers

class StackedProfileEvents;

namespace drv3d_vulkan
{

struct ExecutionAsyncCompileState;
class BindlessManagerBackend;
class RenderStateSystemBackend;
class PipelineCompiler;
struct BackExecutionTimings;
class ExecutionSyncTracker;
struct ExecutionSyncCapture;
class ExecutionState;
class PipelineState;
class PipelineStatePendingReferenceList;
struct PredictedLatencyWaiter;
class ShaderModuleStorage;
class GpuExecuteTimelineSpan;
struct BackendInterop;
class AliasedMemoryStorage;
struct WrappedCommandBuffer;
class BEContext;

struct Backend
{
  static BackExecutionTimings timings;
  static ExecutionSyncTracker sync;
  static ExecutionSyncCapture syncCapture;
  static StackedProfileEvents profilerStack;
  static BindlessManagerBackend bindless;
  static RenderStateSystemBackend renderStateSystem;
  static PipelineCompiler pipelineCompiler;
  static PredictedLatencyWaiter latencyWaiter;
  static ShaderModuleStorage shaderModules;
  static GpuExecuteTimelineSpan gpuJob;
  static BackendInterop interop;
  static AliasedMemoryStorage aliasedMemory;
  static WrappedCommandBuffer cb;
  static BEContext ctx;

  struct State
  {
    static ExecutionAsyncCompileState asyncCompileState;
    static ExecutionState exec;
    static PipelineState pipe;
    static PipelineStatePendingReferenceList pendingCleanups;
  };
};

} // namespace drv3d_vulkan
