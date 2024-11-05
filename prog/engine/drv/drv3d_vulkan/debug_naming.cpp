// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "debug_naming.h"
#include "globals.h"
#include "vulkan_device.h"
#include "image_resource.h"
#include "buffer_resource.h"
#include "render_pass_resource.h"

#if D3D_HAS_RAY_TRACING
#include "raytrace_as_resource.h"
#endif

using namespace drv3d_vulkan;

void DebugNaming::setVkObjectDebugName(VulkanHandle handle, VkDebugReportObjectTypeEXT type1, VkObjectType type2, const char *name)
{
  G_UNUSED(handle);
  G_UNUSED(type1);
  G_UNUSED(type2);
  G_UNUSED(name);

#if VK_EXT_debug_utils
  auto &instance = Globals::VK::inst;
  if (instance.hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.pNext = nullptr;
    info.objectType = type2;
    info.objectHandle = handle;
    info.pObjectName = name;
    VULKAN_LOG_CALL(instance.vkSetDebugUtilsObjectNameEXT(Globals::VK::dev.get(), &info));
    // if we have VK_EXT_debug_utils & VK_EXT_debug_marker are mutually exclusive via dependency
    return;
  }
#endif
#if VK_EXT_debug_marker
  if (Globals::VK::dev.hasExtension<DebugMarkerEXT>())
  {
    VkDebugMarkerObjectNameInfoEXT info;
    info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
    info.pNext = NULL;
    info.objectType = type1;
    info.object = handle;
    info.pObjectName = name;
    VULKAN_LOG_CALL(Globals::VK::dev.vkDebugMarkerSetObjectNameEXT(Globals::VK::dev.get(), &info));
  }
#endif
}

void DebugNaming::setPipelineLayoutName(VulkanPipelineLayoutHandle layout, const char *name)
{
  setVkObjectDebugName(generalize(layout), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}

void DebugNaming::setCommandBufferName(VulkanCommandBufferHandle cmdb, const char *name)
{
  setVkObjectDebugName(generalize(cmdb), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
}

void DebugNaming::setDescriptorName(VulkanDescriptorSetHandle set_handle, const char *name)
{
  setVkObjectDebugName(generalize(set_handle), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}

void DebugNaming::setShaderModuleName(VulkanShaderModuleHandle shader, const char *name)
{
  setVkObjectDebugName(generalize(shader), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, VK_OBJECT_TYPE_SHADER_MODULE, name);
}

void DebugNaming::setPipelineName(VulkanPipelineHandle pipe, const char *name)
{
  setVkObjectDebugName(generalize(pipe), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, VK_OBJECT_TYPE_PIPELINE, name);
}

void DebugNaming::setImageViewName(VulkanImageViewHandle image_view, const char *name)
{
  setVkObjectDebugName(generalize(image_view), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, VK_OBJECT_TYPE_IMAGE_VIEW, name);
}

void DebugNaming::setTexName(Image *img, const char *name)
{
  img->setDebugName(name);
  setVkObjectDebugName(generalize(img->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, VK_OBJECT_TYPE_IMAGE, name);
}

void DebugNaming::setBufName(Buffer *buf, const char *name)
{
  buf->setDebugName(name);
  setVkObjectDebugName(generalize(buf->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, VK_OBJECT_TYPE_BUFFER, name);
}

void DebugNaming::setRenderPassName(RenderPassResource *rp, const char *name)
{
  rp->setDebugName(name);
  setVkObjectDebugName(generalize(rp->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, VK_OBJECT_TYPE_RENDER_PASS, name);
}

#if D3D_HAS_RAY_TRACING
void DebugNaming::setAccelerationStructureName(RaytraceAccelerationStructure *as, const char *name)
{
  setVkObjectDebugName(generalize(as->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT,
    VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, name);
}
#endif
