#include "device.h"
#include "perfMon/dag_statDrv.h"
#include "render_pass_resource.h"

using namespace drv3d_vulkan;

uint32_t ComputePipeline::spirvWorkGroupSizeDimConstantIds[ComputePipeline::workGroupDims] = {
  spirv::WORK_GROUP_SIZE_X_CONSTANT_ID,
  spirv::WORK_GROUP_SIZE_Y_CONSTANT_ID,
  spirv::WORK_GROUP_SIZE_Z_CONSTANT_ID,
};

uint32_t PipelineBindlessConfig::bindlessSetCount = 0;
VulkanDescriptorSetLayoutHandle PipelineBindlessConfig::bindlessTextureSetLayout;
VulkanDescriptorSetLayoutHandle PipelineBindlessConfig::bindlessSamplerSetLayout;

const VkShaderStageFlagBits ComputePipelineShaderConfig::stages[ComputePipelineShaderConfig::count] = {VK_SHADER_STAGE_COMPUTE_BIT};

const VkShaderStageFlagBits GraphicsPipelineShaderConfig::stages[GraphicsPipelineShaderConfig::count] = {VK_SHADER_STAGE_VERTEX_BIT,
  VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_GEOMETRY_BIT, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT};

const uint32_t ComputePipelineShaderConfig::registerIndexes[ComputePipelineShaderConfig::count] = {
  spirv::compute::REGISTERS_SET_INDEX};

const uint32_t GraphicsPipelineShaderConfig::registerIndexes[GraphicsPipelineShaderConfig::count] = {
  spirv::graphics::vertex::REGISTERS_SET_INDEX, spirv::graphics::fragment::REGISTERS_SET_INDEX,
  spirv::graphics::geometry::REGISTERS_SET_INDEX, spirv::graphics::control::REGISTERS_SET_INDEX,
  spirv::graphics::evaluation::REGISTERS_SET_INDEX};

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
const ShaderDebugInfo GraphicsPipeline::emptyDebugInfo = {};
#endif

namespace drv3d_vulkan
{

template <>
void ComputePipeline::onDelayedCleanupFinish<ComputePipeline::CLEANUP_DESTROY>()
{
  shutdown(get_device().getVkDevice());
  delete this;
}

} // namespace drv3d_vulkan

ComputePipeline::ComputePipeline(VulkanDevice &device, ProgramID prog, VulkanPipelineCacheHandle cache, LayoutType *l,
  const CreationInfo &info) :
  DebugAttachedPipeline(l)
{
  VulkanShaderModuleHandle shader = get_device().makeVkModule(info.sci);

  VkComputePipelineCreateInfo cpci = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, NULL};
  cpci.flags = 0;
  cpci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  cpci.stage.pNext = NULL;
  cpci.stage.flags = 0;
  cpci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  cpci.stage.module = shader;
  cpci.stage.pName = "main";
  cpci.stage.pSpecializationInfo = NULL;
  cpci.layout = layout->handle;
  cpci.basePipelineHandle = VK_NULL_HANDLE;
  cpci.basePipelineIndex = -1;

  CreationFeedback crFeedback;
  crFeedback.chainWith(cpci, device);

  int64_t compilationTime = 0;
  VkResult compileResult = VK_ERROR_UNKNOWN;
  {
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    TIME_PROFILE_NAME(vulkan_cs_pipeline_compile, info.sci->name)
#else
    TIME_PROFILE(vulkan_cs_pipeline_compile)
#endif
    ScopedTimer compilationTimer(compilationTime);
    compileResult = device.vkCreateComputePipelines(device.get(), cache, 1, &cpci, NULL, ptr(handle));
  }

  if (is_null(handle) && VULKAN_OK(compileResult))
  {
    debug("vulkan: pipeline [compute:%u] not compiled but result was ok (%u)", prog.get(), compileResult);
    compileResult = VK_ERROR_UNKNOWN;
  }

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  if (VULKAN_FAIL(compileResult))
    debug("vulkan: pipeline [compute:%u] cs: %s failed to compile", prog.get(), info.sci->name);
  get_device().setPipelineName(handle, info.sci->name);
  get_device().setPipelineLayoutName(getLayout()->handle, info.sci->name);
  totalCompilationTime = compilationTime;
  variantCount = 1;
#endif
  VULKAN_EXIT_ON_FAIL(compileResult);

#if VULKAN_LOG_PIPELINE_ACTIVITY < 1
  if (compilationTime > PIPELINE_COMPILATION_LONG_THRESHOLD)
#endif
  {
    debug("vulkan: pipeline [compute:%u] compiled in %u us", prog.get(), compilationTime);
    crFeedback.logFeedback();
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    debug("vulkan: with cs %s , handle %p", info.sci->name, generalize(handle));
#endif
  }

  // no need to keep the shader module, delete it to save memory
  VULKAN_LOG_CALL(device.vkDestroyShaderModule(device.get(), shader, NULL));
}

void ComputePipeline::bind(VulkanDevice &vk_dev, VulkanCommandBufferHandle cmd_buffer)
{
#if VULKAN_LOG_PIPELINE_ACTIVITY > 1
  debug("vulkan: bind compute cs %s", debugInfo.cs().name);
#endif
  VULKAN_LOG_CALL(vk_dev.vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, handle));
}

static VkSampleCountFlagBits checkSampleCount(unsigned int count, uint8_t colorMask, uint8_t hasDepth)
{
  G_UNUSED(colorMask);
  G_UNUSED(hasDepth);
  if (count <= 1)
    return VK_SAMPLE_COUNT_1_BIT;
  VkSampleCountFlagBits ret = VK_SAMPLE_COUNT_1_BIT;
  switch (count)
  {
    case 2: ret = VK_SAMPLE_COUNT_2_BIT; break;
    case 4: ret = VK_SAMPLE_COUNT_4_BIT; break;
    case 8: ret = VK_SAMPLE_COUNT_8_BIT; break;
    case 16: ret = VK_SAMPLE_COUNT_16_BIT; break;
    case 32: ret = VK_SAMPLE_COUNT_32_BIT; break;
    case 64:
      ret = VK_SAMPLE_COUNT_64_BIT;
      break;
      // this allows the compiler to optimize out this switch, while maintaining correctness
    default: ret = static_cast<VkSampleCountFlagBits>(count); break;
  }
  const VkPhysicalDeviceProperties &properties = get_device().getDeviceProperties().properties;
  G_UNUSED(properties);
  G_ASSERTF(properties.limits.framebufferNoAttachmentsSampleCounts & ret,
    "Selected sample count is not supported on the current platform");
  G_ASSERTF(!hasDepth && !colorMask, "Forced multisampling is only supported when there is no color and depth attachment");
  return ret;
}

GraphicsPipeline::GraphicsPipeline(VulkanDevice &device, VulkanPipelineCacheHandle cache, LayoutType *l, const CreationInfo &info) :
  BasePipeline(l), dynStateMask(info.dynStateMask)
{
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
  VkGraphicsPipelineCreateInfo gpci;

  // deal with render pass dependencies
  VulkanRenderPassHandle renderPassHandle;
  bool hasDepth = false;
  bool forceNoZWrite = false;
  uint32_t rpColorTargetMask = 0;
  VkSampleCountFlagBits sampleCount;
  if (info.nativeRP)
  {
    sampleCount = info.nativeRP->getMSAASamples(info.varDsc.subpass); // TODO: add MSAA test and make it work
    renderPassHandle = info.nativeRP->getHandle();
    hasDepth = info.nativeRP->hasDepthAtSubpass(info.varDsc.subpass);
    rpColorTargetMask = info.nativeRP->getColorWriteMaskAtSubpass(info.varDsc.subpass);
  }
  else
  {
    sampleCount = VkSampleCountFlagBits(eastl::max(info.varDsc.rpClass.colorSamples[0], uint8_t(1)));
    RenderPassClass *passClassRef = info.pass_man.getPassClass(info.varDsc.rpClass);
    renderPassHandle = passClassRef->getPass(device, 0);
    hasDepth = info.varDsc.rpClass.depthState != RenderPassClass::Identifier::NO_DEPTH;
    forceNoZWrite = info.varDsc.rpClass.depthState == RenderPassClass::Identifier::RO_DEPTH;
    rpColorTargetMask = info.varDsc.rpClass.colorTargetMask;
  }
  ///

  uint32_t attribs = 0;
  uint32_t lss = 0;

  auto &staticState = info.rsBackend.getStatic(info.varDsc.state.renderState.staticIdx);

  InputLayout inputLayout = get_shader_program_database().getInputLayoutFromId(info.varDsc.state.inputLayout);

  for (int32_t i = 0; i < inputLayout.attribs.size(); ++i)
    if (inputLayout.attribs[i].used)
      inputAttribs[attribs++] = inputLayout.attribs[i].toVulkan();

  for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    if (inputLayout.streams.used[i])
      inputStreams[lss++] = inputLayout.streams.toVulkan(i, info.varDsc.state.strides[i]);

  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.pNext = NULL;
  vertexInput.flags = 0;
  vertexInput.vertexBindingDescriptionCount = lss;
  vertexInput.pVertexBindingDescriptions = inputStreams.data();
  vertexInput.vertexAttributeDescriptionCount = attribs;
  vertexInput.pVertexAttributeDescriptions = inputAttribs.data();

  tesselation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  tesselation.pNext = NULL;
  tesselation.flags = 0;
  tesselation.patchControlPoints = 4;

  raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster.pNext = NULL;
  raster.flags = 0;
#if !_TARGET_ANDROID
  raster.depthClampEnable = staticState.depthClipEnable ? VK_FALSE : VK_TRUE;
#else
  raster.depthClampEnable = VK_FALSE;
#endif
  raster.rasterizerDiscardEnable = VK_FALSE;
  raster.polygonMode = static_cast<VkPolygonMode>((uint32_t)info.varDsc.state.polygonLine);
  uint32_t cull_mode = staticState.cullMode;

  if (!cull_mode)
  {
    raster.cullMode = 0;
  }
  else if (cull_mode == (CULL_CW - CULL_NONE))
  {
    raster.cullMode = VK_CULL_MODE_FRONT_BIT;
  }
  else if (cull_mode == (CULL_CCW - CULL_NONE))
  {
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
  }
  raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
  raster.depthBiasEnable = VK_TRUE;
  raster.depthBiasConstantFactor = 0.f;
  raster.depthBiasClamp = 0.f;
  raster.depthBiasSlopeFactor = 0.f;
  raster.lineWidth = 1.f;
#if VK_EXT_conservative_rasterization
  VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterStateCI{};
  if (staticState.conservativeRasterEnable && device.hasExtension<ConservativeRasterizationEXT>())
  {
    conservativeRasterStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
    conservativeRasterStateCI.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
    conservativeRasterStateCI.extraPrimitiveOverestimationSize = 0;
    chain_structs(raster, conservativeRasterStateCI);
  }
#endif

  auto forcedSamplerCount = staticState.getForcedSamplerCount();
  multisample.rasterizationSamples =
    forcedSamplerCount == 0 ? sampleCount : checkSampleCount(staticState.getForcedSamplerCount(), rpColorTargetMask, hasDepth);
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.pNext = NULL;
  multisample.flags = 0;
  multisample.sampleShadingEnable = VK_FALSE;
  multisample.minSampleShading = 1.f;
  multisample.pSampleMask = NULL;
  multisample.alphaToCoverageEnable = staticState.alphaToCoverage ? VK_TRUE : VK_FALSE;
  multisample.alphaToOneEnable = VK_FALSE;

  G_ASSERTF((multisample.alphaToCoverageEnable == VK_FALSE) ||
              ((multisample.alphaToCoverageEnable == VK_TRUE) && !(multisample.rasterizationSamples & VK_SAMPLE_COUNT_1_BIT)),
    "vulkan: alpha to coverage must be used with MSAA");

  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.pNext = NULL;
  depthStencil.flags = 0;
  depthStencil.depthTestEnable = staticState.depthTestEnable;
  depthStencil.depthWriteEnable = staticState.depthWriteEnable && !forceNoZWrite;
  depthStencil.depthCompareOp = (VkCompareOp)(uint32_t)staticState.depthTestFunc;
  depthStencil.depthBoundsTestEnable = staticState.depthBoundsEnable;
  depthStencil.stencilTestEnable = staticState.stencilTestEnable;
  depthStencil.front.failOp = (VkStencilOp)(uint32_t)staticState.stencilTestOpStencilFail;
  depthStencil.front.passOp = (VkStencilOp)(uint32_t)staticState.stencilTestOpPass;
  depthStencil.front.depthFailOp = (VkStencilOp)(uint32_t)staticState.stencilTestOpDepthFail;
  depthStencil.front.compareOp = (VkCompareOp)(uint32_t)staticState.stencilTestFunc;
  depthStencil.front.compareMask = 0xFF;
  depthStencil.front.writeMask = 0xFF;
  depthStencil.front.reference = 0xFF;
  depthStencil.back = depthStencil.front;
  depthStencil.minDepthBounds = 0.f;
  depthStencil.maxDepthBounds = 1.f;

  uint32_t attachmentCount = 0;

  // if color targets are needed, fill attachments to max slot used by colorTargetMask
  // to be both compatible with render pass and conserve resources
  //
  // compatibility with fragment shader on output attachments amount is not needed as
  // when fragment writes to non defined attachment, write is simply ignored
  //
  // The driver then has to sort out stuff by removing not written or not
  // used outputs (which it has to do anyways to optimize shaders).
  if (rpColorTargetMask)
  {
    uint8_t swMask = layout->registers.fs().header.outputMask;
    uint8_t rpMask = rpColorTargetMask;
    for (uint32_t i = 0; (i < Driver3dRenderTarget::MAX_SIMRT) && rpMask; ++i, rpMask >>= 1)
    {
      bool isResolveAttachment = (i > 0) && info.varDsc.rpClass.colorSamples[i - 1] > 1 && !info.nativeRP;
      if (isResolveAttachment)
      {
        continue;
      }

      auto &state = attachmentStates[attachmentCount];
      state.blendEnable = staticState.blendEnable;
      state.srcColorBlendFactor = (VkBlendFactor)(uint32_t)staticState.blendSrcFactor;
      state.dstColorBlendFactor = (VkBlendFactor)(uint32_t)staticState.blendDstFactor;
      state.colorBlendOp = (VkBlendOp)(uint32_t)staticState.blendOp;

      if (staticState.blendSeparateAlphaEnable)
      {
        state.srcAlphaBlendFactor = (VkBlendFactor)(uint32_t)staticState.blendSrcAlphaFactor;
        state.dstAlphaBlendFactor = (VkBlendFactor)(uint32_t)staticState.blendDstAlphaFactor;
        state.alphaBlendOp = (VkBlendOp)(uint32_t)staticState.blendOpAlpha;
      }
      else
      {
        state.srcAlphaBlendFactor = (VkBlendFactor)(uint32_t)staticState.blendSrcFactor;
        state.dstAlphaBlendFactor = (VkBlendFactor)(uint32_t)staticState.blendDstFactor;
        state.alphaBlendOp = (VkBlendOp)(uint32_t)staticState.blendOp;
      }

      if (0 != (swMask & rpMask & 1))
      {
        state.colorWriteMask = staticState.colorMask >> (attachmentCount * 4) & VK_COLOR_COMPONENT_RGBA_BIT;
      }
      else
      {
        // if the shader does not provide any value,
        // or if the shader does provide value but RP do not have target
        // we need to set this to 0
        // to avoid writing random values to that slot in the framebuffer.
        state.colorWriteMask = 0;
      }

      ++attachmentCount;
      swMask >>= 1;
    }
  }

  colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendState.pNext = NULL;
  colorBlendState.flags = 0;
  colorBlendState.logicOpEnable = VK_FALSE;
  colorBlendState.logicOp = VK_LOGIC_OP_COPY;
  colorBlendState.attachmentCount = attachmentCount;
  colorBlendState.pAttachments = attachmentStates.data();
  memset(colorBlendState.blendConstants, 0, sizeof(colorBlendState.blendConstants));

  if (!hasDepth)
  {
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    raster.depthBiasEnable = VK_FALSE;
    raster.depthClampEnable = VK_FALSE;
  }

  VkDynamicState dynamicStateList[] = //
    {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, VK_DYNAMIC_STATE_DEPTH_BOUNDS,
      VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK, VK_DYNAMIC_STATE_STENCIL_REFERENCE,
      VK_DYNAMIC_STATE_BLEND_CONSTANTS};

  dynamicStates.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStates.pNext = NULL;
  dynamicStates.flags = 0;
  dynamicStates.dynamicStateCount = array_size(dynamicStateList);
  dynamicStates.pDynamicStates = dynamicStateList;

  VkPipelineInputAssemblyStateCreateInfo piasci = //
    {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0, info.varDsc.topology, VK_FALSE};

  static const VkRect2D rect = {{0, 0}, {1, 1}};
  static const VkViewport viewport = {0.f, 0.f, 1.f, 1.f, 0.f, 1.f};
  // no need for unique states per variant, they are all the same
  static const VkPipelineViewportStateCreateInfo viewportStates = //
    {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0, 1, &viewport, 1, &rect};

  VkPipelineShaderStageCreateInfo stages[spirv::graphics::MAX_SETS] = {};
  unsigned stagesCount = 0;
  for (size_t i = 0; i < spirv::graphics::MAX_SETS; ++i)
  {
    const ShaderModule *shModule = info.modules.list[i];
    if (shModule)
    {
      stages[stagesCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      stages[stagesCount].stage = LayoutType::ShaderConfiguration::stages[i];
      stages[stagesCount].module = shModule->module;
      stages[stagesCount].pName = "main";
      stagesCount++;
    }
  }
  gpci.stageCount = stagesCount;

  gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gpci.pNext = NULL;
  gpci.pVertexInputState = &vertexInput;
  gpci.pTessellationState = layout->hasTC() ? &tesselation : NULL;
  gpci.pRasterizationState = &raster;
  gpci.pMultisampleState = &multisample;
  gpci.pDepthStencilState = &depthStencil;
  gpci.pColorBlendState = &colorBlendState;
  gpci.pDynamicState = &dynamicStates;
  gpci.pInputAssemblyState = &piasci;
  gpci.pViewportState = &viewportStates;
  gpci.pStages = stages;
  gpci.layout = layout->handle;
  gpci.renderPass = renderPassHandle;
  gpci.subpass = info.varDsc.subpass;
  gpci.basePipelineIndex = 0;
  gpci.basePipelineHandle = VK_NULL_HANDLE;
  gpci.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

  if (!is_null(info.parentPipeline))
  {
    gpci.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    gpci.basePipelineIndex = -1;
    gpci.basePipelineHandle = info.parentPipeline;
  }

  info.crFeedback.chainWith(gpci, device);

  VULKAN_EXIT_ON_FAIL(device.vkCreateGraphicsPipelines(device.get(), cache, 1, &gpci, NULL, ptr(handle)));
}

void GraphicsPipeline::bind(VulkanDevice &vk_dev, VulkanCommandBufferHandle cmd_buffer) const
{
  VULKAN_LOG_CALL(vk_dev.vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, handle));
}

void GraphicsPipelineDynamicStateMask::from(RenderStateSystem::Backend &rs_backend, const GraphicsPipelineVariantDescription &desc,
  RenderPassResource *native_rp)
{
  auto rsSt = rs_backend.getStatic(desc.state.renderState.staticIdx);

  if ((desc.rpClass.depthState == RenderPassClass::Identifier::NO_DEPTH) ||
      (native_rp ? !native_rp->hasDepthAtSubpass(desc.subpass) : false))
  {
    hasDepthBias = 0;
    hasDepthBoundsTest = 0;
    hasStencilTest = 0;
  }
  else
  {
    // TODO: check this again.
    // if (rsSt.depthBiasEnable)
    hasDepthBias = 1;
    if (rsSt.depthBoundsEnable)
      hasDepthBoundsTest = 1;
    if (rsSt.stencilTestEnable)
      hasStencilTest = 1;
  }

  hasBlendConstants = 0;

  if (rsSt.blendEnable)
  {
    VkBlendFactor srcBF = (VkBlendFactor)(uint32_t)rsSt.blendSrcFactor;
    VkBlendFactor dstBF = (VkBlendFactor)(uint32_t)rsSt.blendDstFactor;

    if (((srcBF >= VK_BLEND_FACTOR_CONSTANT_COLOR) && (srcBF <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
        ((dstBF >= VK_BLEND_FACTOR_CONSTANT_COLOR) && (dstBF <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)))
    {
      hasBlendConstants = 1;
    }
  }
}

void innerSetDebugName(VulkanPipelineHandle handle, const char *name);