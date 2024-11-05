// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include "manager.h"
#include "globals.h"
#include "backend.h"
#include "driver_config.h"
#include "timelines.h"

using namespace drv3d_vulkan;

void PipelineManager::addCompute(VulkanDevice &device, VulkanPipelineCacheHandle cache, ProgramID program, const ShaderModuleBlob &sci,
  const ShaderModuleHeader &header)
{
  ComputePipeline::CreationInfo info = {&sci, {{&header}}, asyncCompileEnabledCS()};
  compute.add(device, program, cache, info);
}

void PipelineManager::trackGrPipelineSeenStatus(VariatedGraphicsPipeline::CreationInfo &ci)
{
  if (!Globals::cfg.bits.compileSeenGrPipelinesUsingSyncCompile)
    return;

  VariatedGraphicsPipeline::CreationInfo::Hash ciHash = ci.hash();
  HashedSeenGraphicsPipesMap::iterator lookupResult = seenGraphicsPipes.find(ciHash);
  if (lookupResult != seenGraphicsPipes.end())
  {
    ci.seenBefore = true;
    for (int i = 0; i < spirv::graphics::MAX_SETS; ++i)
    {
      if (!ci.modules.list[i])
        continue;

      if (ci.modules.list[i]->hash != lookupResult->second.list[i])
      {
        const int BUF_SZ = 20;
        char lBuf[BUF_SZ];
        char rBuf[BUF_SZ];
        ci.modules.list[i]->hash.convertToString(lBuf, BUF_SZ);
        lookupResult->second.list[i].convertToString(rBuf, BUF_SZ);
        D3D_ERROR("vulkan: hash collistion on seen pipelines tracker %016llX hash same for different shader %u pairs %s and %s",
          ciHash, i, lBuf, rBuf);
        ci.seenBefore = false;
        break;
      }
    }
  }
  else
  {
    GraphicsPipelineShaderSet<spirv::HashValue> moduleHashes;
    for (int i = 0; i < spirv::graphics::MAX_SETS; ++i)
    {
      if (!ci.modules.list[i])
      {
        moduleHashes.list[i].clear();
        continue;
      }
      else
        moduleHashes.list[i] = ci.modules.list[i]->hash;
    }
    seenGraphicsPipes[ciHash] = moduleHashes;
  }
}

void PipelineManager::addGraphics(VulkanDevice &device, ProgramID program, const ShaderModule &vs_module,
  const ShaderModuleHeader &vs_header, const ShaderModule &fs_module, const ShaderModuleHeader &fs_header,
  const ShaderModule *gs_module, const ShaderModuleHeader *gs_header, const ShaderModule *tc_module,
  const ShaderModuleHeader *tc_header, const ShaderModule *te_module, const ShaderModuleHeader *te_header)
{
  VariatedGraphicsPipeline::CreationInfo info = {{&vs_module, &fs_module, gs_module, tc_module, te_module}, graphicVariations,
    {{&vs_header, &fs_header, gs_header, tc_header, te_header}}, /*seenBefore*/ false};

  trackGrPipelineSeenStatus(info);

  graphics.add(device, program, VulkanPipelineCacheHandle(), info);
}

#if D3D_HAS_RAY_TRACING
void PipelineManager::copyRaytraceShaderGroupHandlesToMemory(VulkanDevice &dev, ProgramID prog, uint32_t first_group,
  uint32_t group_count, uint32_t size, void *ptr)
{
  G_UNUSED(dev);
  G_UNUSED(prog);
  G_UNUSED(first_group);
  G_UNUSED(group_count);
  G_UNUSED(size);
  G_UNUSED(ptr);
  G_ASSERTF(false, "PipelineManager::copyRaytraceShaderGroupHandlesToMemory called on API without support");
}

void PipelineManager::addRaytrace(VulkanDevice &device, VulkanPipelineCacheHandle cache, ProgramID id, uint32_t max_recursion,
  uint32_t shader_count, const ShaderModuleUse *shaders, uint32_t group_count, const RaytraceShaderGroup *groups,
  const eastl::unique_ptr<ShaderModule> *module_set)
{
  G_UNUSED(device);
  G_UNUSED(cache);
  G_UNUSED(id);
  G_UNUSED(max_recursion);
  G_UNUSED(shader_count);
  G_UNUSED(shaders);
  G_UNUSED(group_count);
  G_UNUSED(groups);
  G_UNUSED(module_set);
  G_ASSERTF(false, "PipelineManager::addRaytrace called on API without support");
}
#endif

void PipelineManager::unloadAll(VulkanDevice &device)
{
  graphics.unload(device);
  compute.unload(device);
#if D3D_HAS_RAY_TRACING
  raytrace.unload(device);
#endif
  seenGraphicsPipes.clear();
}

void PipelineManager::prepareRemoval(ProgramID program)
{
  CleanupQueue &cleanups = Backend::gpuJob.get().cleanups;
  auto index = program_to_index(program);
  switch (get_program_type(program))
  {
#if D3D_HAS_RAY_TRACING
    case program_type_raytrace: cleanups.enqueueFromBackend(*raytrace.takeOut(index)); break;
#endif
    case program_type_graphics: cleanups.enqueueFromBackend(*graphics.takeOut(index)); break;
    case program_type_compute: cleanups.enqueueFromBackend(*compute.takeOut(index)); break;
    default: G_ASSERTF(false, "Broken program type"); return;
  }
}

VulkanShaderModuleHandle PipelineManager::makeVkModule(const ShaderModuleBlob *module)
{
  VkShaderModuleCreateInfo smci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0};
  smci.codeSize = module->getBlobSize();
  smci.pCode = module->blob.data();

  VulkanShaderModuleHandle shader = VulkanShaderModuleHandle();
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateShaderModule(Globals::VK::dev.get(), &smci, NULL, ptr(shader)));

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  Globals::Dbg::naming.setShaderModuleName(shader, module->name.c_str());
#endif
  return shader;
}
