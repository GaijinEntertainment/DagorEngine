// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "screenSpaceReflections_api.h"
#include <render/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/autoResolutionRequest.h>
#include <drv/3d/dag_resource.h>
#include <EASTL/variant.h>

namespace render::ssr
{

struct ResourceNames
{
  const char *closeDepthResName = nullptr;
  const char *normalsResName = "downsampled_normals";
  const char *motionVectorsResName = "downsampled_motion_vectors_tex";

  const char *closeDepthSamplerResName = nullptr;
  const char *normalsSamplerResName = nullptr;
  const char *motionVectorsSamplerResName = nullptr;
  const char *prevFrameSamplerResName = "prev_frame_sampler";

  const char *motionVectorsVar = "downsampled_motion_vectors_tex";

  const char *checkerboardDepthResName = nullptr;

  const char *gbufSamplerResName = nullptr;
};

inline constexpr const char *frame_params_blob_name = "ssr_frame_params";
inline constexpr const char *camera_state_blob_name = "ssr_camera_state";

struct FrameParams
{
  // Set when the project fills the camera state blob itself, the trace node takes it from current_camera otherwise.
  bool rebase = false;
  bool historyValid = false;
};

// Creates the camera the ssr traces with.
dafg::VirtualResourceHandle<ssr_pass::CameraState, false, false> create_camera_state_blob(dafg::Registry registry);

struct TargetDesc
{
  uint32_t fmt = 0;
  SSRQuality quality = {};
  // Half res or full res.
  bool isFullres = false;
};

// Only the compute path reprojects the previous normals, the ps one has to see prev_downsampled_normals as NULL.
inline bool ssr_needs_downsampled_normals_history(SSRQuality quality) { return quality == SSRQuality::Compute; }

namespace detail
{

// Which parts of the shared input set a node reads.
struct InputParameters
{
  TargetDesc target;
  dafg::Stage stage = dafg::Stage::PS_OR_CS;
  bool readPrevFrame = false;
  bool readCheckerboardDepth = false;
  bool readMaterialGbuf = false;
  bool readNormalsHistory = false;
};

// The only place that maps the ssr shader inputs to daFG resources and shader var slots.
void declare_inputs(dafg::Registry registry, const ResourceNames &names, const InputParameters &params);

inline bool use_simple_denoiser(const TargetDesc &target, bool requested)
{
  // The ps path doesn't support the simple denoiser for now.
  return requested && target.quality == SSRQuality::Compute;
}

} // namespace detail

struct ResourceProviderParameters
{
  TargetDesc target;
  d3d::AddressMode samplerAddressMode = d3d::AddressMode::Clamp;
  d3d::BorderColor::Color samplerBorderColor = d3d::BorderColor::Color::TransparentBlack;
};

// Creates "ssr_target_before_denoise" (target-sized, mip count from
// ssr_pass::get_mip_count(quality)), "ssr_target_sampler" and the shaders every view shares.
dafg::NodeHandle make_resource_provider_node(dafg::NameSpace ns, const ResourceProviderParameters &params);

struct DenoiserParameters
{
  ResourceNames resourceNames;
  // Namespace of the trace node, relative to the root, when it is not the denoise node's own one.
  const char *traceViewNsName = nullptr;
  TargetDesc target;
  bool useSimpleDenoiser = false;
  bool readMaterialGbuf = false;
  bool readNormalsHistory = true;
};

// Either renames ssr_target_before_denoise -> ssr_target (useSimpleDenoiser == false), or
// dispatches ssr_temporal_denoise_cs into a newly created one.
dafg::NodeHandle make_denoiser_node(dafg::NameSpace ns, const DenoiserParameters &params);

struct TraceNodeParameters
{
  ResourceNames resourceNames;
  TargetDesc target;
  bool useSimpleDenoiser = false;
  bool readMaterialGbuf = false;
  bool readNormalsHistory = true;
  bool isMainView = true;
};

namespace detail
{

// The trace resources that do not depend on the project's context type.
struct TraceResources
{
  dafg::VirtualResourceHandle<BaseTexture, true, false> target;
  dafg::VirtualResourceHandle<const FrameParams, false, false> frameParams;
  dafg::VirtualResourceHandle<ssr_pass::CameraState, false, false> ssrCamera;
  dafg::VirtualResourceHandle<const ssr_pass::CameraState, false, false> prevSsrCamera;
  dafg::VirtualResourceHandle<const CameraParams, false, false> camera;
  dafg::VirtualResourceHandle<ssr_pass::RenderResources *const, false, false> renderResources;
  dafg::AutoResolutionRequest<2> resolution;
  int32_t targetMipCount = 1;
  bool useSimpleDenoiser = false;
  bool isMainView = true;
};

TraceResources declare_trace_resources(dafg::Registry registry, const TraceNodeParameters &params);
void prepare_ssr(const TraceResources &res);
void render_ssr(const TraceResources &res, const dafg::multiplexing::Index &multiplexing_index);

} // namespace detail

// The trace node.
// Context is an RAII guard around the trace call, constructed as
// Context(const CameraParams &camera, const ssr_pass::CameraState &ssr_camera, const FrameParams &params, bool is_main_view).
// An optional static Context::declare(dafg::Registry) adds the project's own requests to the node.
// DoneToken types the done blob.
template <class Context, class DoneToken = eastl::monostate>
dafg::NodeHandle make_trace_node(dafg::NameSpace ns, const TraceNodeParameters &params)
{
  return ns.registerNode("ssr_node", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    registry.createBlob<DoneToken>("after_ssr_node_token");
    const detail::TraceResources res = detail::declare_trace_resources(registry, params);
    if constexpr (requires { Context::declare(registry); })
      Context::declare(registry);

    return [res](const dafg::multiplexing::Index &multiplexing_index) {
      detail::prepare_ssr(res);

      // Project specific context.
      const Context ctx(res.camera.ref(), res.ssrCamera.ref(), res.frameParams.ref(), res.isMainView);
      detail::render_ssr(res, multiplexing_index);
    };
  });
}

} // namespace render::ssr
