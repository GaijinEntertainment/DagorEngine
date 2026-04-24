// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/3d/dag_commands.h>
#include "pipeline_state.h"
#include "resource_manager.h"
#include "image_resource.h"
#include <3d/gpuLatency.h>

namespace drv3d_vulkan
{

struct CmdCompileGraphicsPipeline
{
  VkPrimitiveTopology top;
};

struct CmdCompileComputePipeline
{};

struct CmdRecordFrameTimings
{
  Drv3dTimings *timingData;
  uint64_t kickoffTime;
};

struct CmdCleanupPendingReferences
{
  PipelineState state;
};

struct CmdShutdownPendingReferences
{};

struct CmdRegisterFrameEventsCallback
{
  FrameEvents *callback;
};

struct CmdGetWorkerCpuCore
{
  int *core;
  int *threadId;
};

struct CmdAsyncPipeFeedbackBegin
{
  uint32_t *feedbackPtr;
};

struct CmdAsyncPipeFeedbackEnd
{};

struct CmdUpdateAliasedMemoryInfo
{
  ResourceMemoryId id;
  AliasedResourceMemory info;
};

struct CmdImgActivate
{
  Image *img;
};

struct CmdSetLatencyMarker
{
  uint32_t frame_id;
  lowlatency::LatencyMarkerType type;
};

struct CmdRestartPipelineCompiler
{};

} // namespace drv3d_vulkan
