// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// globals facade-like structure for quick access on everything that is singleton/global like
// use with related headers

class WinCritSec;
struct Driver3dDesc;

namespace drv3d_vulkan
{

class RaytraceScratchBuffer;
class DummyResources;
class DeviceMemoryPool;
class ResourceManager;
class PipelineManager;
class RenderPassManager;
class FenceManager;
class TimelineManager;
class Swapchain;
class ExecutionMarkers;
class SurveyQueryManager;
class TimestampQueryManager;
class BindlessManager;
class DeviceMemoryReport;
class RenderStateSystem;
class PipelineCache;
class VulkanDevice;
class DeviceQueueGroup;
struct DriverConfig;
class DeviceContext;
struct PhysicalDeviceSet;
class DebugNaming;
class SamplerCache;
struct FormatUtil;
class BufferAlignment;
class RaytraceBLASCompactionSizeQueryPool;
class VulkanInstance;
class ShaderProgramDatabase;
class VulkanLoader;
struct WindowState;
struct TexturePool;
struct BufferPool;
class RenderDocCaptureModule;
class DebugCallbacks;
struct GlobalDriverLock;

struct Globals
{
  static RaytraceScratchBuffer rtScratchBuffer;
  static RaytraceBLASCompactionSizeQueryPool rtSizeQueryPool;
  static DummyResources dummyResources;

  struct Mem
  {
    static WinCritSec mutex;
    static DeviceMemoryPool pool;
    static ResourceManager res;
    static DeviceMemoryReport report;
  };

  struct Res
  {
    static TexturePool tex;
    static BufferPool buf;
  };

  struct Dbg
  {
    static DebugNaming naming;
    static RenderDocCaptureModule rdoc;
    static DebugCallbacks callbacks;
  };

  static PipelineManager pipelines;
  static RenderPassManager passes;
  static FenceManager fences;
  static TimelineManager timelines;
  static Swapchain swapchain;
  static ExecutionMarkers gpuExecMarkers;
  static SurveyQueryManager surveys;
  static TimestampQueryManager timestamps;
  static BindlessManager bindless;
  static RenderStateSystem renderStates;
  static PipelineCache pipeCache;
  static DriverConfig cfg;
  static DeviceContext ctx;
  static SamplerCache samplers;
  static ShaderProgramDatabase shaderProgramDatabase;
  static Driver3dDesc desc;
  static WindowState window;
  static GlobalDriverLock lock;

  struct VK
  {
    static VulkanLoader loader;
    static VulkanDevice dev;
    static VulkanInstance inst;
    static DeviceQueueGroup que;
    static PhysicalDeviceSet phy;
    static FormatUtil fmt;
    static BufferAlignment bufAlign;
  };
};

} // namespace drv3d_vulkan
