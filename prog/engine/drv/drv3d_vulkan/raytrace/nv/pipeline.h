// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"

namespace drv3d_vulkan
{

class ProgramDatabase;
class RaytraceAccelerationStructure;

template <typename fieldType>
struct RaytracePipelineShaderSet
{
  fieldType &rs() { return list[spirv::raytrace::REGISTERS_SET_INDEX]; }

  fieldType list[spirv::raytrace::MAX_SETS];
};

struct RaytracePipelineShaderConfig
{
  static const VkPipelineBindPoint bind = VK_PIPELINE_BIND_POINT_RAY_TRACING_NV;
  static const uint32_t count = spirv::raytrace::MAX_SETS;
  static const uint32_t stages[count];
  static const uint32_t registerIndexes[count];
};

typedef BasePipelineLayout<RaytracePipelineShaderSet, RaytracePipelineShaderConfig> RaytracePipelineLayout;

class RaytracePipeline : public BasePipeline<RaytracePipelineLayout, RaytraceProgram>
{
  ProgramID program;

public:
  struct CreationInfo
  {
    uint32_t max_recursion;
    uint32_t shader_count;
    const ShaderModuleUse *uses;
    uint32_t group_count;
    const RaytraceShaderGroup *groups;
    const eastl::unique_ptr<ShaderModule> *module_set;
    const LayoutType::CreationInfo &layout;

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

  const char *resTypeString() { return "RaytracePipeline"; }

  RaytracePipeline(VulkanDevice &dev, ProgramID prog, VulkanPipelineCacheHandle cache, LayoutType *l, const CreationInfo &info);

  VulkanPipelineHandle getHandleForUse();
  void copyRaytraceShaderGroupHandlesToMemory(VulkanDevice &dev, uint32_t first_group, uint32_t group_count, uint32_t size, void *ptr);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  void printDebugInfo()
  {
    for (int i = 0; i < debugInfo.size(); ++i)
      debug(">> shader %u = %s", i, debugInfo[i].name);
  }

  String printDebugInfoBuffered()
  {
    String ret(0, "pipe %p \n", this);
    for (int i = 0; i < debugInfo.size(); ++i)
      ret += String(0, "  shader %u = %s \n", i, debugInfo[i].name);
    return ret;
  }

  dag::Vector<ShaderDebugInfo> dumpShaderInfos() const { return {eastl::begin(debugInfo), eastl::end(debugInfo)}; }

  int64_t dumpCompilationTime() const { return compilationTime; }
  size_t dumpVariantCount() const { return 1; }

private:
  eastl::vector<ShaderDebugInfo> debugInfo;
  int64_t compilationTime;
#endif
};

} // namespace drv3d_vulkan