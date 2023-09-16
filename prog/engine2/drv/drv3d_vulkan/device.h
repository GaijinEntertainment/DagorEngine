#pragma once
#ifndef __DRV_VULKAN_DEVICE_H__
#define __DRV_VULKAN_DEVICE_H__

#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <generic/dag_carray.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_threads.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include "pipeline_cache_file.h"

#include "device_memory.h"
#include "driver.h"
#include "event_pool.h"
#include "fence_manager.h"
#include "shader.h"
#include "pipeline.h"
#include "render_pass.h"
#include "vendor.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "graphics_state.h"
#include "bindless.h"

#include "physical_device_set.h"
#include "device_memory_report.h"

#include "device_queue.h"

#include "swapchain.h"
#include "device_context.h"
#include "util/guarded_object.h"
#include "render_state_system.h"
#include "image_resource.h"
#include "buffer_resource.h"
#include "render_pass_resource.h"

#if D3D_HAS_RAY_TRACING
#include "raytrace_as_resource.h"
#endif

#include "resource_manager.h"
#include "timeline.h"
#include "execution_markers.h"

namespace drv3d_vulkan
{
class Device;
class ShaderProgramDatabase;

#if D3D_HAS_RAY_TRACING

struct DeviceRaytraceData
{
  Buffer *scratchBuffer = nullptr;
  WinCritSec scratchReallocMutex;
};

#endif
class Device
{
public:
  struct Config
  {
    DeviceFeaturesConfig::Type features;
    uint32_t dedicatedMemoryForImageTexelCountThreshold;
    uint32_t memoryStatisticsPeriod;
    uint32_t texUploadLimitMb;
    uint32_t debugLevel;
  };

  // immovable,non-copyable objects, no need for getters for them
  GuardedObject<DeviceMemoryPool> memory;
  ResourceManager resources;
  PipelineManager pipeMan; //-V730_NOINIT
  RenderPassManager passMan;
  FenceManager fenceManager;
  TimelineManager timelineMan;
  Swapchain swapchain;
  ExecutionMarkers execMarkers;
  SurveyQueryManager surveys;
  TimestampQueryManager timestamps;
  BindlessManager bindlessManager;

private:
  VulkanDevice device;
  DeviceQueueGroup queues;
  PhysicalDeviceSet physicalDeviceInfo; //-V730_NOINIT
  DeviceFeaturesConfig::Type features;
  uint32_t debugLevel = 0;
  DeviceMemoryReport deviceMemoryReport;

  Tab<SamplerInfo *> samplers;
  SamplerInfo *defaultSampler; //-V730_NOINIT

  VulkanPipelineCacheHandle pipelineCache; //-V730_NOINIT


  ResourceDummySet dummyResourceTable = {};
  Image *dummyImage2D = nullptr;
  Image *dummyImage2DwithCompare = nullptr;
  Image *dummyImage3D = nullptr;
  Image *dummyImage3DwithCompare = nullptr;
  Image *dummyStorage2D = nullptr;
  Image *dummyStorage3D = nullptr;
  Buffer *dummyBuffer = nullptr;

  size_t arrayImageTo3DBufferSize = 0;
  Buffer *arrayImageTo3DBuffer = nullptr;

  uint32_t dedicatedMemoryForImageTexelCountThreshold = 0;
  uint32_t texUploadLimit = 0;

  EventPool eventPool;
  DeviceContext context;
#if D3D_HAS_RAY_TRACING
  DeviceRaytraceData raytraceData;
#endif
  RenderStateSystem renderStateSystem;

  void configure(const Config &ucfg, const PhysicalDeviceSet &pds);

public:
  // returns true if a image should be allocated via pool memory or
  // use a dedicated allocation
  bool shouldUsePool(const VkImageCreateInfo &ii);
  uint32_t getMinimalBufferAlignment() const;
  VulkanPipelineCacheHandle getPipeCache() const { return pipelineCache; }

  Image *getDummy2DImage() { return dummyImage2D; }
  const ResourceDummySet &getDummyResourceTable() const { return dummyResourceTable; }

  VulkanImageViewHandle getImageView(Image *img, ImageViewState state);

  VulkanDevice &getVkDevice() { return device; };
  bool checkImageCreate(const VkImageCreateInfo &ici, FormatStore format);
  Buffer *getArrayImageTo3DBuffer(size_t required_size);
  uint32_t getTexUploadLimit() const { return texUploadLimit; }

  const DataBlock *getPerDriverPropertyBlock(const char *prop_name);

private:
  Device(const Device &);
  Device &operator=(const Device &);

  void setupDummyResources();
  void shutdownDummyResources();
  void tryCreateDummyImage(const Image::Description::TrimmedCreateInfo &ii, const char *tex_name, bool needCompare, Image *&target);
  void setVkObjectDebugName(VulkanHandle handle, VkDebugReportObjectTypeEXT type1, VkObjectType type2, const char *name);


public:
  Device() : context(*this), swapchain(*this), resources(), timelineMan() {}
  ~Device();
  void preRelease();
  DeviceMemoryConfiguration getDeviceMemoryConfiguration() const;
  bool checkFormatSupport(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkSampleCountFlags samples);
  VkFormatFeatureFlags getFormatFeatures(VkFormat format);
  bool init(VulkanInstance &inst, const Config &ucfg, const PhysicalDeviceSet &set, const VkDeviceCreateInfo &config,
    const SwapchainMode &swc_info, const DeviceQueueGroup::Info &queue_info);
  bool isInitialized() const;
  void shutdown();
  void adjustCaps(Driver3dDesc &);
  void initBindless();
#if _TARGET_ANDROID
  void fillMobileCaps(Driver3dDesc &caps);
#endif
  void loadPipelineCache(PipelineCacheFile &src);
  void storePipelineCache(PipelineCacheFile &dst);
  bool canRenderTo3D() const;
#if D3D_HAS_RAY_TRACING
  bool hasRaytraceSupport() const;
#endif
  bool hasGpuTimestamps();
  uint64_t getGpuTimestampFrequency() const;
  Image *createImage(const ImageCreateInfo &ii);
  Buffer *createBuffer(uint32_t size, DeviceMemoryClass memory_class, uint32_t discard_count, BufferMemoryFlags mem_flags);
  void addBufferView(Buffer *buffer, FormatStore format);
  SamplerInfo *getSampler(SamplerState state);
  void setShaderModuleName(VulkanShaderModuleHandle shader, const char *name);
  void setPipelineName(VulkanPipelineHandle pipe, const char *name);
  void setPipelineLayoutName(VulkanPipelineLayoutHandle layout, const char *name);
  void setDescriptorName(VulkanDescriptorSetHandle set_handle, const char *name);
  void setTexName(Image *img, const char *name);
  void setBufName(Buffer *buf, const char *name);
  void setRenderPassName(RenderPassResource *rp, const char *name);
  int getDeviceVendor() const;
  bool hasGeometryShader() const;
  bool hasTesselationShader() const;
  bool hasFragmentShaderUAV() const;
  bool hasDepthBoundsTest() const;
  bool hasAnisotropicSampling() const;
  bool hasDepthClamp() const;
  bool hasImageCubeArray() const;
  bool hasMultiDrawIndirect() const;
  bool hasDrawIndirectFirstInstance() const;
  bool hasConditionalRender() const;
  bool hasUAVOnlyForcedSampleCount() const;
  bool hasImagelessFramebuffer() const;
  bool hasWaveOperations(VkShaderStageFlags stages) const;
  bool hasAttachmentNoStoreOp() const;
  bool hasDepthStencilResolve() const;
  bool hasCreateRenderPass2() const;
  uint32_t getCurrentAvailableMemoryKb();

  VkSampleCountFlagBits calcMSAAQuality() const;

  VulkanShaderModuleHandle makeVkModule(const ShaderModuleBlob *module);

  VkDevice getDevice() const { return device.get(); }
  VkInstance getInstance() const { return device.getInstance().get(); }
  VkPhysicalDevice getPhysicalDevice() const { return getDeviceProperties().device; }

  DeviceQueue &getQueue(DeviceQueueType que_type) { return queues[que_type]; }

  RenderStateSystem &getRenderStateSystem() { return renderStateSystem; }

  const PhysicalDeviceSet &getDeviceProperties() const { return physicalDeviceInfo; }

  DeviceMemoryReport &getMemoryReport() { return deviceMemoryReport; }

  DeviceFeaturesConfig::Type getFeatures() const { return features; }
  uint32_t getDebugLevel() const { return debugLevel; }

  DeviceContext &getContext() { return context; }

  Texture *wrapVKImage(VkImage tex_res, ResourceBarrier current_state, int width, int height, int layers, int mips, const char *name,
    int flg);

#if D3D_HAS_RAY_TRACING
  RaytraceAccelerationStructure *createRaytraceAccelerationStructure(RaytraceGeometryDescription *desc, uint32_t count,
    RaytraceBuildFlags flags);
  RaytraceAccelerationStructure *createRaytraceAccelerationStructure(uint32_t elements, RaytraceBuildFlags flags);

  Buffer *getRaytraceScratchBuffer()
  {
    // FIXME: potentional race (it is backed up by delayed destruction in frame)
    WinAutoLock lock(raytraceData.scratchReallocMutex);
    return raytraceData.scratchBuffer;
  }
#endif
  bool isCoherencyAllowedFor(uint32_t memoryType)
  {
    return features.test(DeviceFeaturesConfig::HAS_COHERENT_MEMORY) && memory.unlocked().isCoherentMemoryType(memoryType);
  }
#if D3D_HAS_RAY_TRACING
  void ensureRoomForRaytraceBuildScratchBuffer(VkDeviceSize size);
#endif

  std::atomic<uint32_t> allocated_upload_buffer_size{0};
};

Device::Config get_device_config(const DataBlock *cfg);
} // namespace drv3d_vulkan

#endif //__DRV_VULKAN_DEVICE_H__
