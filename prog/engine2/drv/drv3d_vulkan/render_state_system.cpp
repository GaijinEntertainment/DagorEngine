#include "render_state_system.h"
#include "device.h"

using namespace drv3d_vulkan;

GraphicsPipelineStaticState RenderStateSystem::Backend::extractStaticState(const shaders::RenderState &state, Device &device)
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
  ret.blendOp = translate_blend_op_to_vulkan(state.blendOp);
  ret.blendOpAlpha = translate_blend_op_to_vulkan(state.sepablendOp);
  ret.conservativeRasterEnable = state.conservativeRaster;
  ret.depthBoundsEnable = state.depthBoundsEnable;
  ret.colorMask = state.colorWr;
  ret.alphaToCoverage = state.alphaToCoverage;

  if (device.getDeviceProperties().features.depthClamp != VK_FALSE)
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

  ret.blendDstAlphaFactor = VK_BLEND_FACTOR_ZERO;
  ret.blendSrcAlphaFactor = VK_BLEND_FACTOR_ONE;

  if (state.ablend)
  {
    ret.blendEnable = 1;
    ret.blendDstFactor = translate_rgb_blend_mode_to_vulkan(state.ablendDst);
    ret.blendSrcFactor = translate_rgb_blend_mode_to_vulkan(state.ablendSrc);
  }
  else
  {
    ret.blendEnable = 0;
    ret.blendSeparateAlphaEnable = 0;
    ret.blendDstFactor = VK_BLEND_FACTOR_ZERO;
    ret.blendSrcFactor = VK_BLEND_FACTOR_ONE;
    ret.blendOp = VK_BLEND_OP_ADD;
    ret.blendOpAlpha = VK_BLEND_OP_ADD;
  }

  if (state.sepablend)
  {
    ret.blendSeparateAlphaEnable = 1;
    ret.blendDstAlphaFactor = translate_alpha_blend_mode_to_vulkan(state.sepablendDst);
    ret.blendSrcAlphaFactor = translate_alpha_blend_mode_to_vulkan(state.sepablendSrc);
  }
  else
    ret.blendSeparateAlphaEnable = 0;

  return ret;
}

RenderStateSystem::DynamicState RenderStateSystem::Backend::extractDynamicState(const shaders::RenderState &state)
{
  DynamicState ret;

  if (state.stencil.func > 0)
  {
    // FIXME: separate read & write mask?
    ret.stencilMask = state.stencil.readMask | state.stencil.writeMask;
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

void RenderStateSystem::Backend::setRenderStateData(shaders::DriverRenderStateId id, const shaders::RenderState &state, Device &device)
{
  DynamicState dynamicPart = extractDynamicState(state);
  GraphicsPipelineStaticState staticPart = extractStaticState(state, device);

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