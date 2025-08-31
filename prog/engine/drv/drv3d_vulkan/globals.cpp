// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>

#include "globals.h"
#include "vulkan_api.h"
#include "dummy_resources.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "pipeline/manager.h"
#include "device_memory_report.h"
#include "fence_manager.h"
#include "timelines.h"
#include "execution_markers.h"
#include "query_pools.h"
#include "bindless.h"
#include "dlss.h"
#include "streamline_adapter.h"
#include "render_state_system.h"
#include "pipeline_cache.h"
#include "vulkan_device.h"
#include "device_queue.h"
#include "driver_config.h"
#include "device_context.h"
#include "physical_device_set.h"
#include "debug_naming.h"
#include "sampler_cache.h"
#include "vk_format_utils.h"
#include "buffer_alignment.h"
#include "vulkan_instance.h"
#include "shader/program_database.h"
#include "os.h"
#include "external_resource_pools.h"
#include "renderDoc_capture_module.h"
#include "debug_callbacks.h"
#include "global_lock.h"

using namespace drv3d_vulkan;

RaytraceBLASCompactionSizeQueryPool Globals::rtSizeQueryPool;
DummyResources Globals::dummyResources;

WinCritSec Globals::Mem::mutex("vk_mem_mutex");
DeviceMemoryPool Globals::Mem::pool;
ResourceManager Globals::Mem::res;
DeviceMemoryReport Globals::Mem::report;

TexturePool Globals::Res::tex;
BufferPool Globals::Res::buf;

DebugNaming Globals::Dbg::naming;
RenderDocCaptureModule Globals::Dbg::rdoc;
DebugCallbacks Globals::Dbg::callbacks;

PipelineManager Globals::pipelines;
RenderPassManager Globals::passes;
FenceManager Globals::fences;
TimelineManager Globals::timelines;
ExecutionMarkers Globals::gpuExecMarkers;
SurveyQueryManager Globals::surveys;
QueryManager Globals::timestamps;
QueryManager Globals::occlusionQueries;
BindlessManager Globals::bindless;
RenderStateSystem Globals::renderStates;
PipelineCache Globals::pipeCache;
DriverConfig Globals::cfg;
DeviceContext Globals::ctx;
SamplerCache Globals::samplers;
ShaderProgramDatabase Globals::shaderProgramDatabase;
Driver3dDesc Globals::desc;
WindowState Globals::window;
#if !USE_STREAMLINE_FOR_DLSS
DLSSSuperResolutionDirect Globals::dlss;
#endif

GlobalDriverLock Globals::lock;
thread_local uint32_t GlobalDriverLock::acquired = 0;

VulkanLoader Globals::VK::loader;
VulkanDevice Globals::VK::dev;
DeviceQueueGroup Globals::VK::queue;
PhysicalDeviceSet Globals::VK::phy;
FormatUtil Globals::VK::fmt;
BufferAlignment Globals::VK::bufAlign;
VulkanInstance Globals::VK::inst;
