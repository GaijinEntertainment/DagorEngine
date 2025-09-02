// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// globals facade-like structure for quick access on everything that is singleton/global like under frontend visibility
// use with related headers

namespace drv3d_vulkan
{

struct GlobalConstBuffer;
struct PredictedLatencyWaiter;
class PipelineState;
class CpuReplayTimelineSpan;
struct ResourceUploadLimit;
struct FrontExecutionTimings;
struct FrontendPODState;
class TempBufferManager;
class FramememBufferManager;
struct ExecutionSyncCapture;
struct ResourceReadbacks;
class Swapchain;

struct Frontend
{
  static GlobalConstBuffer GCB;
  static PredictedLatencyWaiter latencyWaiter;
  static CpuReplayTimelineSpan replay;
  static ResourceUploadLimit resUploadLimit;
  static FrontExecutionTimings timings;
  static TempBufferManager tempBuffers;
  static FramememBufferManager frameMemBuffers;
  static ExecutionSyncCapture syncCapture;
  static ResourceReadbacks readbacks;
  static Swapchain swapchain;

  struct State
  {
    static FrontendPODState pod;
    static PipelineState pipe;
  };
};

} // namespace drv3d_vulkan
