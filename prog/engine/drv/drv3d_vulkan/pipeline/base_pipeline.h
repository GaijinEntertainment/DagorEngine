// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"
#include "shader.h"
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_atomic.h>
#include "globals.h"
#include "physical_device_set.h"
#include "driver_config.h"

namespace drv3d_vulkan
{

template <template <typename> class BaseShaderSet, typename ShaderConfig>
class BasePipelineLayout
{
public:
  template <typename T>
  using ShaderSet = BaseShaderSet<T>;

  typedef ShaderConfig ShaderConfiguration;
  typedef ShaderSet<const ShaderModuleHeader *> CreationInfo;

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  typedef ShaderSet<ShaderDebugInfo> DebugInfo;
#endif

  ShaderSet<DescriptorSet> registers;
  VulkanPipelineLayoutHandle handle;
  VkShaderStageFlags shaderStages;
  size_t activeRegisters;

private:
  void init(const CreationInfo &headers)
  {
    VulkanDescriptorSetLayoutHandle layoutSet[ShaderConfig::count + spirv::bindless::MAX_SETS];
    VkPushConstantRange pushConstantSet[ShaderConfig::count + spirv::bindless::MAX_SETS];
    uint32_t pushConstantsOnExtraStages = 0;

    for (int i = 0; i < ShaderConfig::bindlessSetCount; ++i)
    {
      G_ASSERT(is_null(layoutSet[i]));
      layoutSet[i] = ShaderConfig::bindlessSetLayouts[i];
    }

    activeRegisters = 0;
    for (size_t i = 0; i < ShaderConfig::count; ++i) // -V1008
      if (headers.list[i])
        activeRegisters = i + 1;

    shaderStages = 0;
    VkPipelineLayoutCreateInfo plci;
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pNext = NULL;
    plci.flags = 0;
    plci.pushConstantRangeCount = 0;
    plci.pPushConstantRanges = pushConstantSet;
    plci.setLayoutCount = activeRegisters + ShaderConfig::bindlessSetCount;
    plci.pSetLayouts = ary(layoutSet);

    for (size_t i = 0; i < activeRegisters; ++i)
    {
      if (headers.list[i])
      {
        registers.list[i].init(headers.list[i]->hash, headers.list[i]->header, ShaderConfig::stages[i]);
        shaderStages |= headers.list[i]->stage;
        if (headers.list[i]->header.pushConstantsCount)
        {
          // VS+PS can use different push constant ranges
          // yet other stages must share range from VS, and is always present in this case
          // so conunt offsets for VS+PS combo and expand VS range by stage flags for other stages
          uint32_t pushConstantsSize = (uint32_t)(4 * headers.list[i]->header.pushConstantsCount);

          if (headers.list[i]->stage & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT))
          {
            // for PS we use MAX_IMMEDIATE_CONST_WORDS elements hardcoded offset
            uint32_t offset =
              (headers.list[i]->stage & VK_SHADER_STAGE_FRAGMENT_BIT) ? MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t) : 0;
            pushConstantSet[plci.pushConstantRangeCount] = {headers.list[i]->stage, offset, pushConstantsSize};
            ++plci.pushConstantRangeCount;
          }
          else
            pushConstantsOnExtraStages = max(pushConstantsOnExtraStages, pushConstantsSize);
        }
      }
      else
        registers.list[i].initEmpty();

      layoutSet[ShaderConfig::registerIndexes[i] + ShaderConfig::bindlessSetCount] = registers.list[i].getLayout();
    }

    // shaders on extra stage should reuse VS push constants, but there is a bit of corner cases to process properly
    // 1. VS may not use them, so they will be optimized out fully and there will be no range to append stage to
    // 2. extra stages may use different amount of consts
    // 3. stage masks for push and range must be equal
    if (pushConstantsOnExtraStages)
    {
      int32_t vsPushRange = -1;
      for (int32_t i = 0; i < plci.pushConstantRangeCount; ++i)
      {
        if (pushConstantSet[i].stageFlags & VK_SHADER_STAGE_VERTEX_BIT)
          vsPushRange = i;
      }
      const VkShaderStageFlags mirroredExtraStagesMask =
        VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
      // allocate MAX_IMMEDIATE_CONST_WORDS to avoid selecing max at apply (mask select there!)
      if (!vsPushRange)
      {
        pushConstantSet[plci.pushConstantRangeCount] = {
          VK_SHADER_STAGE_VERTEX_BIT | (mirroredExtraStagesMask & shaderStages), 0, MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t)};
        ++plci.pushConstantRangeCount;
      }
      else
      {
        pushConstantSet[vsPushRange].stageFlags |= mirroredExtraStagesMask & shaderStages;
        pushConstantSet[vsPushRange].size = MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t);
      }
    }

    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreatePipelineLayout(Globals::VK::dev.get(), &plci, VKALLOC(pipeline_layout), ptr(handle)));

    for (size_t i = 0; i < activeRegisters; ++i)
    {
      if (headers.list[i])
        registers.list[i].initUpdateTemplate(ShaderConfig::bind, handle,
          ShaderConfig::registerIndexes[i] + ShaderConfig::bindlessSetCount);
    }
  }

public:
  BasePipelineLayout(const CreationInfo &headers) { init(headers); }

  void shutdown()
  {
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyPipelineLayout(Globals::VK::dev.get(), handle, VKALLOC(pipeline_layout)));

    for (uint32_t i = 0; i < activeRegisters; ++i)
      registers.list[i].shutdown();
  }

  bool matches(const CreationInfo &headers) const
  {
    for (uint32_t i = 0; i < ShaderConfig::count; ++i) // -V1008
      if (headers.list[i] && i >= activeRegisters)
        return false;

    for (uint32_t i = 0; i < activeRegisters; ++i)
    {
      if (!registers.list[i].matches(headers.list[i]))
        return false;
    }

    return true;
  }
};

template <typename PipelineLayoutType, typename PipelineProgramType>
class CustomPipeline
{
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  bool allowUsage = true;
#endif
public:
  typedef PipelineLayoutType LayoutType;
  typedef PipelineProgramType ProgramType;

  CustomPipeline(LayoutType *iLayout) : layout(iLayout) {}

  inline LayoutType *getLayout() const { return layout; }

#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  bool isUsageAllowed() { return allowUsage; }

  // block usage should only work on graphics programs. Would've been a lot easier with if constexpr...
  template <typename Dummy = void>
  std::enable_if_t<std::is_same<PipelineProgramType, GraphicsProgram>::value, Dummy> blockUsage(bool doBlock = true)
  {
    allowUsage = !doBlock;
  }

  template <typename Dummy = void>
  std::enable_if_t<!std::is_same<PipelineProgramType, GraphicsProgram>::value, Dummy> blockUsage(bool = true)
  {
    G_ASSERTF(false, "vulkan: Attempted to block usage of a non-graphics pipeline!");
  }

  bool isShaderBlocked(const char *shader_name)
  {
    bool ret = ::dgs_get_settings()
                 ->getBlockByNameEx("vulkan")
                 ->getBlockByNameEx("debug")
                 ->getBlockByNameEx("shader_block_list")
                 ->getBool(shader_name, false);
    if (ret)
      debug("vulkan: blocked shader %s", shader_name);
    return ret;
  }
#endif

protected:
  LayoutType *layout;
};

enum class PipelineCompileStatus
{
  OK,
  PENDING,
  FAIL
};

template <typename PipelineLayoutType, typename PipelineProgramType>
class BasePipeline : public CustomPipeline<PipelineLayoutType, PipelineProgramType>
{
public:
  BasePipeline(PipelineLayoutType *iLayout) :
    CustomPipeline<PipelineLayoutType, PipelineProgramType>(iLayout), handle(), compiledHandle()
  {}

  void shutdown()
  {
    if (is_null(handle))
      return;

    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyPipeline(Globals::VK::dev.get(), handle, VKALLOC(pipeline)));
    handle = VulkanNullHandle();
  }

  VulkanPipelineHandle getHandle() const { return handle; }

#if VK_USE_64_BIT_PTR_DEFINES
  PipelineCompileStatus checkCompiled()
  {
    if (is_null(handle))
    {
      if (interlocked_relaxed_load_ptr(compiledHandle.value) == VK_NULL_HANDLE)
        return PipelineCompileStatus::PENDING;
      swapHandles();
      if (is_null(handle))
        return PipelineCompileStatus::FAIL;
    }
    return PipelineCompileStatus::OK;
  }

  // use offset to encode fact of failed compilation (i.e. handle is null but compile was completed)
  static const VulkanHandle compiled_handle_value_offset = 1;
  VkPipeline decodeCompiledHandle(VkPipeline v) { return (VkPipeline)((VulkanHandle)v - compiled_handle_value_offset); }
  VkPipeline encodeCompiledHandle(VkPipeline v) { return (VkPipeline)((VulkanHandle)v + compiled_handle_value_offset); }

  void swapHandles() { handle.value = decodeCompiledHandle(interlocked_acquire_load_ptr(compiledHandle.value)); }
  void setCompiledHandle(VulkanPipelineHandle new_handle)
  {
    interlocked_release_store_ptr(compiledHandle.value, encodeCompiledHandle(new_handle.value));
  }
  void setHandle(VulkanPipelineHandle new_handle) { handle = new_handle; }
  VulkanPipelineHandle getCompiledHandle()
  {
    VulkanPipelineHandle ret{};
    VkPipeline localCopy = interlocked_acquire_load_ptr(compiledHandle.value);
    if (localCopy)
      ret.value = decodeCompiledHandle(localCopy);
    return ret;
  }
#else
  PipelineCompileStatus checkCompiled()
  {
    if (is_null(handle))
    {
      if (interlocked_relaxed_load(compileSync) == 0)
        return PipelineCompileStatus::PENDING;
      swapHandles();
      if (is_null(handle))
        return PipelineCompileStatus::FAIL;
    }
    return PipelineCompileStatus::OK;
  }

  void swapHandles()
  {
    volatile int sync = interlocked_acquire_load(compileSync);
    G_UNUSED(sync);
    handle = compiledHandle;
  }
  void setCompiledHandle(VulkanPipelineHandle new_handle)
  {
    compiledHandle = new_handle;
    interlocked_release_store(compileSync, 1);
  }
  void setHandle(VulkanPipelineHandle new_handle) { handle = new_handle; }
  VulkanPipelineHandle getCompiledHandle()
  {
    volatile int sync = interlocked_acquire_load(compileSync);
    return sync ? compiledHandle : VulkanPipelineHandle{};
  }
#endif

  // external provided handle to properly process async compilations and other related stuff
  void dumpExecutablesInfo(VulkanPipelineHandle pipe_handle)
  {
    if (!Globals::VK::phy.hasPipelineExecutableInfo || !Globals::cfg.bits.dumpPipelineExecutableStatistics || is_null(pipe_handle))
      return;

    debug("====%016llX====", generalize(pipe_handle));

    VkPipelineInfoKHR pipeInfo = {VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR, nullptr, pipe_handle};
    dag::RelocatableFixedVector<VkPipelineExecutablePropertiesKHR, (int)ExtendedShaderStage::MAX> executablesProps;
    dag::RelocatableFixedVector<VkPipelineExecutableStatisticKHR, 10> executableStats;
    uint32_t executablesCount = 0;

    if (VULKAN_CHECK_FAIL(
          Globals::VK::dev.vkGetPipelineExecutablePropertiesKHR(Globals::VK::dev.get(), &pipeInfo, &executablesCount, nullptr)))
    {
      debug("executable count query failed");
      return;
    }

    executablesProps.resize(executablesCount);

    if (VULKAN_CHECK_FAIL(Globals::VK::dev.vkGetPipelineExecutablePropertiesKHR(Globals::VK::dev.get(), &pipeInfo, &executablesCount,
          executablesProps.data())))
    {
      debug("executables prop query failed");
      return;
    }

    for (VkPipelineExecutablePropertiesKHR &i : executablesProps)
    {
      uint32_t executableIndex = &i - executablesProps.begin();
      debug("executable: %u stage %s %s-%s subgroup %u", executableIndex, formatShaderStageFlags(i.stages), i.name, i.description,
        i.subgroupSize);
      uint32_t statisticsCount = 0;

      VkPipelineExecutableInfoKHR executableInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR, nullptr, pipe_handle, executableIndex};

      if (VULKAN_CHECK_FAIL(
            Globals::VK::dev.vkGetPipelineExecutableStatisticsKHR(Globals::VK::dev.get(), &executableInfo, &statisticsCount, nullptr)))
      {
        debug("executable statistics count query failed");
        continue;
      }

      executableStats.resize(statisticsCount);

      if (VULKAN_CHECK_FAIL(Globals::VK::dev.vkGetPipelineExecutableStatisticsKHR(Globals::VK::dev.get(), &executableInfo,
            &statisticsCount, executableStats.data())))
      {
        debug("executable statistics query failed");
        continue;
      }

      for (VkPipelineExecutableStatisticKHR &j : executableStats)
      {
        String statLine(128, "stat %u: %s-%s ", &j - executableStats.begin(), j.name, j.description);
        switch (j.format)
        {
          case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
            statLine += String(32, "b %s", j.value.b32 ? "true" : "false");
            break;
          case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR: statLine += String(32, "i %lli", j.value.i64); break;
          case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR: statLine += String(32, "u %llu", j.value.u64); break;
          case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR: statLine += String(32, "f %lf", j.value.f64); break;
          default: statLine += String(32, "unknown fmt %u", (uint32_t)j.format); break;
        }
        debug(statLine);
      }
    }

    debug("====%016llX====", generalize(pipe_handle));
  }

private:
  VulkanPipelineHandle handle;
  VulkanPipelineHandle compiledHandle;
#if !VK_USE_64_BIT_PTR_DEFINES
  volatile uint32_t compileSync = 0;
#endif
};

// use module impl method to avoid including too much in header
void setDebugNameForDescriptorSet(VulkanDescriptorSetHandle ds_handle, const char *name);

template <typename PipelineLayoutType, typename PipelineProgramType, template <typename, typename> class TargetPipelineType>
class DebugAttachedPipeline : public TargetPipelineType<PipelineLayoutType, PipelineProgramType>
{
  typedef TargetPipelineType<PipelineLayoutType, PipelineProgramType> BaseType;

public:
  DebugAttachedPipeline(PipelineLayoutType *iLayout) :
    TargetPipelineType<PipelineLayoutType, PipelineProgramType>(iLayout)
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
    ,
    totalCompilationTime(0),
    variantCount(0)
#endif
  {}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  void setDebugInfo(typename PipelineLayoutType::DebugInfo iDebugInfo)
  {
    debugInfo = iDebugInfo;
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
    for (int i = 0; i < PipelineLayoutType::ShaderConfiguration::count; ++i)
    {
      if (BaseType::isShaderBlocked(debugInfo.list[i].name))
      {
        BaseType::blockUsage();
        break;
      }
    }
#endif
  }

  void printDebugInfo()
  {
    for (int i = 0; i < PipelineLayoutType::ShaderConfiguration::count; ++i)
      debug(">> shader %u = %s", i, debugInfo.list[i].debugName);
  }

  String printDebugInfoBuffered() const
  {
    String ret(0, "pipe %p \n", this);
    for (int i = 0; i < PipelineLayoutType::ShaderConfiguration::count; ++i)
      ret += String(0, "  shader %u = %s \n", i, debugInfo.list[i].name);
    return ret;
  }

  String printDebugInfoBuffered(int shader_idx) const
  {
    return String(0, "pipe %p, shader %u = %s", this, shader_idx, debugInfo.list[shader_idx].name);
  }

  void setNameForDescriptorSet(VulkanDescriptorSetHandle ds_handle, int shader_idx)
  {
    if (is_null(ds_handle))
      return;

    setDebugNameForDescriptorSet(ds_handle, printDebugInfoBuffered(shader_idx));
  }

  dag::Vector<ShaderDebugInfo> dumpShaderInfos() const { return {eastl::begin(debugInfo.list), eastl::end(debugInfo.list)}; }

  int64_t dumpCompilationTime() const { return totalCompilationTime; }
  size_t dumpVariantCount() const { return variantCount; }

protected:
  typename PipelineLayoutType::DebugInfo debugInfo;
  // Total time for all variants (us)
  int64_t totalCompilationTime;
  // Variant count for variated graphics pipelines (1 for compute)
  size_t variantCount;
#else
  void setNameForDescriptorSet(VulkanDescriptorSetHandle, int) {}
  String printDebugInfoBuffered() const
  {
    String ret(0, "pipe %p ", this);
    return ret;
  }
#endif
};

} // namespace drv3d_vulkan