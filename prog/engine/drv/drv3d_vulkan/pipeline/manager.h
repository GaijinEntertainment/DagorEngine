// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <EASTL/type_traits.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING

#include "descriptor_set.h"
#include "render_pass.h"
#include "pipeline/base_pipeline.h"
#include "pipeline/main_pipelines.h"
#include "pipeline/variated_graphics.h"
#include "raytrace/pipeline.h"

namespace drv3d_vulkan
{

template <typename PipelineType>
class PipelineManagerStorage
{
  typedef typename PipelineType::LayoutType LayoutType;
  typedef typename PipelineType::ProgramType ProgramType;

  Tab<LayoutType *> layouts;
  Tab<PipelineType *> pipelines;

  LayoutType *findOrAddLayout(VulkanDevice &device, const typename LayoutType::CreationInfo &layoutInfo)
  {
    for (auto layout : layouts)
    {
      if (layout->matches(layoutInfo))
        return layout;
    }

    auto ret = new LayoutType(device, layoutInfo);
    layouts.push_back(ret);
    return ret;
  }

  void ensureSpaceForIndex(LinearStorageIndex index)
  {
    auto storageExtend = pipelines.size() <= index ? (index - pipelines.size() + 1) : 0;
    if (storageExtend)
    {
      append_items(pipelines, storageExtend);
      mem_set_0(make_span(pipelines).last(storageExtend));
    }
  }

  template <bool tightlyPacked, typename ArrayType>
  void shutdownArray(VulkanDevice &device, ArrayType &arr)
  {
    for (auto i : arr)
      if (i || tightlyPacked)
        i->shutdown(device);
  }

public:
  PipelineManagerStorage() = default;
  PipelineManagerStorage(const PipelineManagerStorage &) = delete;

  template <typename F>
  void enumerate(F callback)
  {
    for (std::size_t i = 0; i < pipelines.size(); ++i)
      if (pipelines[i])
        callback(*pipelines[i], ProgramType::makeID(static_cast<LinearStorageIndex>(i)));
  }

  template <typename T>
  void enumerateLayouts(T clb)
  {
    for (auto &&l : layouts)
      clb(*l);
  }

  void add(VulkanDevice &device, ProgramID program, VulkanPipelineCacheHandle cache, const typename PipelineType::CreationInfo &info)
  {
    G_ASSERT(ProgramType::checkID(program));

    LayoutType *layout = findOrAddLayout(device, info.layout);

    LinearStorageIndex index = ProgramType::getIndexFromID(program);
    ensureSpaceForIndex(index);

    G_ASSERT(!pipelines[index]);
    pipelines[index] = new PipelineType(device, program, cache, layout, info);
  }

  // take object from storage and replace with nullptr
  PipelineType *takeOut(LinearStorageIndex index)
  {
    PipelineType *ret = pipelines[index];
    pipelines[index] = nullptr;

    G_ASSERT(ret);
    return ret;
  }

  void unload(VulkanDevice &device)
  {
    shutdownArray<false>(device, pipelines);
    clear_all_ptr_items_and_shrink(pipelines);

    shutdownArray<true>(device, layouts);
    clear_all_ptr_items_and_shrink(layouts);
  }

  PipelineType &get(ProgramID program)
  {
    G_ASSERT(ProgramType::checkID(program));

    return *pipelines[ProgramType::getIndexFromID(program)];
  }

  bool valid(ProgramID program)
  {
    const LinearStorageIndex index = ProgramType::getIndexFromID(program);
    return ProgramType::checkID(program) && index >= 0 && index < pipelines.size() && pipelines[index] != nullptr;
  }
};

class PipelineManager
{
  GraphicsPipelineVariationStorage graphicVariations;

  template <typename T>
  PipelineManagerStorage<T> &storage()
  {
    return get_storage(eastl::type_identity<T>{});
  }

#define PIPE_STORAGE_ENTRY(PipelineType, name)                                          \
  PipelineManagerStorage<PipelineType> name;                                            \
  PipelineManagerStorage<PipelineType> &get_storage(eastl::type_identity<PipelineType>) \
  {                                                                                     \
    return name;                                                                        \
  }

  PIPE_STORAGE_ENTRY(VariatedGraphicsPipeline, graphics);
  PIPE_STORAGE_ENTRY(ComputePipeline, compute);
#if D3D_HAS_RAY_TRACING
  PIPE_STORAGE_ENTRY(RaytracePipeline, raytrace);
#endif

#undef PIPE_STORAGE_ENTRY

  using HashedSeenGraphicsPipesMap =
    ska::flat_hash_map<VariatedGraphicsPipeline::CreationInfo::Hash, GraphicsPipelineShaderSet<spirv::HashValue>>;
  HashedSeenGraphicsPipesMap seenGraphicsPipes;

  void trackGrPipelineSeenStatus(VariatedGraphicsPipeline::CreationInfo &ci);

public:
  enum AsyncMask
  {
    ASYNC_MASK_NONE = 0,
    ASYNC_MASK_RENDER = 1,
    ASYNC_MASK_COMPUTE = 2,
    ASYNC_MASK_ALL = 3
  };

  template <typename PipelineType>
  PipelineType &get(ProgramID id)
  {
    return storage<PipelineType>().get(id);
  }

  template <typename PipelineType, typename F>
  void enumerate(F callback)
  {
    return storage<PipelineType>().enumerate(std::move(callback));
  }

  template <typename PipelineType, typename T>
  void enumerateLayouts(T clb)
  {
    return storage<PipelineType>().enumerateLayouts(clb);
  }

  /**
   * Polymorphically visits a pipeline object. `func` should  be a functional object that has operator()
   * overloads accepting all pipeline types (right now, VariatedGraphics, Compute and maybe Raytracing)
   * If program ID was invalid, returns false and does nothing, otherwise returns true.
   */
  template <typename F>
  bool visit(ProgramID program, F &&func)
  {
#define PROCESS_PIPELINE_STORAGE_ENTRY(Type)               \
  do                                                       \
  {                                                        \
    if (Type::ProgramType::checkID(program))               \
    {                                                      \
      if (!storage<Type>().valid(program))                 \
        return false;                                      \
      std::forward<F>(func)(storage<Type>().get(program)); \
      return true;                                         \
    }                                                      \
  } while (false)


    PROCESS_PIPELINE_STORAGE_ENTRY(VariatedGraphicsPipeline);
    PROCESS_PIPELINE_STORAGE_ENTRY(ComputePipeline);

#undef PROCESS_PIPELINE_STORAGE_ENTRY

    return false;
  }

  void addCompute(VulkanDevice &device, VulkanPipelineCacheHandle cache, ProgramID program, const ShaderModuleBlob &sci,
    const ShaderModuleHeader &header);
  void addGraphics(VulkanDevice &device, ProgramID program, const ShaderModule &vs_module, const ShaderModuleHeader &vs_header,
    const ShaderModule &fs_module, const ShaderModuleHeader &fs_header, const ShaderModule *gs_module,
    const ShaderModuleHeader *gs_header, const ShaderModule *tc_module, const ShaderModuleHeader *tc_header,
    const ShaderModule *te_module, const ShaderModuleHeader *te_header);
  void unloadAll(VulkanDevice &device);
  void prepareRemoval(ProgramID program);
  void setAsyncCompile(AsyncMask allowed) { asyncCompileAllowed = allowed; }
  bool asyncCompileEnabledGR() { return (asyncCompileAllowed & ASYNC_MASK_RENDER) > 0; }
  bool asyncCompileEnabledCS() { return (asyncCompileAllowed & ASYNC_MASK_COMPUTE) > 0; }

  static VulkanShaderModuleHandle makeVkModule(const ShaderModuleBlob *module);

private:
  AsyncMask asyncCompileAllowed = ASYNC_MASK_NONE;
};

} // namespace drv3d_vulkan