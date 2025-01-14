// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "driver.h"
#include "format_traits.h"
#include <ioSys/dag_dataBlock.h>
#include "globals.h"
#include "device_context.h"

using namespace drv3d_vulkan;

namespace app_info
{
static uint32_t ver = 0;
static const char *name = "Dagor";

static uint32_t engine_ver = 0x06000000; // dagor 6.0.0.0
static const char *engine_name = "Dagor";
} // namespace app_info

void drv3d_vulkan::generateFaultReport() { Globals::ctx.generateFaultReport(); }

void drv3d_vulkan::set_app_info(const char *name, uint32_t ver)
{
  app_info::ver = ver;
  app_info::name = name;
}

uint32_t drv3d_vulkan::get_app_ver() { return app_info::ver; }

VkApplicationInfo drv3d_vulkan::create_app_info(const DataBlock *cfg)
{
  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = NULL;

  if (cfg->getBool("noAppInfo", false))
  {
    appInfo.pApplicationName = NULL;
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = NULL;
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VULKAN_USED_VERSION;
  }
  else
  {
    if (cfg->getBool("useAppInfoOverride", false))
    {
      appInfo.pApplicationName = cfg->getStr("applicationName", app_info::name);
      appInfo.applicationVersion = cfg->getInt("applicationVersion", app_info::ver);
      appInfo.pEngineName = cfg->getStr("engineName", app_info::engine_name);
      appInfo.engineVersion = cfg->getInt("engineVersion", app_info::engine_ver);
      appInfo.apiVersion = cfg->getInt("apiVersion", VULKAN_USED_VERSION);
    }
    else
    {
      appInfo.pApplicationName = app_info::name;
      appInfo.applicationVersion = app_info::ver;
      appInfo.pEngineName = app_info::engine_name;
      appInfo.engineVersion = app_info::engine_ver;
      appInfo.apiVersion = VULKAN_USED_VERSION;
    }
  }
  return appInfo;
}

VkPresentModeKHR drv3d_vulkan::get_requested_present_mode(bool use_vsync, bool use_adaptive_vsync)
{
  if (use_adaptive_vsync && use_vsync)
    return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
  else if (use_vsync)
    return VK_PRESENT_MODE_FIFO_KHR;
  else
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkBufferImageCopy drv3d_vulkan::make_copy_info(FormatStore format, uint32_t mip, uint32_t base_layer, uint32_t layers,
  VkExtent3D original_size, VkDeviceSize src_offset)
{
  VkBufferImageCopy ret = {};
  ret.bufferOffset = src_offset;
  ret.imageSubresource = {format.getAspektFlags(), mip, base_layer, layers};
  ret.imageExtent = {max<uint32_t>(original_size.width >> mip, 1u), max<uint32_t>(original_size.height >> mip, 1u),
    max<uint32_t>(original_size.depth >> mip, 1u)};

  return ret;
}

const char *drv3d_vulkan::formatActiveExecutionStage(ActiveExecutionStage stage)
{
  const char *msg = "frame begin";
  if (stage == ActiveExecutionStage::GRAPHICS)
    msg = "graphics";
  else if (stage == ActiveExecutionStage::COMPUTE)
    msg = "compute";
  else if (stage == ActiveExecutionStage::CUSTOM)
    msg = "custom";
#if D3D_HAS_RAY_TRACING
  else if (stage == ActiveExecutionStage::RAYTRACE)
    msg = "raytrace";
#endif
  return msg;
}

const char *drv3d_vulkan::formatExtendedShaderStage(ExtendedShaderStage stage)
{
  switch (stage)
  {
    case ExtendedShaderStage::CS: return "CS";
    case ExtendedShaderStage::PS: return "PS";
    case ExtendedShaderStage::VS: return "VS";
    case ExtendedShaderStage::RT: return "RT";
    case ExtendedShaderStage::TC: return "TC";
    case ExtendedShaderStage::TE: return "TE";
    case ExtendedShaderStage::GS: return "GS";
    default: return "UNK";
  }
  return "UNK";
}

const char *drv3d_vulkan::formatShaderStage(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: return "CS";
    case STAGE_PS: return "PS";
    case STAGE_VS: return "VS";
    default: return "UNK";
  }
  return "UNK";
}
