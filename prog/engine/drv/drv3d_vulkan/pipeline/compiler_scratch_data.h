// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_carray.h>
#include <drv/3d/dag_decl.h>

#include "driver.h"

namespace drv3d_vulkan
{

class GraphicsPipeline;
class RenderPassResource;
struct ComputePipelineCompileScratchData
{
  VulkanShaderModuleHandle vkModule;
  VulkanPipelineLayoutHandle vkLayout;
  VulkanPipelineCacheHandle vkCache;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String name;
#endif
  int progIdx;
  bool allocated;
};

struct GraphicsPipelineCompileScratchData
{
  GraphicsPipeline *parentPipe;
  VulkanPipelineCacheHandle vkCache;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String shortDebugName;
  String fullDebugName;
#endif
  int progIdx;
  int varIdx;
  int varTotal;
  bool allocated;
  bool nonDrawCompile;
  bool failIfNotCached;
  RenderPassResource *nativeRP;
  VkResult compileResult;

  // for actual create pipe API
  carray<VkVertexInputBindingDescription, MAX_VERTEX_INPUT_STREAMS> inputStreams;
  carray<VkVertexInputAttributeDescription, MAX_VERTEX_ATTRIBUTES> inputAttribs;
  VkPipelineVertexInputStateCreateInfo vertexInput;
  VkPipelineTessellationStateCreateInfo tesselation;
  VkPipelineRasterizationStateCreateInfo raster;
  VkPipelineMultisampleStateCreateInfo multisample;
  VkPipelineDepthStencilStateCreateInfo depthStencil;
  carray<VkPipelineColorBlendAttachmentState, Driver3dRenderTarget::MAX_SIMRT> attachmentStates;
  VkPipelineColorBlendStateCreateInfo colorBlendState;
  VkPipelineDynamicStateCreateInfo dynamicStates;
#if VK_EXT_conservative_rasterization
  VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterStateCI;
#endif
  VkPipelineShaderStageCreateInfo stages[spirv::graphics::MAX_SETS];
  VkPipelineInputAssemblyStateCreateInfo piasci;
  VkGraphicsPipelineCreateInfo gpci;
};

} // namespace drv3d_vulkan
