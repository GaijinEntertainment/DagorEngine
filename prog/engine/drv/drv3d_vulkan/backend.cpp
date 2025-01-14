// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "backend.h"
#include "timelines.h"
#include "execution_timings.h"
#include "execution_sync.h"
#include "stacked_profile_events.h"
#include "bindless.h"
#include "render_state_system.h"
#include "pipeline/compiler.h"
#include "state_field_resource_binds.h"
#include "predicted_latency_waiter.h"
#include "driver.h"
#include "execution_pod_state.h"
#include "execution_state.h"
#include "pipeline_state.h"
#include "pipeline_state_pending_references.h"
#include "globals.h"
#include "shader/module.h"
#include "backend_interop.h"
#include "resource_manager.h"
#include "execution_sync_capture.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

BackExecutionTimings Backend::timings;
ExecutionSyncTracker Backend::sync;
ExecutionSyncCapture Backend::syncCapture;
StackedProfileEvents Backend::profilerStack;
BindlessManagerBackend Backend::bindless;
RenderStateSystemBackend Backend::renderStateSystem;
PipelineCompiler Backend::pipelineCompiler(Globals::timelines);
ImmediateConstBuffers Backend::immediateConstBuffers;
PredictedLatencyWaiter Backend::latencyWaiter;
ShaderModuleStorage Backend::shaderModules;
GpuExecuteTimelineSpan Backend::gpuJob(Globals::timelines);
BackendInterop Backend::interop;
AliasedMemoryStorage Backend::aliasedMemory;
WrappedCommandBuffer Backend::cb;

ExecutionPODState Backend::State::pod;
ExecutionState Backend::State::exec;
PipelineState Backend::State::pipe;
PipelineStatePendingReferenceList Backend::State::pendingCleanups;
