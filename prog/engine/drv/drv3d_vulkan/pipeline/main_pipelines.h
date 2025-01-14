// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"
#include "base_pipeline.h"
#include "shader/pipeline_description.h"
#include "render_state_system.h"
#include <util/dag_hash.h>
#include "render_pass_resource.h"
#include "compiler_scratch_data.h"
#include "sampler_resource.h"
#include "bindless_common.h"
#include "shader/module.h"

namespace drv3d_vulkan
{

class ProgramDatabase;
class RenderPassResource;

template <typename fieldType>
struct ComputePipelineShaderSet
{
  fieldType &cs() { return list[spirv::compute::REGISTERS_SET_INDEX]; }

  fieldType list[spirv::compute::MAX_SETS];
};

template <typename fieldType>
struct GraphicsPipelineShaderSet
{
  fieldType &vs() { return list[spirv::graphics::vertex::REGISTERS_SET_INDEX]; }
  fieldType &fs() { return list[spirv::graphics::fragment::REGISTERS_SET_INDEX]; }
  fieldType &gs() { return list[spirv::graphics::geometry::REGISTERS_SET_INDEX]; }
  fieldType &tc() { return list[spirv::graphics::control::REGISTERS_SET_INDEX]; }
  fieldType &te() { return list[spirv::graphics::evaluation::REGISTERS_SET_INDEX]; }

  fieldType list[spirv::graphics::MAX_SETS];
};

struct PipelineBindlessConfig
{
  static uint32_t bindlessSetCount;
  static BindlessSetLayouts bindlessSetLayouts;
};

struct ComputePipelineShaderConfig : PipelineBindlessConfig
{
  static const VkPipelineBindPoint bind = VK_PIPELINE_BIND_POINT_COMPUTE;
  static const uint32_t count = spirv::compute::MAX_SETS;
  static const VkShaderStageFlagBits stages[count];
  static const uint32_t registerIndexes[count];
};

struct GraphicsPipelineShaderConfig : PipelineBindlessConfig
{
  static const VkPipelineBindPoint bind = VK_PIPELINE_BIND_POINT_GRAPHICS;
  static const uint32_t count = spirv::graphics::MAX_SETS;
  static const VkShaderStageFlagBits stages[count];
  static const uint32_t registerIndexes[count];
};

typedef BasePipelineLayout<ComputePipelineShaderSet, ComputePipelineShaderConfig> ComputePipelineLayout;

typedef BasePipelineLayout<GraphicsPipelineShaderSet, GraphicsPipelineShaderConfig> GraphicsPipelineLayoutBase;

class GraphicsPipelineLayout : public GraphicsPipelineLayoutBase
{
public:
  GraphicsPipelineLayout(VulkanDevice &device, const CreationInfo &headers) : GraphicsPipelineLayoutBase(device, headers) {}

  bool hasGS() { return shaderStages & VK_SHADER_STAGE_GEOMETRY_BIT; }
  bool hasTC() { return shaderStages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; }
  bool hasTE() { return shaderStages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; }
};

template <size_t MaxStagesCount>
class CreationFeedbackBase
{
#if VK_EXT_pipeline_creation_feedback
public:
  void chainWith(VkGraphicsPipelineCreateInfo &gpci, const VulkanDevice &device) { chainWithInternal(gpci.stageCount, gpci, device); }

  void chainWith(VkComputePipelineCreateInfo &cpci, const VulkanDevice &device) { chainWithInternal(1, cpci, device); }

  void logFeedback() const
  {
    G_ASSERT(stagesCount <= MaxStagesCount);
    const char *prefix = "vulkan: pipeline creation feedback:";


    if (!(pipelineFeedback.flags & VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT))
    {
      debug("%s pipeline feedback is not valid", prefix);
      return;
    }

    if (pipelineFeedback.flags & VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT)
      debug("%s pipeline hit cache: yes", prefix);

    String stagesFb;
    for (size_t i = 0; i < stagesCount; ++i)
    {
      if (stagesFeedback[i].flags & VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT)
      {
        const bool hitCache = stagesFeedback[i].flags & VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
        stagesFb += String(40, "    stage [%d]: hit cache: %s\n", i, hitCache ? "yes" : "no");
      }
      else
        stagesFb += String(45, "    stage [%d] doesn't have a feedback \n", i);
    }

    debug("%s \n  %s\n%s", prefix, "pipeline hit cache: no", stagesFb.c_str());
  }

private:
  template <class VkPipelineCreateInfo>
  void chainWithInternal(const size_t stgsCount, VkPipelineCreateInfo &pipelineCi, const VulkanDevice &device)
  {
    stagesCount = stgsCount;

    if (device.hasExtension<PipelineCreationFeedbackReportEXT>())
    {
      fillCreateInfo(stagesCount);
      chain_structs(pipelineCi, createInfo);
    }
  }

  void fillCreateInfo(const size_t stages)
  {
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
    createInfo.pNext = NULL;
    createInfo.pPipelineCreationFeedback = &pipelineFeedback;
    createInfo.pipelineStageCreationFeedbackCount = stages;
    createInfo.pPipelineStageCreationFeedbacks = stagesFeedback.data();
  }

  VkPipelineCreationFeedbackEXT pipelineFeedback = {0, 0};
  eastl::array<VkPipelineCreationFeedbackEXT, MaxStagesCount> stagesFeedback;
  size_t stagesCount = 0;
  VkPipelineCreationFeedbackCreateInfoEXT createInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT, NULL, NULL, 0, NULL};

#else
  void chainWith(VkGraphicsPipelineCreateInfo &, const VulkanDevice &) {}
  void chainWith(VkComputePipelineCreateInfo &, const VulkanDevice &) {}
  void logFeedback() const {}
#endif
};

class ComputePipeline : public DebugAttachedPipeline<ComputePipelineLayout, ComputeProgram, BasePipeline>
{
public:
  using CreationFeedback = CreationFeedbackBase<spirv::compute::MAX_SETS>;

  struct CreationInfo
  {
    const ShaderModuleBlob *sci;
    const LayoutType::CreationInfo &layout;
    const bool allowAsyncCompile;

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

  const char *resTypeString() { return "ComputePipeline"; }

  ComputePipeline(VulkanDevice &dev, ProgramID prog, VulkanPipelineCacheHandle cache, LayoutType *l, const CreationInfo &info);
  VulkanPipelineHandle getHandleForUse();

  void compile();
  bool pendingCompilation();

private:
  static const uint32_t workGroupDims = 3;
  static uint32_t spirvWorkGroupSizeDimConstantIds[workGroupDims];

  ComputePipelineCompileScratchData *compileScratch;
};

struct GraphicsPipelineVariantDescription
{
  GraphicsPipelineStateDescription state;
  RenderPassClass::Identifier rpClass;
  uint64_t nativeRPhash;
  uint32_t subpass;
  VkPrimitiveTopology topology;

  typedef uint64_t Hash;

#if DAGOR_DBGLEVEL > 0
  bool compare(const GraphicsPipelineVariantDescription &l) const
  {
    if (!state.compare(l.state, true))
      return false;
    if (!nativeRPhash)
    {
      if (l.nativeRPhash)
        return false;
      if (rpClass != l.rpClass)
        return false;
    }
    else
    {
      if (!l.nativeRPhash)
        return false;
      if (nativeRPhash != l.nativeRPhash)
        return false;
      if (subpass != l.subpass)
        return false;
    }
    if (topology != l.topology)
      return false;

    return true;
  }

  void print() const
  {
    uint16_t trimmedRS = state.renderState.staticIdx;

    debug("vulkan: gfx pipeline variation desc: %04X %02X %02X %02X %02X %08X %016lX %02X %02X %02X | %016llX [%u]", trimmedRS,
      state.strides[0], state.strides[1], state.strides[2], state.strides[3], // hacky, but will work for debug
      state.inputLayout.get(), *((uint64_t *)&rpClass.colorFormats[0]), (uint8_t)rpClass.depthStencilFormat,
      (uint8_t)rpClass.colorTargetMask, (topology << 3) | (state.polygonLine << 2) | rpClass.depthState, nativeRPhash, subpass);
  }
#endif

  template <typename T>
  void fnv1_helper(T *data, Hash &hash) const
  {
    uint8_t *dt = (uint8_t *)data;
    for (int i = 0; i < sizeof(T); ++i)
      hash = fnv1a_step<64>(dt[i], hash);
  }

  Hash getHash() const
  {
    // 65k states should be enough
    G_ASSERT(state.renderState.staticIdx < 0xFFFF);

    Hash ret = FNV1Params<64>::offset_basis;

    // we can't bulk hash this data as we have different paddings
    // on different platforms, that will garbage-contribute
    // to hash value, so hash data by fields
    // and save some iterations by mixing/trimming/ignoring some of fields

    // we don't care about dyn state, and trim some likely zeroes
    uint16_t trimmedRS = state.renderState.staticIdx;
    fnv1_helper(&trimmedRS, ret);

    for (int i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
      fnv1_helper(&state.strides[i], ret);
    fnv1_helper(&state.inputLayout, ret);

    if (!nativeRPhash)
    {
      for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
        ret = fnv1a_step<64>((uint8_t)rpClass.colorFormats[i], ret);
      ret = fnv1a_step<64>((uint8_t)rpClass.depthStencilFormat, ret);
      ret = fnv1a_step<64>((uint8_t)rpClass.colorTargetMask, ret);
      ret = fnv1a_step<64>((uint8_t)rpClass.depthSamples, ret);
      ret = fnv1a_step<64>((uint32_t)(rpClass.colorSamplesPacked >> 32), ret);        // high bits
      ret = fnv1a_step<64>((uint32_t)(rpClass.colorSamplesPacked & 0xffffffff), ret); // low bits

      // save 2 bytes by mixing booleans and small topo value
      ret = fnv1a_step<64>((topology << 3) | (state.polygonLine << 2) | rpClass.depthState, ret);
    }
    else
    {
      uint64_t h = nativeRPhash;
      fnv1_helper(&h, ret);
      ret = fnv1a_step<64>((uint8_t)subpass, ret);
      ret = fnv1a_step<64>((topology << 1) | state.polygonLine, ret);
    }

    return ret;
  }
};

BEGIN_BITFIELD_TYPE(GraphicsPipelineDynamicStateMask, uint8_t)
  ADD_BITFIELD_MEMBER(hasDepthBias, 0, 1)
  ADD_BITFIELD_MEMBER(hasDepthBoundsTest, 1, 1)
  ADD_BITFIELD_MEMBER(hasStencilTest, 2, 1)
  ADD_BITFIELD_MEMBER(hasBlendConstants, 3, 1)

  void from(RenderStateSystemBackend & rs_backend, const GraphicsPipelineVariantDescription &desc, RenderPassResource *native_rp);

END_BITFIELD_TYPE()

class GraphicsPipeline : public BasePipeline<GraphicsPipelineLayout, GraphicsProgram>
{
  int32_t refs = 1;
  bool ignore;

public:
  using CreationFeedback = CreationFeedbackBase<spirv::graphics::MAX_SETS>;

  struct CreationInfo
  {
    RenderStateSystemBackend &rsBackend;
    const GraphicsPipelineVariantDescription &varDsc;
    const GraphicsPipelineDynamicStateMask &dynStateMask;
    const GraphicsPipelineShaderSet<const ShaderModule *> &modules;
    GraphicsPipeline *parentPipeline;
    RenderPassResource *nativeRP;
    GraphicsPipelineCompileScratchData *scratch;

    CreationInfo() = delete;
  };

  GraphicsPipeline(VulkanDevice &dev, VulkanPipelineCacheHandle cache, LayoutType *l, const CreationInfo &info);
  void bind() const;

  const GraphicsPipelineDynamicStateMask &getDynamicStateMask() { return dynStateMask; }

  int32_t addRef() { return ++refs; }
  int32_t release() { return --refs; }
  bool isIgnored() { return ignore; }
  void compile();
  VulkanPipelineHandle createPipelineObject(CreationFeedback &cr_feedback);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  static const ShaderDebugInfo emptyDebugInfo;
#endif

private:
  GraphicsPipelineDynamicStateMask dynStateMask;
  GraphicsPipelineCompileScratchData *compileScratch;
};

} // namespace drv3d_vulkan