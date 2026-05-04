// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/nodeHandle.h>
#include <render/world/gbufferConsts.h>
#include <EASTL/span.h>
#include <render/daFrameGraph/daFG.h>
#include <render/viewVecs.h>
#include "dynamicMirrorRenderer.h"

constexpr float MIRROR_RESOLUTION_SCALE = 0.25f;

inline auto get_dynamic_mirrors_namespace() { return dafg::root() / "mirrors"; }

struct DynamicMirrorShaderVars
{
  Frustum frustum;
  IPoint4 gbufferViewSize;
  Color4 screenPosToTexcoord;
  ViewVecs viewVecs;

  static const Point4 &get_view_vecLT(const DynamicMirrorShaderVars &params) { return params.viewVecs.viewVecLT; }
  static const Point4 &get_view_vecRT(const DynamicMirrorShaderVars &params) { return params.viewVecs.viewVecRT; }
  static const Point4 &get_view_vecLB(const DynamicMirrorShaderVars &params) { return params.viewVecs.viewVecLB; }
  static const Point4 &get_view_vecRB(const DynamicMirrorShaderVars &params) { return params.viewVecs.viewVecRB; }
  static const IPoint4 &get_gbufferViewSize(const DynamicMirrorShaderVars &params) { return params.gbufferViewSize; }
  static const Color4 &get_screenPosToTexcoord(const DynamicMirrorShaderVars &params) { return params.screenPosToTexcoord; }
  static const vec4f &get_frustumPlane03X(const DynamicMirrorShaderVars &params) { return params.frustum.plane03X; }
  static const vec4f &get_frustumPlane03Y(const DynamicMirrorShaderVars &params) { return params.frustum.plane03Y; }
  static const vec4f &get_frustumPlane03Z(const DynamicMirrorShaderVars &params) { return params.frustum.plane03Z; }
  static const vec4f &get_frustumPlane03W(const DynamicMirrorShaderVars &params) { return params.frustum.plane03W; }
  static const vec4f &get_frustumPlane4(const DynamicMirrorShaderVars &params) { return params.frustum.camPlanes[4]; }
  static const vec4f &get_frustumPlane5(const DynamicMirrorShaderVars &params) { return params.frustum.camPlanes[5]; }

  // TODO This should not exist. Shader variables should be all handled by the framegraph, but for now they aren't.
  struct ScopedVars
  {
    ScopedVars(const IPoint4 &gbuffer_view_size, const Color4 &screen_pos_to_texcoord, const ViewVecs &view_vecs);
    ~ScopedVars();

    IPoint4 originalGbufferViewSize;
    Color4 originalScreenPosToTexcoord;
    ViewVecs originalViewVecs;
  };
  ScopedVars getScopedVars() const { return {gbufferViewSize, screenPosToTexcoord, viewVecs}; }
};


inline auto use_mirror_shader_vars(dafg::Registry registry)
{
  return registry.readBlob<DynamicMirrorShaderVars>("mirror_vars")
    .bindToShaderVar<DynamicMirrorShaderVars::get_frustumPlane03X>("frustumPlane03X")
    .bindToShaderVar<DynamicMirrorShaderVars::get_frustumPlane03Y>("frustumPlane03Y")
    .bindToShaderVar<DynamicMirrorShaderVars::get_frustumPlane03Z>("frustumPlane03Z")
    .bindToShaderVar<DynamicMirrorShaderVars::get_frustumPlane03W>("frustumPlane03W")
    .bindToShaderVar<DynamicMirrorShaderVars::get_frustumPlane4>("frustumPlane4")
    .bindToShaderVar<DynamicMirrorShaderVars::get_frustumPlane5>("frustumPlane5")
    .handle();
  // TODO bind these shader vars as soon as they are compatible with the framegraph
  //    .bindToShaderVar<DynamicMirrorShaderVars::get_gbufferViewSize>("gbuffer_view_size")
  //    .bindToShaderVar<DynamicMirrorShaderVars::get_screenPosToTexcoord>("screen_pos_to_texcoord")
  //    .bindToShaderVar<DynamicMirrorShaderVars::get_view_vecLT>("view_vecLT")
  //    .bindToShaderVar<DynamicMirrorShaderVars::get_view_vecRT>("view_vecRT")
  //    .bindToShaderVar<DynamicMirrorShaderVars::get_view_vecLB>("view_vecLB")
  //    .bindToShaderVar<DynamicMirrorShaderVars::get_view_vecRB>("view_vecRB");
}

inline auto use_mirror_gbuffer_pass(dafg::Registry registry)
{
  registry.requestRenderPass()
    .depthRw("mirror_gbuf_depth")
    .color({"mirror_gbuf_0", "mirror_gbuf_1", registry.modify("mirror_gbuf_2").texture().optional(),
      registry.modify("mirror_gbuf_3").texture().optional()});

  use_mirror_shader_vars(registry);

  return registry.readBlob<DynamicMirrorRenderer::CameraData>("mirror_camera")
    .bindAsView<&DynamicMirrorRenderer::CameraData::viewTm>()
    .bindAsProj<&DynamicMirrorRenderer::CameraData::projTm>()
    .handle();
}

inline auto use_mirror_gbuffer_prepass(dafg::Registry registry)
{
  registry.requestRenderPass().depthRw("mirror_prepass_gbuf_depth");

  use_mirror_shader_vars(registry);

  return registry.readBlob<DynamicMirrorRenderer::CameraData>("mirror_camera")
    .bindAsView<&DynamicMirrorRenderer::CameraData::viewTm>()
    .bindAsProj<&DynamicMirrorRenderer::CameraData::prepassProjTm>()
    .handle();
}


inline auto get_mirror_resolution(dafg::Registry registry) { return registry.getResolution<2>("main_view", MIRROR_RESOLUTION_SCALE); }

class DynamicMirrorRenderer;

dafg::NodeHandle create_dynamic_mirror_prepare_node(DynamicMirrorRenderer &mirror_renderer,
  uint32_t global_flags,
  uint32_t gbuf_cnt,
  eastl::span<uint32_t> main_gbuf_fmts,
  int depth_format);
dafg::NodeHandle create_dynamic_mirror_prepass_node(DynamicMirrorRenderer &mirror_renderer);
dafg::NodeHandle create_dynamic_mirror_end_prepass_node();
dafg::NodeHandle create_dynamic_mirror_render_dynamic_node();
dafg::NodeHandle create_dynamic_mirror_render_static_node(DynamicMirrorRenderer &mirror_renderer);
dafg::NodeHandle create_dynamic_mirror_render_ground_node();
dafg::NodeHandle create_dynamic_mirror_gbuf_resolve_node(const char *resolve_pshader_name);
dafg::NodeHandle create_dynamic_mirror_resolve_node(DynamicMirrorRenderer &mirror_renderer);
dafg::NodeHandle create_dynamic_mirror_envi_node(DynamicMirrorRenderer &mirror_renderer);