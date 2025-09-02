// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frontend.h"
#include "global_const_buffer.h"
#include "predicted_latency_waiter.h"
#include "pipeline_state.h"
#include "globals.h"
#include "timelines.h"
#include "resource_upload_limit.h"
#include "execution_timings.h"
#include "frontend_pod_state.h"
#include "temp_buffers.h"
#include "execution_sync_capture.h"
#include "resource_readbacks.h"
#include "swapchain.h"

using namespace drv3d_vulkan;

GlobalConstBuffer Frontend::GCB;
PredictedLatencyWaiter Frontend::latencyWaiter;
CpuReplayTimelineSpan Frontend::replay(Globals::timelines);
ResourceUploadLimit Frontend::resUploadLimit;
FrontExecutionTimings Frontend::timings;
TempBufferManager Frontend::tempBuffers;
FramememBufferManager Frontend::frameMemBuffers;
ExecutionSyncCapture Frontend::syncCapture;
ResourceReadbacks Frontend::readbacks;
Swapchain Frontend::swapchain;

PipelineState Frontend::State::pipe;
FrontendPODState Frontend::State::pod;
