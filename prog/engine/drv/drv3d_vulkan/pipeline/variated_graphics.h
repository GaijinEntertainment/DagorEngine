// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"
#include <EASTL/hash_map.h>
#include <EASTL/map.h>
#include "main_pipelines.h"

namespace drv3d_vulkan
{
class ProgramDatabase;

class GraphicsPipelineVariationStorage
{

public:
  typedef uint32_t Index;

  struct ExtendedVariantDescription
  {
    GraphicsPipelineVariantDescription base;
    GraphicsPipelineDynamicStateMask mask;
    Index index;
  };

  ExtendedVariantDescription &get(const GraphicsPipelineVariantDescription &dsc, GraphicsPipelineVariantDescription::Hash hash,
    RenderStateSystemBackend &rs_backend, RenderPassResource *native_rp);

private:
  typedef eastl::pair<GraphicsPipelineVariantDescription::Hash, Index> KeyIndexPair;

  Index lastIndex = 0;
  eastl::vector<KeyIndexPair> indexArray;
  eastl::vector<ExtendedVariantDescription> dataArray;
};

class VariatedGraphicsPipeline : public DebugAttachedPipeline<GraphicsPipelineLayout, GraphicsProgram, CustomPipeline>
{

public:
  struct CompilationContext
  {
    VulkanDevice &dev;
    RenderStateSystemBackend &rsBackend;
    VulkanPipelineCacheHandle pipeCache;
    RenderPassResource *nativeRP;
    uint32_t *asyncPipeFeedbackPtr;
    bool nonDrawCompile;
  };

private:
  GraphicsPipeline *findVariant(const GraphicsPipelineVariantDescription &dsc);
  GraphicsPipeline *mapDublicate(GraphicsPipelineVariantDescription &dsc);
  GraphicsPipeline *compileNewVariant(CompilationContext &comp_ctx, const GraphicsPipelineVariantDescription &dsc);

public:
  typedef GraphicsPipelineLayout LayoutType;
  typedef GraphicsProgram ProgramType;

  struct CreationInfo
  {
    const GraphicsPipelineShaderSet<const ShaderModule *> &modules;
    GraphicsPipelineVariationStorage &varStorage;
    const LayoutType::CreationInfo &layout;
    bool seenBefore;

    typedef uint64_t Hash;

    Hash hash()
    {
      uint64_t ret = FNV1Params<64>::offset_basis;

      for (const ShaderModule *module : modules.list)
      {
        if (!module)
          continue;
        for (uint8_t iter : module->hash.value)
          ret = fnv1a_step<64>(iter, ret);
      }

      return ret;
    }

    CreationInfo() = delete;
  };

  // generic cleanup queue support
  static constexpr int CLEANUP_DESTROY = 0;

  template <int Tag>
  void onDelayedCleanupBackend()
  {}

  template <int Tag>
  void onDelayedCleanupFrontend()
  {}

  template <int Tag>
  void onDelayedCleanupFinish();

  const char *resTypeString() { return "VariatedGraphicsPipeline"; }

  VariatedGraphicsPipeline(VulkanDevice &, ProgramID inProg, VulkanPipelineCacheHandle, LayoutType *l, const CreationInfo &info) :
    DebugAttachedPipeline(l), modules(info.modules), variations(info.varStorage), program(inProg), seenBefore(info.seenBefore)
  {}

  GraphicsPipeline *getVariant(CompilationContext &comp_ctx, const GraphicsPipelineVariantDescription &dsc);

  void shutdown(VulkanDevice &device);

  bool hasGeometryStage() const { return layout->hasGS(); }

  bool hasTessControlStage() const { return layout->hasTC(); }

  bool hasTessEvaluationStage() const { return layout->hasTE(); }

  bool hasTesselationStage() const { return hasTessControlStage() && hasTessEvaluationStage(); }

  bool pendingCompilation();

private:
  GraphicsPipelineShaderSet<const ShaderModule *> modules;

  eastl::vector<eastl::pair<GraphicsPipelineVariantDescription::Hash, GraphicsPipeline *>> items;
  GraphicsPipelineVariationStorage &variations;
  ProgramID program;
  bool seenBefore;
};

} // namespace drv3d_vulkan