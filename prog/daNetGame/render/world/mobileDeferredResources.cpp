// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/mobileDeferredResources.h>

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <render/vertexDensityOverlay.h>
#include <render/world/gbufferConsts.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/subpass_registers.hlsli>
#include <drv/3d/dag_renderPass.h>

MobileDeferredResources::MobileDeferredResources(int rt_fmt, int depth_fmt, bool simplified)
{
  initShaders();
  initRenderPasses(rt_fmt, depth_fmt, simplified);

  VertexDensityOverlay::init();
  vertexDensityInitStatus = true;
}

void MobileDeferredResources::initShaders()
{
  deferredResolve = eastl::make_unique<PostFxRenderer>();
  deferredResolve->init("mobile_deferred_resolve");
}

void MobileDeferredResources::initRenderPasses(int rt_fmt, int depth_fmt, bool simplified)
{
  const uint32_t rtFmt = rt_fmt | TEXCF_RTARGET;
  const uint32_t depthFmt = depth_fmt | TEXCF_RTARGET;

  auto targets = getTargetDescriptions(rtFmt, depthFmt, simplified);
  eastl::vector<RenderPassBind> opaqueBinds = constructOpaqueBinds(simplified, OPAQUE_SP, DECALS_SP, RESOLVE_SP, PANORAMA_APPLY_SP);

  opaque = d3d::create_render_pass(
    {"opaque", (uint32_t)targets.size(), (uint32_t)opaqueBinds.size(), targets.data(), opaqueBinds.data(), SP_REG_BASE});

  ShaderGlobal::set_int(get_shader_variable_id("mobile_simplified_materials", false), simplified ? 1 : 0);
}

eastl::fixed_vector<RenderPassTargetDesc, MobileDeferredResources::FULL_MAT_MAIN_RT_COUNT + 1> MobileDeferredResources::
  getTargetDescriptions(uint32_t rtFmt, uint32_t depthFmt, bool simplified)
{
  if (simplified)
    return {RenderPassTargetDesc{nullptr, MOBILE_SIMPLIFIED_GBUFFER_FORMATS[0], false},
      RenderPassTargetDesc{nullptr, MOBILE_SIMPLIFIED_GBUFFER_FORMATS[1], false}, RenderPassTargetDesc{nullptr, depthFmt, false},
      RenderPassTargetDesc{nullptr, rtFmt, false}};
  else
    return {RenderPassTargetDesc{nullptr, MOBILE_GBUFFER_FORMATS[0], false},
      RenderPassTargetDesc{nullptr, MOBILE_GBUFFER_FORMATS[1], false}, RenderPassTargetDesc{nullptr, MOBILE_GBUFFER_FORMATS[2], false},
      RenderPassTargetDesc{nullptr, depthFmt, false}, RenderPassTargetDesc{nullptr, rtFmt, false}};
}

eastl::vector<RenderPassBind> MobileDeferredResources::constructOpaqueBinds(
  bool simplified, int32_t opaqueSp, int32_t decalsSp, int32_t resolveSp, int32_t panoramaApplySp)
{
  const RenderPassTargetAction clearAction = opaqueSp == 0 ? RP_TA_LOAD_CLEAR : RP_TA_NONE;

  if (simplified)
    return {
      // opaque
      {SIMPLE_MAT_IGBUF0, opaqueSp, SP_MAIN_PASS_MRT_GBUF0, RP_TA_SUBPASS_WRITE | clearAction, rwMrt},
      {SIMPLE_MAT_IGBUF1, opaqueSp, SP_MAIN_PASS_MRT_GBUF1, RP_TA_SUBPASS_WRITE | clearAction, rwMrt},
      {SIMPLE_MAT_IDEPTH, opaqueSp, iDS, RP_TA_SUBPASS_WRITE | clearAction, rwDepth},
      // decals
      {SIMPLE_MAT_IGBUF0, decalsSp, SP_MAIN_PASS_MRT_GBUF0, RP_TA_SUBPASS_WRITE, rwMrt},
      {SIMPLE_MAT_IDEPTH, decalsSp, SP_DECALS_IA_DEPTH, RP_TA_SUBPASS_READ, roDepth},
      {SIMPLE_MAT_IGBUF1, decalsSp, SP_DECALS_IA_MASK, RP_TA_SUBPASS_READ, inputAtt},
      // resolve
      {SIMPLE_MAT_IRESOLVE, resolveSp, SP_RESOLVE_MRT_RESOLVE_OUTPUT, RP_TA_SUBPASS_WRITE | RP_TA_LOAD_NO_CARE, rwMrt},
      {SIMPLE_MAT_IGBUF0, resolveSp, SP_RESOLVE_IA_GBUF0, RP_TA_SUBPASS_READ, inputAtt},
      {SIMPLE_MAT_IGBUF1, resolveSp, SP_RESOLVE_IA_GBUF1, RP_TA_SUBPASS_READ, inputAtt},
      {SIMPLE_MAT_IDEPTH, resolveSp, SP_RESOLVE_IA_DEPTH, RP_TA_SUBPASS_READ, roDepth},
      // apply panorama
      {SIMPLE_MAT_IRESOLVE, panoramaApplySp, SP_PANORAMA_MRT_RESOLVE_OUTPUT, RP_TA_SUBPASS_WRITE, rwMrt},
      {SIMPLE_MAT_IDEPTH, panoramaApplySp, SP_PANORAMA_IA_DEPTH, RP_TA_SUBPASS_READ, roDepth},
      // rp end
      {SIMPLE_MAT_IGBUF0, iEX, 0, RP_TA_STORE_NO_CARE, rwMrt},
      {SIMPLE_MAT_IGBUF1, iEX, 2, RP_TA_STORE_NO_CARE, rwMrt},
      {SIMPLE_MAT_IDEPTH, iEX, 0, RP_TA_STORE_WRITE, rwDepth},
      {SIMPLE_MAT_IRESOLVE, iEX, 0, RP_TA_STORE_WRITE, rwMrt},
    };
  else
    return {
      // opaque
      {FULL_MAT_IGBUF0, opaqueSp, SP_MAIN_PASS_MRT_GBUF0, RP_TA_SUBPASS_WRITE | clearAction, rwMrt},
      {FULL_MAT_IGBUF1, opaqueSp, SP_MAIN_PASS_MRT_GBUF1, RP_TA_SUBPASS_WRITE | clearAction, rwMrt},
      {FULL_MAT_IGBUF2, opaqueSp, SP_MAIN_PASS_MRT_GBUF2, RP_TA_SUBPASS_WRITE | clearAction, rwMrt},
      {FULL_MAT_IDEPTH, opaqueSp, iDS, RP_TA_SUBPASS_WRITE | clearAction, rwDepth},
      // decals
      {FULL_MAT_IGBUF0, decalsSp, SP_MAIN_PASS_MRT_GBUF0, RP_TA_SUBPASS_WRITE, rwMrt},
      {FULL_MAT_IGBUF1, decalsSp, SP_MAIN_PASS_MRT_GBUF1, RP_TA_SUBPASS_WRITE, rwMrt},
      {FULL_MAT_IDEPTH, decalsSp, SP_DECALS_IA_DEPTH, RP_TA_SUBPASS_READ, roDepth},
      {FULL_MAT_IGBUF2, decalsSp, SP_DECALS_IA_MASK, RP_TA_SUBPASS_READ, inputAtt},
      // resolve
      {FULL_MAT_IRESOLVE, resolveSp, SP_RESOLVE_MRT_RESOLVE_OUTPUT, RP_TA_SUBPASS_WRITE | RP_TA_LOAD_NO_CARE, rwMrt},
      {FULL_MAT_IGBUF0, resolveSp, SP_RESOLVE_IA_GBUF0, RP_TA_SUBPASS_READ, inputAtt},
      {FULL_MAT_IGBUF1, resolveSp, SP_RESOLVE_IA_GBUF1, RP_TA_SUBPASS_READ, inputAtt},
      {FULL_MAT_IGBUF2, resolveSp, SP_RESOLVE_IA_GBUF2, RP_TA_SUBPASS_READ, inputAtt},
      {FULL_MAT_IDEPTH, resolveSp, SP_RESOLVE_IA_DEPTH, RP_TA_SUBPASS_READ, roDepth},
      // apply panorama
      {FULL_MAT_IRESOLVE, panoramaApplySp, SP_PANORAMA_MRT_RESOLVE_OUTPUT, RP_TA_SUBPASS_WRITE, rwMrt},
      {FULL_MAT_IDEPTH, panoramaApplySp, SP_PANORAMA_IA_DEPTH, RP_TA_SUBPASS_READ, roDepth},
      // rp end
      {FULL_MAT_IGBUF0, iEX, 0, RP_TA_STORE_NO_CARE, rwMrt},
      {FULL_MAT_IGBUF1, iEX, 1, RP_TA_STORE_NO_CARE, rwMrt},
      {FULL_MAT_IGBUF2, iEX, 2, RP_TA_STORE_NO_CARE, rwMrt},
      {FULL_MAT_IDEPTH, iEX, 0, RP_TA_STORE_WRITE, rwDepth},
      {FULL_MAT_IRESOLVE, iEX, 0, RP_TA_STORE_WRITE, rwMrt},
    };
}

MobileDeferredResources::~MobileDeferredResources() { destroy(); }

void MobileDeferredResources::destroy()
{
  if (opaque)
  {
    d3d::delete_render_pass(opaque);
    opaque = nullptr;
  }

  deferredResolve.reset();

  if (vertexDensityInitStatus)
    VertexDensityOverlay::shutdown();
}

MobileDeferredResources &MobileDeferredResources::operator=(MobileDeferredResources &&rvl)
{
  eastl::swap(opaque, rvl.opaque);
  eastl::swap(deferredResolve, rvl.deferredResolve);
  eastl::swap(vertexDensityInitStatus, rvl.vertexDensityInitStatus);
  rvl.destroy();

  return *this;
}