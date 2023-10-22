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
  const GraphicsPipelineVariantDescription &dsc, unsigned &inOutTotalCompilationTime)
{
  auto hash = dsc.getHash();
  auto eDsc = variations.get(dsc, hash, comp_ctx.rsBackend, comp_ctx.nativeRP);

  GraphicsPipeline *ret;
  int64_t compilationTime = 0;

  GraphicsPipeline::CreationFeedback crFeedback;
  {
    ScopedTimer compileTimer(compilationTime);

    VulkanPipelineHandle parentPipe;
    if (!items.empty())
      parentPipe = items[0].second->getHandle();

    {
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
      String shortDebugName(128, "%s-%s", debugInfo.vs().name, debugInfo.fs().name);
      TIME_PROFILE_NAME(vulkan_gr_pipeline_compile, shortDebugName)
#else
      TIME_PROFILE(vulkan_gr_pipeline_compile)
#endif

      ret = new GraphicsPipeline(comp_ctx.dev, comp_ctx.pipeCache, layout,
        {comp_ctx.passMan, comp_ctx.rsBackend, eDsc.base, eDsc.mask, modules, crFeedback, parentPipe, comp_ctx.nativeRP});
    }

    items.push_back(eastl::make_pair(hash, ret));
  }
  inOutTotalCompilationTime += compilationTime;

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String fullDebugName(512, "vs: %s\nps: %s\nvaridx: %u", debugInfo.vs().debugName, debugInfo.fs().debugName, eDsc.index);
#endif

  if (is_null(ret->getHandle()))
  {
    logerr("vulkan: pipeline [gfx:%u:%u(%u)] not compiled but result was ok", program.get(), eDsc.index, items.size());
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    logerr("vulkan: with\n %s", fullDebugName);
#endif
    return ret;
  }

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  get_device().setPipelineName(ret->getHandle(), fullDebugName.c_str());
  if (items.size() == 1)
    get_device().setPipelineLayoutName(getLayout()->handle, fullDebugName.c_str());
  totalCompilationTime += compilationTime;
  ++variantCount;
#endif

#if VULKAN_LOG_PIPELINE_ACTIVITY < 1
  if (compilationTime > PIPELINE_COMPILATION_LONG_THRESHOLD)
#endif
  {
    debug("vulkan: pipeline [gfx:%u:%u(%u)] compiled in %u us", program.get(), eDsc.index, items.size(), compilationTime);
    crFeedback.logFeedback();
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    debug("vulkan: with\n %s handle: %p", fullDebugName, generalize(ret->getHandle()));
#endif
  }

  return ret;
}

GraphicsPipeline *VariatedGraphicsPipeline::getVariant(CompilationContext &comp_ctx, const GraphicsPipelineVariantDescription &dsc,
  bool compilationTimeout, unsigned &inOutTotalCompilationTime)
{
  GraphicsPipeline *pipe = findVariant(dsc);
  if (compilationTimeout && !pipe)
    return nullptr;

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
      pipe = compileNewVariant(comp_ctx, dsc, inOutTotalCompilationTime);
    else
    {
      // register/find new input layout
      // it is fine as input layout does not need registration in execution context
      modDsc.state.inputLayout = spdb.registerInputLayout(get_device().getContext(), maskedLayout);

      pipe = findVariant(modDsc);
      if (!pipe)
        pipe = compileNewVariant(comp_ctx, modDsc, inOutTotalCompilationTime);

      // add to list as mapped to another desc
      auto origHash = dsc.getHash();
      variations.get(dsc, origHash, comp_ctx.rsBackend, comp_ctx.nativeRP);
      items.push_back(eastl::make_pair(origHash, pipe));
      pipe->addRef();
    }
  }

  if (is_null(pipe->getHandle()))
  {
    return nullptr;
  }

  return pipe;
}
