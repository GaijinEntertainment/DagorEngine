// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// wrapper for debug naming of objects

#include "vk_wrapped_handles.h"

namespace drv3d_vulkan
{

class Image;
class Buffer;
class RenderPassResource;
class RaytraceAccelerationStructure;

class DebugNaming
{
  void setVkObjectDebugName(VulkanHandle handle, VkDebugReportObjectTypeEXT type1, VkObjectType type2, const char *name);

public:
  void setShaderModuleName(VulkanShaderModuleHandle shader, const char *name);
  void setPipelineName(VulkanPipelineHandle pipe, const char *name);
  void setPipelineLayoutName(VulkanPipelineLayoutHandle layout, const char *name);
  void setDescriptorName(VulkanDescriptorSetHandle set_handle, const char *name);
  void setCommandBufferName(VulkanCommandBufferHandle cmdb, const char *name);
  void setImageViewName(VulkanImageViewHandle image_view, const char *name);
  void setTexName(Image *img, const char *name);
  void setBufName(Buffer *buf, const char *name);
  void setRenderPassName(RenderPassResource *rp, const char *name);
  void setRenderPassName(VulkanRenderPassHandle rp, const char *name);
  void setAccelerationStructureName(RaytraceAccelerationStructure *as, const char *name);
  void setSamplerName(VulkanSamplerHandle sampler, const char *name);
  void setSemaphoreName(VulkanSemaphoreHandle sem, const char *name);
};

} // namespace drv3d_vulkan
