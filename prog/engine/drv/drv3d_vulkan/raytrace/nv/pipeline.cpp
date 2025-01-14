// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include "perfMon/dag_statDrv.h"

using namespace drv3d_vulkan;

const uint32_t RaytracePipelineShaderConfig::stages[RaytracePipelineShaderConfig::count] = {
  VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_ANY_HIT_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV |
  VK_SHADER_STAGE_INTERSECTION_BIT_NV | VK_SHADER_STAGE_CALLABLE_BIT_NV};

const uint32_t RaytracePipelineShaderConfig::registerIndexes[RaytracePipelineShaderConfig::count] = {
  spirv::raytrace::REGISTERS_SET_INDEX};

namespace drv3d_vulkan
{

template <>
void RaytracePipeline::onDelayedCleanupFinish<RaytracePipeline::CLEANUP_DESTROY>()
{
  shutdown(Globals::VK::dev);
  delete this;
}

} // namespace drv3d_vulkan

RaytracePipeline::RaytracePipeline(VulkanDevice &dev, ProgramID prog, VulkanPipelineCacheHandle cache, LayoutType *l,
  const CreationInfo &info) :
  BasePipeline(l), program(prog)
{
  eastl::vector<VkPipelineShaderStageCreateInfo> shaderInfos;
  eastl::vector<VkRayTracingShaderGroupCreateInfoNV> groupInfos;
  shaderInfos.resize(info.shader_count);
  groupInfos.resize(info.group_count);

  for (uint32_t i = 0; i < info.shader_count; ++i)
  {
    auto &target = shaderInfos[i];
    target.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    target.pNext = nullptr;
    target.flags = 0;
    target.stage = VkShaderStageFlagBits(info.uses[i].header.stage);
    target.module = info.module_set[info.uses[i].blobId]->module;
    target.pName = "main";
    target.pSpecializationInfo = nullptr;
  }

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  debugInfo.resize(info.shader_count);
  for (uint32_t i = 0; i < info.shader_count; ++i)
    debugInfo[i] = info.uses[i].dbg;
#endif

  eastl::transform(info.groups, info.groups + info.group_count, begin(groupInfos),
    [maxShader = info.shader_count](const RaytraceShaderGroup &groupInfo) {
      VkRayTracingShaderGroupCreateInfoNV gi = {VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV};

      if (groupInfo.type == RaytraceShaderGroupType::GENERAL)
      {
        gi.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
        gi.generalShader = groupInfo.indexToGeneralShader;
        gi.closestHitShader = VK_SHADER_UNUSED_NV;
        gi.anyHitShader = VK_SHADER_UNUSED_NV;
        gi.intersectionShader = VK_SHADER_UNUSED_NV;
      }
      else
      {
        gi.generalShader = VK_SHADER_UNUSED_NV;
        gi.closestHitShader = groupInfo.indexToClosestHitShader < maxShader ? groupInfo.indexToClosestHitShader : VK_SHADER_UNUSED_NV;
        gi.anyHitShader = groupInfo.indexToAnyHitShader < maxShader ? groupInfo.indexToAnyHitShader : VK_SHADER_UNUSED_NV;

        if (groupInfo.type == RaytraceShaderGroupType::TRIANGLES_HIT)
        {
          gi.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
          gi.intersectionShader = VK_SHADER_UNUSED_NV;
        }
        else // if (groupInfo.type == RaytraceShaderGroupType::PROCEDURAL_HIT)
        {
          gi.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV;
          gi.intersectionShader = groupInfo.indexToIntersecionShader;
        }
      }

      return gi;
    });

  VkRayTracingPipelineCreateInfoNV rpci;
  rpci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
  rpci.pNext = nullptr;
  rpci.flags = 0;
  rpci.stageCount = info.shader_count;
  rpci.pStages = shaderInfos.data();
  rpci.groupCount = info.group_count;
  rpci.pGroups = groupInfos.data();
  rpci.maxRecursionDepth = info.max_recursion;
  rpci.layout = layout->handle;
  rpci.basePipelineHandle = VK_NULL_HANDLE;
  rpci.basePipelineIndex = 0;

#if !VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  int64_t compilationTime = 0;
#endif

  {
    TIME_PROFILE(vulkan_rt_pipeline_compile);
    ScopedTimer compilationTimer(compilationTime);
    VULKAN_EXIT_ON_FAIL(dev.vkCreateRayTracingPipelinesNV(dev.get(), cache, 1, &rpci, nullptr, ptr(handle)));
  }

#if VULKAN_LOG_PIPELINE_ACTIVITY < 1
  if (compilationTime > PIPELINE_COMPILATION_LONG_THRESHOLD)
#endif
  {
    debug("vulkan: pipeline [raytrace:%u] compiled in %u us", prog.get(), compilationTime);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    debug("vulkan: handle %p, with shaders", generalize(handle));
    for (uint32_t i = 0; i < info.shader_count; ++i)
      debug("vulkan: %u = %s", i, debugInfo[i].debugName);
#endif
  }
}

VulkanPipelineHandle RaytracePipeline::getHandleForUse()
{
#if VULKAN_LOG_PIPELINE_ACTIVITY > 1
  debug("vulkan: bind raytrace prog %i", program.get());
#endif
  return getHandle();
}

void RaytracePipeline::copyRaytraceShaderGroupHandlesToMemory(VulkanDevice &dev, uint32_t first_group, uint32_t group_count,
  uint32_t size, void *ptr)
{
  VULKAN_EXIT_ON_FAIL(dev.vkGetRayTracingShaderGroupHandlesNV(dev.get(), handle, first_group, group_count, size, ptr));
}