#pragma once
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"
#include <EASTL/hash_map.h>
#include <EASTL/map.h>

namespace drv3d_vulkan
{
struct ContextBackend;
class Device;
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
    RenderStateSystem::Backend &rs_backend, RenderPassResource *native_rp);

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
    RenderPassManager &passMan;
    RenderStateSystem::Backend &rsBackend;
    VulkanPipelineCacheHandle pipeCache;
    RenderPassResource *nativeRP;
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

    CreationInfo() = delete;
  };

  // generic cleanup queue support
  static constexpr int CLEANUP_DESTROY = 0;

  template <int Tag>
  void onDelayedCleanupBackend(drv3d_vulkan::ContextBackend &)
  {}

  template <int Tag>
  void onDelayedCleanupFrontend()
  {}

  template <int Tag>
  void onDelayedCleanupFinish();

  const char *resTypeString() { return "VariatedGraphicsPipeline"; }

  VariatedGraphicsPipeline(VulkanDevice &, ProgramID inProg, VulkanPipelineCacheHandle, LayoutType *l, const CreationInfo &info) :
    DebugAttachedPipeline(l), modules(info.modules), variations(info.varStorage), program(inProg)
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
};

} // namespace drv3d_vulkan