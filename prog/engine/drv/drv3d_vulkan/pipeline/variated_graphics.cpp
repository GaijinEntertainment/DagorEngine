#include "device.h"

namespace drv3d_vulkan
{

template <>
void VariatedGraphicsPipeline::onDelayedCleanupFinish<VariatedGraphicsPipeline::CLEANUP_DESTROY>()
{
  shutdown(get_device().getVkDevice());
  delete this;
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

GraphicsPipelineVariationStorage::ExtendedVariantDescription &GraphicsPipelineVariationStorage::get(
  const GraphicsPipelineVariantDescription &dsc, GraphicsPipelineVariantDescription::Hash hash, RenderStateSystem::Backend &rs_backend,
  RenderPassResource *native_rp)
{
  const auto itr = eastl::lower_bound(indexArray.begin(), indexArray.end(), eastl::make_pair(hash, 0),
    [](const KeyIndexPair &one, const KeyIndexPair &two) { return one.first < two.first; });

  if ((itr != indexArray.end()) && !(hash < itr->first))
  {
    auto &dataRef = dataArray.at(itr->second);
#if DAGOR_DBGLEVEL > 0
    if (!dataRef.base.compare(dsc))
    {
      dsc.print();
      dataRef.base.print();
      fatal("vulkan: graphics variation hash collision");
    }
#endif

    return dataRef;
  }
  else
  {
#if VULKAN_LOG_PIPELINE_ACTIVITY > 0
    debug("vulkan: new graphics pipeline variation, total %u", lastIndex + 1);
#endif

    GraphicsPipelineDynamicStateMask mask;
    mask.from(rs_backend, dsc, native_rp);

    indexArray.insert(itr, eastl::make_pair(hash, dataArray.size()));
    dataArray.push_back({dsc, mask, lastIndex++});
    return dataArray.back();
  }
}

bool VariatedGraphicsPipeline::pendingCompilation()
{
  for (const auto &[_, pipeline] : items)
  {
    if (!pipeline->checkCompiled())
      return true;
  }
  return false;
}

void VariatedGraphicsPipeline::shutdown(VulkanDevice &device)
{
  for (const auto &[_, pipeline] : items)
  {
    if (!pipeline->release())
    {
      if (!pipeline->checkCompiled())
        get_device().getContext().getBackend().pipelineCompiler.waitFor(pipeline);
      pipeline->shutdown(device);
      delete pipeline;
    }
  }
  items.clear();
}

GraphicsPipeline *VariatedGraphicsPipeline::findVariant(const GraphicsPipelineVariantDescription &dsc)
{
  auto hash = dsc.getHash();
#if 0
  //hash collision check
  variations.get(dsc, hash);
#endif

  auto ref = eastl::find_if(begin(items), end(items), [&hash](auto &iter) { return iter.first == hash; });

  return (ref != items.end()) ? ref->second : nullptr;
}

GraphicsPipeline *VariatedGraphicsPipeline::compileNewVariant(CompilationContext &comp_ctx,
  const GraphicsPipelineVariantDescription &dsc)
{
  auto hash = dsc.getHash();
  auto eDsc = variations.get(dsc, hash, comp_ctx.rsBackend, comp_ctx.nativeRP);

  GraphicsPipeline *ret;
  int64_t compilationTime = 0;

  GraphicsPipeline::CreationFeedback crFeedback;
  {
    ScopedTimer compileTimer(compilationTime);

    GraphicsPipeline *parentPipe = nullptr;
    if (!items.empty())
      parentPipe = items[0].second;

    bool async = get_device().pipeMan.asyncCompileEnabled();
    GraphicsPipelineCompileScratchData localCompileData;
    GraphicsPipelineCompileScratchData *csd = async ? new GraphicsPipelineCompileScratchData() : &localCompileData;
    memset(csd, 0, sizeof(GraphicsPipelineCompileScratchData));
    csd->allocated = async;

    csd->varIdx = eDsc.index;
    csd->varTotal = items.size();
    csd->progIdx = program.get();
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    csd->shortDebugName = String(128, "%s-%s", debugInfo.vs().name, debugInfo.fs().name);
    csd->fullDebugName = String(512, "vs: %s\nps: %s\nvaridx: %u", debugInfo.vs().debugName, debugInfo.fs().debugName, eDsc.index);
#endif

    ret = new GraphicsPipeline(comp_ctx.dev, comp_ctx.pipeCache, layout,
      {comp_ctx.passMan, comp_ctx.rsBackend, eDsc.base, eDsc.mask, modules, parentPipe, comp_ctx.nativeRP, csd});

    if (async)
      get_device().getContext().getBackend().pipelineCompiler.queue(ret);
    else
      ret->compile();

    items.push_back(eastl::make_pair(hash, ret));
  }
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  totalCompilationTime += compilationTime;
  ++variantCount;
#endif

  return ret;
}

GraphicsPipeline *VariatedGraphicsPipeline::getVariant(CompilationContext &comp_ctx, const GraphicsPipelineVariantDescription &dsc)
{
  GraphicsPipeline *pipe = findVariant(dsc);

#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  if (!isUsageAllowed())
    return nullptr;
#endif

#if VULKAN_LOG_PIPELINE_ACTIVITY > 1
  debug("vulkan: get gfx vs %s fs %s", debugInfo.vs().name, debugInfo.fs().name);
#endif

  if (!pipe)
  {
    // apply input mask & related things at compile time only
    // by mapping pipeline with x description to pipeline with y description
    // where x is original and y is resulted masked description
    ShaderProgramDatabase &spdb = get_shader_program_database();

    GraphicsPipelineVariantDescription modDsc = dsc;
    InputLayout originalLayout = spdb.getInputLayoutFromId(dsc.state.inputLayout);
    InputLayout maskedLayout = originalLayout;
    modDsc.state.maskInputs(layout->registers.vs().header.inputMask, maskedLayout);

    // masking did not changed layout, no need to map dsc to something
    if (originalLayout.isSame(maskedLayout))
      pipe = compileNewVariant(comp_ctx, dsc);
    else
    {
      // register/find new input layout
      // it is fine as input layout does not need registration in execution context
      modDsc.state.inputLayout = spdb.registerInputLayout(get_device().getContext(), maskedLayout);

      pipe = findVariant(modDsc);
      if (!pipe)
        pipe = compileNewVariant(comp_ctx, modDsc);

      // add to list as mapped to another desc
      auto origHash = dsc.getHash();
      variations.get(dsc, origHash, comp_ctx.rsBackend, comp_ctx.nativeRP);
      items.push_back(eastl::make_pair(origHash, pipe));
      pipe->addRef();
    }
  }

  if (!pipe->checkCompiled())
    return nullptr;

  return pipe;
}
