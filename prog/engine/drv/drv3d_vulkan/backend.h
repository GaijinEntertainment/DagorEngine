// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// globals facade-like structure for quick access on everything that is singleton/global like under backend visibility
// use with related headers

namespace drv3d_vulkan
{

struct ExecutionPODState;
class BindlessManagerBackend;
class RenderStateSystemBackend;
class PipelineCompiler;
struct BackExecutionTimings;
class ExecutionSyncTracker;
struct ExecutionSyncCapture;
struct StackedProfileEvents;
class ExecutionState;
class PipelineState;
class PipelineStatePendingReferenceList;
struct PredictedLatencyWaiter;
class ShaderModuleStorage;
class GpuExecuteTimelineSpan;
struct ImmediateConstBuffers;
struct BackendInterop;
class AliasedMemoryStorage;
struct WrappedCommandBuffer;

struct Backend
{
  static BackExecutionTimings timings;
  static ExecutionSyncTracker sync;
  static ExecutionSyncCapture syncCapture;
  static StackedProfileEvents profilerStack;
  static BindlessManagerBackend bindless;
  static RenderStateSystemBackend renderStateSystem;
  static PipelineCompiler pipelineCompiler;
  static ImmediateConstBuffers immediateConstBuffers;
  static PredictedLatencyWaiter latencyWaiter;
  static ShaderModuleStorage shaderModules;
  static GpuExecuteTimelineSpan gpuJob;
  static BackendInterop interop;
  static AliasedMemoryStorage aliasedMemory;
  static WrappedCommandBuffer cb;

  struct State
  {
    static ExecutionPODState pod;
    static ExecutionState exec;
    static PipelineState pipe;
    static PipelineStatePendingReferenceList pendingCleanups;
  };
};

} // namespace drv3d_vulkan
