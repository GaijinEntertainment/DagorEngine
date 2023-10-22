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

  ret.indenpendentBlendEnabled = state.independentBlendEnabled;
  for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
  {
    ret.blendStates[i].blendOp = translate_blend_op_to_vulkan(state.blendParams[i].blendOp);
    ret.blendStates[i].blendOpAlpha = translate_blend_op_to_vulkan(state.blendParams[i].sepablendOp);
    ret.blendStates[i].blendDstAlphaFactor = VK_BLEND_FACTOR_ZERO;
    ret.blendStates[i].blendSrcAlphaFactor = VK_BLEND_FACTOR_ONE;

    if (state.blendParams[i].ablend)
    {
      ret.blendStates[i].blendEnable = 1;
      ret.blendStates[i].blendDstFactor = translate_rgb_blend_mode_to_vulkan(state.blendParams[i].ablendFactors.dst);
      ret.blendStates[i].blendSrcFactor = translate_rgb_blend_mode_to_vulkan(state.blendParams[i].ablendFactors.src);
    }
    else
    {
      ret.blendStates[i].blendEnable = 0;
      ret.blendStates[i].blendSeparateAlphaEnable = 0;
      ret.blendStates[i].blendDstFactor = VK_BLEND_FACTOR_ZERO;
      ret.blendStates[i].blendSrcFactor = VK_BLEND_FACTOR_ONE;
      ret.blendStates[i].blendOp = VK_BLEND_OP_ADD;
      ret.blendStates[i].blendOpAlpha = VK_BLEND_OP_ADD;
    }

    if (state.blendParams[i].sepablend)
    {
      ret.blendStates[i].blendSeparateAlphaEnable = 1;
      ret.blendStates[i].blendDstAlphaFactor = translate_alpha_blend_mode_to_vulkan(state.blendParams[i].sepablendFactors.dst);
      ret.blendStates[i].blendSrcAlphaFactor = translate_alpha_blend_mode_to_vulkan(state.blendParams[i].sepablendFactors.src);
    }
    else
      ret.blendStates[i].blendSeparateAlphaEnable = 0;
  }

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