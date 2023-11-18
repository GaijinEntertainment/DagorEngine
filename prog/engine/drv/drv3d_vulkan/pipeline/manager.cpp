#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include "device.h"

using namespace drv3d_vulkan;

void PipelineManager::addCompute(VulkanDevice &device, VulkanPipelineCacheHandle cache, ProgramID program, const ShaderModuleBlob &sci,
  const ShaderModuleHeader &header)
{
  ComputePipeline::CreationInfo info = {&sci, {{&header}}, asyncCompileEnabled()};
  compute.add(device, program, cache, info);
}

void PipelineManager::addGraphics(VulkanDevice &device, ProgramID program, const ShaderModule &vs_module,
  const ShaderModuleHeader &vs_header, const ShaderModule &fs_module, const ShaderModuleHeader &fs_header,
  const ShaderModule *gs_module, const ShaderModuleHeader *gs_header, const ShaderModule *tc_module,
  const ShaderModuleHeader *tc_header, const ShaderModule *te_module, const ShaderModuleHeader *te_header)
{
  VariatedGraphicsPipeline::CreationInfo info = {{&vs_module, &fs_module, gs_module, tc_module, te_module}, graphicVariations,
    {{&vs_header, &fs_header, gs_header, tc_header, te_header}}};

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
}

void PipelineManager::prepareRemoval(ProgramID program)
{
  CleanupQueue &cleanups = get_device().getContext().getBackend().contextState.frame.get().cleanups;
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