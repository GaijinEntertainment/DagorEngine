// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/bfg.h>
#include <render/debugMesh.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraParams.h>
#include <render/world/gbufferConsts.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_texture.h>

#define TRANSPARENCY_NODE_PRIORITY_HAIRS     1
#define TRANSPARENCY_NODE_PRIORITY_RENDINST  3
#define TRANSPARENCY_NODE_PRIORITY_BIN_SCENE 6
#define TRANSPARENCY_NODE_PRIORITY_ECS       7
#define TRANSPARENCY_NODE_PRIORITY_PARTICLES 9

extern ConVarT<bool, false> use_24bit_depth;

extern bool need_hero_cockpit_flag_in_prepass;

// Used in place of actual data when nodes communicate with each other
// through a hidden chanel, such as global state or a shared storage.
// Should be avoided and refactored out as much as possible in favor of
// explicitly passing such data through blobs.
struct OrderingToken
{};


inline void use_current_camera(dabfg::Registry &registry)
{
  registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
  registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
}

inline void render_transparency(dabfg::Registry &registry, const char *depth_bind = nullptr)
{
  auto reactiveMask = registry.modify("reactive_mask").texture().optional();

  if (depth_bind)
    registry.allowAsyncPipelines()
      .requestRenderPass()
      .color({"color_target", reactiveMask})
      .depthRoAndBindToShaderVars("depth", {depth_bind});
  else
    registry.allowAsyncPipelines().requestRenderPass().color({"color_target", reactiveMask}).depthRo("depth");
  use_current_camera(registry);
}
inline void render_transparency_published(dabfg::Registry &registry)
{
  registry.requestRenderPass().color({"target_for_transparency"}).depthRw("depth_for_transparency");
  use_current_camera(registry);
}

inline void start_transparent_region(dabfg::Registry registry, dabfg::NameSpaceRequest previous_region_ns)
{
  registry.requestRenderPass()
    .color({previous_region_ns.rename("color_target_done", "color_target", dabfg::History::No).texture()})
    .depthRo(previous_region_ns.rename("depth_done", "depth", dabfg::History::No).texture());
}

inline void end_transparent_region(dabfg::Registry registry)
{
  registry.requestRenderPass()
    .color({registry.rename("color_target", "color_target_done", dabfg::History::No).texture()})
    .depthRo(registry.rename("depth", "depth_done", dabfg::History::No).texture());
}

inline uint32_t get_frame_render_target_format()
{
  if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING) && !renderer_has_feature(FeatureRenderFlags::POSTFX))
    return TEXFMT_R8G8B8A8;

  uint32_t rtFmt = TEXFMT_R11G11B10F;
  if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
    rtFmt = TEXFMT_A16B16G16R16F;
  return rtFmt;
}

inline uint32_t get_gbuffer_depth_format()
{
#if DAGOR_DBGLEVEL > 0
  if (use_24bit_depth)
    return TEXFMT_DEPTH24;
  if (debug_mesh::is_enabled())
    return TEXFMT_DEPTH32_S8;
#endif
  return TEXFMT_DEPTH32;
}


// The following "read" functions automatically bind gbuffer textures
// to shaderVars, completely mirroring the reading macros you use in
// shaders. The relevant macro name is in the comments before the
// functions.
// Note that this can only be used to read the "final" state of the
// g-buffer, i.e. after ALL of rendering to it is completely done.

namespace readgbuffer
{
enum TextureFlags
{
  ALBEDO = 1,
  NORMAL = 2,
  MATERIAL = 4,
  ALL = ALBEDO | NORMAL | MATERIAL
};
}

// INIT_READ_MATERIAL_GBUFFER_ONLY
inline void read_gbuffer_material_only(dabfg::Registry registry, dabfg::Stage stage = dabfg::Stage::PS, int which = readgbuffer::ALL)
{
  if (which & readgbuffer::MATERIAL) // Not available on bare minimum
  {
    registry.read("gbuf_2").texture().atStage(stage).bindToShaderVar("material_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate").optional();
  }
};

// INIT_READ_GBUFFER
inline void read_gbuffer(dabfg::Registry registry, dabfg::Stage stage = dabfg::Stage::PS, int which = readgbuffer::ALL)
{
  if (which & readgbuffer::ALBEDO)
  {
    registry.read("gbuf_0").texture().atStage(stage).bindToShaderVar("albedo_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("albedo_gbuf_samplerstate");
  }
  if (which & readgbuffer::NORMAL)
  {
    registry.read("gbuf_1").texture().atStage(stage).bindToShaderVar("normal_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
  }
  read_gbuffer_material_only(registry, stage, which);
};

// INIT_READ_DEPTH_GBUFFER
inline void read_gbuffer_depth(dabfg::Registry registry, dabfg::Stage stage = dabfg::Stage::PS)
{
  registry.readTexture("gbuf_depth").atStage(stage).bindToShaderVar("depth_gbuf");
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
}

// INIT_READ_MOTION_BUFFER
inline auto read_gbuffer_motion(dabfg::Registry registry, dabfg::Stage stage = dabfg::Stage::PS)
{
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate").optional();
  return registry.readTexture("motion_vecs").atStage(stage).bindToShaderVar("motion_gbuf"); // You can call .optional() on return value
}

inline auto render_to_gbuffer(dabfg::Registry registry)
{
  return registry.allowAsyncPipelines()
    .requestRenderPass()
    .depthRw("gbuf_depth")
    .color({"gbuf_0", "gbuf_1", registry.modify("gbuf_2").texture().optional(), registry.modify("gbuf_3").texture().optional()});
}

inline auto render_to_gbuffer_prepass(dabfg::Registry registry)
{
  return registry.allowAsyncPipelines().requestRenderPass().depthRw("gbuf_depth_prepass");
}

// Depth can be sampled through depth_gbuf
inline auto render_to_gbuffer_but_sample_depth(dabfg::Registry registry)
{
  return registry.allowAsyncPipelines()
    .requestRenderPass()
    .depthRoAndBindToShaderVars("gbuf_depth", {"depth_gbuf"})
    .color({"gbuf_0", "gbuf_1", registry.modify("gbuf_2").texture().optional(), registry.modify("gbuf_3").texture().optional()});
}

inline void start_gbuffer_rendering_region(dabfg::Registry registry, dabfg::NameSpaceRequest previous_region_ns, bool use_prepass)
{
  // we rename "opaque/DYNAMICS/gbuf_0_done" to "opaque/STATICS/gbuf_0" and so on
  registry.requestRenderPass()
    .color({
      previous_region_ns.rename("gbuf_0_done", "gbuf_0", dabfg::History::No).texture(),
      previous_region_ns.rename("gbuf_1_done", "gbuf_1", dabfg::History::No).texture(),
      previous_region_ns.rename("gbuf_2_done", "gbuf_2", dabfg::History::No).texture().optional(),
      previous_region_ns.rename("gbuf_3_done", "gbuf_3", dabfg::History::No).texture().optional(),
    })
    .depthRw(
      previous_region_ns.rename("gbuf_depth_done", use_prepass ? "gbuf_depth_prepass" : "gbuf_depth", dabfg::History::No).texture());
}

inline void complete_prepass_of_gbuffer_rendering_region(dabfg::Registry registry)
{
  registry.requestRenderPass().depthRw(registry.rename("gbuf_depth_prepass", "gbuf_depth", dabfg::History::No).texture());
}

inline void end_gbuffer_rendering_region(dabfg::Registry registry)
{
  // An additional rename is required for closing ending clipmap feedback
  // reliably. Using priorities is a bad idea, cuz other nodes might want
  // to be rendered after every other static thing. Using explicit
  // node ordering is a PITA, cuz you would have to order every single
  // opaque rendering node with this one, including optional ones from
  // libraries.
  // Similar cases might occur for other regions of rendering stuff,
  // so we require a rename at the end of every such namespace
  registry.requestRenderPass()
    .color({
      registry.rename("gbuf_0", "gbuf_0_done", dabfg::History::No).texture(),
      registry.rename("gbuf_1", "gbuf_1_done", dabfg::History::No).texture(),
      registry.rename("gbuf_2", "gbuf_2_done", dabfg::History::No).texture().optional(),
      registry.rename("gbuf_3", "gbuf_3_done", dabfg::History::No).texture().optional(),
    })
    .depthRw(registry.rename("gbuf_depth", "gbuf_depth_done", dabfg::History::No).texture());
}

inline const vec4f &get_jitter_frustumPlane03X(const CameraParams &params) { return params.jitterFrustum.plane03X; }

inline const vec4f &get_jitter_frustumPlane03Y(const CameraParams &params) { return params.jitterFrustum.plane03Y; }

inline const vec4f &get_jitter_frustumPlane03Z(const CameraParams &params) { return params.jitterFrustum.plane03Z; }

inline const vec4f &get_jitter_frustumPlane03W(const CameraParams &params) { return params.jitterFrustum.plane03W; }

inline const vec4f &get_jitter_frustumPlane4(const CameraParams &params) { return params.jitterFrustum.camPlanes[4]; }

inline const vec4f &get_jitter_frustumPlane5(const CameraParams &params) { return params.jitterFrustum.camPlanes[5]; }

inline const vec4f &get_no_jitter_frustumPlane03X(const CameraParams &params) { return params.noJitterFrustum.plane03X; }

inline const vec4f &get_no_jitter_frustumPlane03Y(const CameraParams &params) { return params.noJitterFrustum.plane03Y; }

inline const vec4f &get_no_jitter_frustumPlane03Z(const CameraParams &params) { return params.noJitterFrustum.plane03Z; }

inline const vec4f &get_no_jitter_frustumPlane03W(const CameraParams &params) { return params.noJitterFrustum.plane03W; }

inline const vec4f &get_no_jitter_frustumPlane4(const CameraParams &params) { return params.noJitterFrustum.camPlanes[4]; }

inline const vec4f &get_no_jitter_frustumPlane5(const CameraParams &params) { return params.noJitterFrustum.camPlanes[5]; }

/**
 * This is similar to calling set_frustum_planes(...), but it is handled by the FG.
 * It sets the frustum plane shader vars according to the current_camera::jitterFrustum
 */
inline void use_jitter_frustum_plane_shader_vars(dabfg::Registry registry)
{
  registry.readBlob<CameraParams>("current_camera")
    .bindToShaderVar<get_jitter_frustumPlane03X>("frustumPlane03X")
    .bindToShaderVar<get_jitter_frustumPlane03Y>("frustumPlane03Y")
    .bindToShaderVar<get_jitter_frustumPlane03Z>("frustumPlane03Z")
    .bindToShaderVar<get_jitter_frustumPlane03W>("frustumPlane03W")
    .bindToShaderVar<get_jitter_frustumPlane4>("frustumPlane4")
    .bindToShaderVar<get_jitter_frustumPlane5>("frustumPlane5");
}

/**
 * This is similar to calling set_frustum_planes(...), but it is handled by the FG.
 * It sets the frustum plane shader vars according to the current_camera::noJitterFrustum
 */
inline void use_no_jitter_frustum_plane_shader_vars(dabfg::Registry registry)
{

  registry.readBlob<CameraParams>("current_camera")
    .bindToShaderVar<get_no_jitter_frustumPlane03X>("frustumPlane03X")
    .bindToShaderVar<get_no_jitter_frustumPlane03Y>("frustumPlane03Y")
    .bindToShaderVar<get_no_jitter_frustumPlane03Z>("frustumPlane03Z")
    .bindToShaderVar<get_no_jitter_frustumPlane03W>("frustumPlane03W")
    .bindToShaderVar<get_no_jitter_frustumPlane4>("frustumPlane4")
    .bindToShaderVar<get_no_jitter_frustumPlane5>("frustumPlane5");
}

inline void use_volfog(dabfg::Registry registry, dabfg::Stage stage)
{
  registry.readBlob<OrderingToken>("volfog_ff_result_token").optional();
  registry.readBlob<OrderingToken>("volfog_df_result_token").optional();
  {
    // for DF upscaling // ideally, we would only check these if DF is on
    registry.readTexture("checkerboard_depth").atStage(stage).bindToShaderVar("downsampled_checkerboard_depth_tex").optional();
    registry.read("checkerboard_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
      .optional();
  }
}

inline void use_volfog_shadow(dabfg::Registry registry, dabfg::Stage stage)
{
  G_UNUSED(stage); // currently volfog shadow resources are fully outside of FG
  registry.readBlob<OrderingToken>("volfog_shadow_token").optional();
}
