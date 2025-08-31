// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_state_system.h"
#include "globals.h"
#include "device_context.h"
#include "physical_device_set.h"
#include "translate_d3d_to_vk.h"
#include <drv_log_defs.h>

using namespace drv3d_vulkan;

GraphicsPipelineStaticState RenderStateSystemBackend::extractStaticState(const shaders::RenderState &state)
{
  GraphicsPipelineStaticState ret;
  ret.reset();

  // 1 - none - cull_enable = false, cull_ccw = false
  // 2 - cw - cull_enable = true, cull_ccw = false
  // 3 - ccw - cull_enable = true, cull_ccw = true
  // reconstructs 0 - none, 1 - cw, 2 - ccw (cull_ccw is false if cull_enable is false)
  ret.cullMode = state.cull - 1;
  ret.depthWriteEnable = state.zwrite;
  ret.depthTestEnable = state.ztest;
  ret.setForcedSamplerCount(state.forcedSampleCount);
  ret.depthTestFunc = translate_compare_func_to_vulkan(state.zFunc);
  ret.conservativeRasterEnable = state.conservativeRaster;
  ret.depthBoundsEnable = state.depthBoundsEnable;
  ret.colorMask = state.colorWr;
  ret.alphaToCoverage = state.alphaToCoverage;

  if (Globals::VK::phy.features.depthClamp != VK_FALSE)
  {
    ret.depthClipEnable = state.zClip;
  }

  if (state.stencil.func > 0)
  {
    ret.stencilTestEnable = 1;
    ret.stencilTestFunc = translate_compare_func_to_vulkan(state.stencil.func);
    ret.stencilTestOpPass = translate_stencil_op_to_vulkan(state.stencil.pass);
    ret.stencilTestOpDepthFail = translate_stencil_op_to_vulkan(state.stencil.zFail);
    ret.stencilTestOpStencilFail = translate_stencil_op_to_vulkan(state.stencil.fail);
  }
  else
  {
    ret.stencilTestEnable = 0;
    ret.stencilTestFunc = VK_COMPARE_OP_ALWAYS;
    ret.stencilTestOpPass = VK_STENCIL_OP_KEEP;
    ret.stencilTestOpDepthFail = VK_STENCIL_OP_KEEP;
    ret.stencilTestOpStencilFail = VK_STENCIL_OP_KEEP;
  }

  ret.indenpendentBlendEnabled = state.independentBlendEnabled;

  auto translateBlendParams = [](GraphicsPipelineStaticState::BlendState &dst, const auto &src) {
    dst.blendOp = translate_blend_op_to_vulkan(src.blendOp);
    dst.blendOpAlpha = translate_blend_op_to_vulkan(src.sepablendOp);
    dst.blendDstAlphaFactor = VK_BLEND_FACTOR_ZERO;
    dst.blendSrcAlphaFactor = VK_BLEND_FACTOR_ONE;

    if (src.ablend)
    {
      dst.blendEnable = 1;
      dst.blendDstFactor = translate_rgb_blend_mode_to_vulkan(src.ablendFactors.dst);
      dst.blendSrcFactor = translate_rgb_blend_mode_to_vulkan(src.ablendFactors.src);
    }
    else
    {
      dst.blendEnable = 0;
      dst.blendSeparateAlphaEnable = 0;
      dst.blendDstFactor = VK_BLEND_FACTOR_ZERO;
      dst.blendSrcFactor = VK_BLEND_FACTOR_ONE;
      dst.blendOp = VK_BLEND_OP_ADD;
      dst.blendOpAlpha = VK_BLEND_OP_ADD;
    }

    if (src.sepablend)
    {
      dst.blendSeparateAlphaEnable = 1;
      dst.blendDstAlphaFactor = translate_alpha_blend_mode_to_vulkan(src.sepablendFactors.dst);
      dst.blendSrcAlphaFactor = translate_alpha_blend_mode_to_vulkan(src.sepablendFactors.src);
    }
    else
      dst.blendSeparateAlphaEnable = 0;
  };

  if (state.dualSourceBlendEnabled)
  {
    translateBlendParams(ret.blendStates[0], state.dualSourceBlend.params);
  }
  else
  {
    for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
      translateBlendParams(ret.blendStates[i], state.blendParams[i]);
  }

  return ret;
}

RenderStateSystem::DynamicState RenderStateSystemBackend::extractDynamicState(const shaders::RenderState &state)
{
  RenderStateSystem::DynamicState ret;

  if (state.stencil.func > 0)
  {
    ret.stencilMask = (state.stencil.readMask << 8) | state.stencil.writeMask;
    ret.stencilRef = state.stencilRef;
  }
  else
  {
    ret.stencilRef = 0;
    ret.stencilMask = 0;
  }

  ret.enableScissor = state.scissorEnabled;
  ret.depthBias = state.zBias;
  ret.slopedDepthBias = state.slopeZBias;

  return ret;
}

void RenderStateSystemBackend::setRenderStateData(shaders::DriverRenderStateId id, const shaders::RenderState &state)
{
  RenderStateSystem::DynamicState dynamicPart = extractDynamicState(state);
  GraphicsPipelineStaticState staticPart = extractStaticState(state);

  if (DAGOR_UNLIKELY(state.dualSourceBlendEnabled && !Globals::VK::phy.features.dualSrcBlend))
  {
#if DAGOR_DBGLEVEL > 0
    D3D_CONTRACT_ERROR(
      "vulkan: Trying to create render state %u with dual source blending, but device does not support it. Check driver desc before "
      "calling such shaders.");
#endif

    for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
    {
      auto &st = staticPart.blendStates[i];
      st.blendSrcFactor = translate_vulkan_blend_mode_to_no_dual_source_mode(VkBlendFactor(st.blendSrcFactor));
      st.blendDstFactor = translate_vulkan_blend_mode_to_no_dual_source_mode(VkBlendFactor(st.blendDstFactor));
      st.blendSrcAlphaFactor = translate_vulkan_blend_mode_to_no_dual_source_mode(VkBlendFactor(st.blendSrcAlphaFactor));
      st.blendDstAlphaFactor = translate_vulkan_blend_mode_to_no_dual_source_mode(VkBlendFactor(st.blendDstAlphaFactor));
    }
  }

  uint32_t intId = (uint32_t)id;
  if (states.size() <= intId)
    states.resize(intId + 1);

  auto &stateRef = states[intId];
  stateRef.staticIdx = addOrReusePart(staticParts, staticPart);
  stateRef.dynamicIdx = addOrReusePart(dynamicParts, dynamicPart);

  debug("vulkan: added render state %u -> [s %u, d %u], total [s %u, d %u]", intId, stateRef.staticIdx, stateRef.dynamicIdx,
    staticParts.size(), dynamicParts.size());
}

shaders::DriverRenderStateId RenderStateSystem::registerState(DeviceContext &ctx, const shaders::RenderState &state)
{
  auto newId = shaders::DriverRenderStateId{++maxId};

  ctx.addRenderState(newId, state);
  return newId;
}