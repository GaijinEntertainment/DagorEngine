// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resources.h"
#include "backend/context.h"
#include "pipeline_cache.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdAddShaderModule &cmd) { Backend::shaderModules.add(cmd.sci, cmd.id); }
TSPEC void BEContext::execCmd(const CmdRemoveShaderModule &cmd) { Backend::shaderModules.remove(cmd.id); }

TSPEC void BEContext::execCmd(const CmdAddComputeProgram &cmd)
{
  Globals::pipelines.addCompute(Globals::pipeCache.getHandle(), cmd.program, *cmd.sci, cmd.header);
  delete cmd.sci;
}

TSPEC void BEContext::execCmd(const CmdAddGraphicsProgram &cmd)
{
  struct ShaderModules
  {
    ShaderModule *module = nullptr;
    ShaderModuleHeader *header = nullptr;
    ShaderModuleUse *use = nullptr;
  };

  auto getShaderModules = [this](uint32_t index) {
    ShaderModules modules;
    if (index < data->shaderModuleUses.size())
    {
      modules.use = &data->shaderModuleUses[index];
      modules.header = &modules.use->header;

      modules.module = Backend::shaderModules.find(modules.use->blobId);
    }
    return modules;
  };

  ShaderModules vsModules = getShaderModules(cmd.vs);
  ShaderModules fsModules = getShaderModules(cmd.fs);
  ShaderModules tcModules = getShaderModules(cmd.tc);
  ShaderModules teModules = getShaderModules(cmd.te);
  ShaderModules gsModules = getShaderModules(cmd.gs);

  Globals::pipelines.addGraphics(cmd.program, *vsModules.module, *vsModules.header, *fsModules.module, *fsModules.header,
    gsModules.module, gsModules.header, tcModules.module, tcModules.header, teModules.module, teModules.header);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  Globals::pipelines.get<VariatedGraphicsPipeline>(cmd.program)
    .setDebugInfo({{vsModules.use->dbg, fsModules.use->dbg, gsModules.use ? gsModules.use->dbg : GraphicsPipeline::emptyDebugInfo,
      tcModules.use ? tcModules.use->dbg : GraphicsPipeline::emptyDebugInfo,
      teModules.use ? teModules.use->dbg : GraphicsPipeline::emptyDebugInfo}});
#endif
}

TSPEC void BEContext::execCmd(const CmdAddPipelineCache &cmd)
{
  if (
    VULKAN_CHECK_OK(Globals::VK::dev.vkMergePipelineCaches(Globals::VK::dev.get(), Globals::pipeCache.getHandle(), 1, ptr(cmd.cache))))
    debug("vulkan: additional pipeline cache added");

  VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyPipelineCache(Globals::VK::dev.get(), cmd.cache, VKALLOC(pipeline_cache)));
}

TSPEC void BEContext::execCmd(const CmdAddRenderState &cmd) { Backend::renderStateSystem.setRenderStateData(cmd.id, cmd.data); }

TSPEC void BEContext::execCmd(const CmdRemoveProgram &cmd)
{
  if (!Backend::State::pendingCleanups.removeWithReferenceCheck(cmd.program, Backend::State::pipe))
    return;
  Globals::pipelines.prepareRemoval(cmd.program);
  Globals::shaderProgramDatabase.reuseId(cmd.program);
}

TSPEC void BEContext::execCmd(const CmdDestroyImage &cmd)
{
  cmd.image->markDead();
  Backend::gpuJob.get().cleanups.enqueue(*cmd.image);
}

TSPEC void BEContext::execCmd(const CmdDestroyRenderPassResource &cmd)
{
  cmd.rp->markDead();
  if (Backend::State::pendingCleanups.removeWithReferenceCheck(cmd.rp, Backend::State::pipe))
    Backend::gpuJob.get().cleanups.enqueue(*cmd.rp);
}

TSPEC void BEContext::execCmd(const CmdDestroyBuffer &cmd)
{
  cmd.buffer->markDead();
  if (Backend::State::pendingCleanups.removeWithReferenceCheck(cmd.buffer, Backend::State::pipe))
    Backend::gpuJob.get().cleanups.enqueue(*cmd.buffer);
}

TSPEC void BEContext::execCmd(const CmdDestroyBufferDelayed &cmd)
{
  cmd.buffer->markDead();
  Backend::State::pendingCleanups.removeReferenced(cmd.buffer);
}

TSPEC void BEContext::execCmd(const CmdDestroyHeap &cmd)
{
  cmd.heap->markDead();
  Backend::gpuJob.get().cleanups.enqueue(*cmd.heap);
}

TSPEC void BEContext::execCmd(const CmdDestroyTLAS &cmd)
{
  cmd.tlas->markDead();
  Backend::gpuJob.get().cleanups.enqueue<CleanupTag::DESTROY_TOP>(*cmd.tlas);
}

TSPEC void BEContext::execCmd(const CmdDestroyBLAS &cmd)
{
  cmd.blas->markDead();
  Backend::gpuJob.get().cleanups.enqueue<CleanupTag::DESTROY_BOTTOM>(*cmd.blas);
}
