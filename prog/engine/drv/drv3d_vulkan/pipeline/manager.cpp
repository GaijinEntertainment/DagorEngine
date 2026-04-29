// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include "manager.h"
#include "globals.h"
#include "backend.h"
#include "driver_config.h"
#include "timelines.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

void PipelineManager::addCompute(VulkanPipelineCacheHandle cache, ProgramID program, const ShaderModuleBlob &sci,
  const ShaderModuleHeader &header)
{
  ComputePipeline::CreationInfo info = {&sci, {{&header}}, asyncCompileEnabledCS()};
  compute.add(program, cache, info);
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

void PipelineManager::addGraphics(ProgramID program, const ShaderModule &vs_module, const ShaderModuleHeader &vs_header,
  const ShaderModule &fs_module, const ShaderModuleHeader &fs_header, const ShaderModule *gs_module,
  const ShaderModuleHeader *gs_header, const ShaderModule *tc_module, const ShaderModuleHeader *tc_header,
  const ShaderModule *te_module, const ShaderModuleHeader *te_header)
{
  VariatedGraphicsPipeline::CreationInfo info = {{&vs_module, &fs_module, gs_module, tc_module, te_module}, graphicVariations,
    {{&vs_header, &fs_header, gs_header, tc_header, te_header}}, /*seenBefore*/ false};

  trackGrPipelineSeenStatus(info);

  graphics.add(program, VulkanPipelineCacheHandle(), info);
}

void PipelineManager::unloadAll()
{
  graphics.unload();
  compute.unload();
  seenGraphicsPipes.clear();
}

void PipelineManager::onDeviceReset()
{
  seenGraphicsPipes.clear();

  graphics.enumerate([](auto &pipeline, auto) { pipeline.shutdown(); });
  graphics.enumerateLayouts([](auto &layout) { layout.onDeviceReset(); });
  compute.enumerate([](auto &pipeline, auto) { pipeline.onDeviceReset(); });
  compute.enumerateLayouts([](auto &layout) { layout.onDeviceReset(); });
}

void PipelineManager::afterDeviceReset()
{
  graphics.enumerateLayouts([](auto &layout) { layout.afterDeviceReset(); });
  compute.enumerateLayouts([](auto &layout) { layout.afterDeviceReset(); });
  compute.enumerate([](auto &pipeline, auto) { pipeline.afterDeviceReset(false); });
}

void PipelineManager::prepareRemoval(ProgramID program)
{
  CleanupQueue &cleanups = Backend::gpuJob.get().cleanups;
  auto index = program_to_index(program);
  switch (get_program_type(program))
  {
    case program_type_graphics: cleanups.enqueue(*graphics.takeOut(index)); break;
    case program_type_compute: cleanups.enqueue(*compute.takeOut(index)); break;
    default: G_ASSERTF(false, "Broken program type"); return;
  }
}

VulkanShaderModuleHandle PipelineManager::makeVkModule(const ShaderModuleBlob *module, spirv::HashValue &hv)
{
  Tab<uint8_t> tmpstorage(framemem_ptr());
  module->source.uncompress(tmpstorage);

  Tab<uint8_t> tmpstorageSmolv(framemem_ptr());
  if (module->sizeSmolv)
  {
    G_ASSERT(module->sizeSmolv <= tmpstorage.size());
    uint32_t decodedSize = smolv::GetDecodedBufferSize(tmpstorage.data() + module->offset, module->sizeSmolv);
    tmpstorageSmolv.resize(decodedSize);
    G_VERIFY(smolv::Decode(tmpstorage.data() + module->offset, module->sizeSmolv, tmpstorageSmolv.data(), tmpstorageSmolv.size()));
  }
  const auto &src = module->sizeSmolv ? tmpstorageSmolv : tmpstorage;
  const uint32_t size = module->sizeSmolv == 0 && module->size ? module->size : src.size();
  const uint32_t offset = module->sizeSmolv ? 0 : module->offset;
  G_ASSERT(offset + size <= src.size());
  hv = spirv::HashValue::calculate(src.data() + offset, size);

  VkShaderModuleCreateInfo smci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0};
  smci.codeSize = size;
  smci.pCode = (const uint32_t *)(src.data() + offset);

  VulkanShaderModuleHandle shader = VulkanShaderModuleHandle();
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateShaderModule(Globals::VK::dev.get(), &smci, VKALLOC(shader_module), ptr(shader)));

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  Globals::Dbg::naming.setShaderModuleName(shader, module->name.c_str());
#endif
  return shader;
}
