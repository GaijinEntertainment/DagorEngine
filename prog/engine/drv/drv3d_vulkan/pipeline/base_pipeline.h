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
  void init(VulkanDevice &device, const CreationInfo &headers)
  {
    VulkanDescriptorSetLayoutHandle layoutSet[ShaderConfig::count + spirv::bindless::MAX_SETS];

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
    for (size_t i = 0; i < activeRegisters; ++i)
    {
      if (headers.list[i])
      {
        registers.list[i].init(device, headers.list[i]->hash, headers.list[i]->header, ShaderConfig::stages[i]);
        shaderStages |= headers.list[i]->stage;
      }
      else
        registers.list[i].initEmpty(device);

      layoutSet[ShaderConfig::registerIndexes[i] + ShaderConfig::bindlessSetCount] = registers.list[i].getLayout();
    }

    VkPipelineLayoutCreateInfo plci;
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pNext = NULL;
    plci.flags = 0;
    plci.pushConstantRangeCount = 0;
    plci.pPushConstantRanges = NULL;
    plci.setLayoutCount = activeRegisters + ShaderConfig::bindlessSetCount;
    plci.pSetLayouts = ary(layoutSet);

    VULKAN_EXIT_ON_FAIL(device.vkCreatePipelineLayout(device.get(), &plci, NULL, ptr(handle)));

    for (size_t i = 0; i < activeRegisters; ++i)
    {
      if (headers.list[i])
        registers.list[i].initUpdateTemplate(device, ShaderConfig::bind, handle,
          ShaderConfig::registerIndexes[i] + ShaderConfig::bindlessSetCount);
    }
  }

public:
  BasePipelineLayout(VulkanDevice &device, const CreationInfo &headers) { init(device, headers); }

  void shutdown(VulkanDevice &device)
  {
    VULKAN_LOG_CALL(device.vkDestroyPipelineLayout(device.get(), handle, NULL));

    for (uint32_t i = 0; i < activeRegisters; ++i)
      registers.list[i].shutdown(device);
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

template <typename PipelineLayoutType, typename PipelineProgramType>
class BasePipeline : public CustomPipeline<PipelineLayoutType, PipelineProgramType>
{
public:
  BasePipeline(PipelineLayoutType *iLayout) :
    CustomPipeline<PipelineLayoutType, PipelineProgramType>(iLayout), handle(), compiledHandle()
  {}

  void shutdown(VulkanDevice &device)
  {
    VULKAN_LOG_CALL(device.vkDestroyPipeline(device.get(), handle, NULL));
    handle = VulkanNullHandle();
  }

  VulkanPipelineHandle getHandle() const { return handle; }

#if VK_USE_64_BIT_PTR_DEFINES
  bool checkCompiled()
  {
    if (is_null(handle))
    {
      if (interlocked_relaxed_load_ptr(compiledHandle.value) == VK_NULL_HANDLE)
        return false;
      swapHandles();
    }
    return true;
  }

  void swapHandles() { handle.value = interlocked_acquire_load_ptr(compiledHandle.value); }
  void setCompiledHandle(VulkanPipelineHandle new_handle) { interlocked_release_store_ptr(compiledHandle.value, new_handle.value); }
  void setHandle(VulkanPipelineHandle new_handle) { handle = new_handle; }
  VulkanPipelineHandle getCompiledHandle()
  {
    VulkanPipelineHandle ret;
    ret.value = interlocked_acquire_load_ptr(compiledHandle.value);
    return ret;
  }
#else
  bool checkCompiled()
  {
    if (is_null(handle))
    {
      if (interlocked_relaxed_load(compileSync) == 0)
        return false;
      swapHandles();
    }
    return true;
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
    G_UNUSED(sync);
    return compiledHandle;
  }
#endif

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
#endif
};

} // namespace drv3d_vulkan