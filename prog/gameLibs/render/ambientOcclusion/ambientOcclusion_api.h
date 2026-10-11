// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/nodeHandle.h>
#include <render/daFrameGraph/nameSpace.h>
#include <render/daFrameGraph/registry.h>
#include <render/daFrameGraph/stage.h>
#include <render/daFrameGraph/usage.h>
#include <render/daFrameGraph/autoResolutionRequest.h>
#include <render/cameraParams.h>
#include "gtao_public.h"
#include "ssao_public.h"
#include <render/subFrameSample.h>
#include "ssao_common_public.h"
#include <render/viewDependentResource.h>
#include <EASTL/array.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>

// Shared SSAO/GTAO frame graph nodes. Both algorithms follow the same
// prepare -> render -> filter split:
//   prepare  creates ssao_tex (with history) and ssao_tmp_tex, and owns the node state
//   render   calls ssao_pass::resolve / gtao_pass::resolve
//   filter   calls ssao_pass::two_pass_blur / gtao_pass::spatiotemporal_filter
namespace ambient_occlusion
{

struct TargetDesc
{
  dafg::Stage stage = dafg::Stage::PS;
  uint32_t cflags = 0;

  dafg::Usage usage() const { return stage == dafg::Stage::CS ? dafg::Usage::SHADER_RESOURCE : dafg::Usage::COLOR_ATTACHMENT; }
};

TargetDesc make_ssao_target_desc(uint32_t creation_flags);
TargetDesc make_gtao_target_desc(uint32_t creation_flags);

struct ResourceNames
{
  // Depth pyramid.
  const char *closeDepthTex = nullptr;
  const char *closeDepthAliasShaderVar = nullptr;
  const char *closeDepthSampler = nullptr;
  const char *farDepthTex = nullptr;

  const char *motionVectorsTex = nullptr;
  const char *motionVectorsAliasShaderVar = nullptr;
  const char *motionVectorsSampler = nullptr;
  const char *motionVectorsHistoryShaderVar = nullptr;

  const char *normalsTex = nullptr;
  const char *normalsSampler = nullptr;

  const char *fomShadowsCosTex = nullptr;
  const char *fomShadowsSinTex = nullptr;
  const char *fomShadowsSampler = nullptr;
  const char *contactShadowTex = nullptr;

  const char *materialGbufferTex = nullptr;
  const char *materialGbufferSampler = nullptr;

  const char *subFrameSampleBlob = nullptr;
};

enum class CameraViewOrdering
{
  None,
  MainView,
  SecondaryView,
};
inline constexpr const char *camera_view_strict_ordering_token = "camera_view_strict_ordering_token";

inline constexpr const char *current_camera_blob = "current_camera";

struct SsaoPrepareParameters
{
  TargetDesc target;
  uint32_t creationFlags = SSAO_NONE;
};

struct GtaoPrepareParameters
{
  TargetDesc target;
  uint32_t creationFlags = SSAO_NONE;
};

struct SsaoRenderParameters
{
  TargetDesc target;
  ResourceNames names;
  // It tells what namespace we should use to request names.materialGbufferTex.
  bool materialGbufferBeforeRootPublish = false;

  bool isMainView = true;
  bool setViewFromMultiplexing = false;

  eastl::array<const char *, 3> waitTokenNames = {};
  const char *doneTokenName = nullptr;
  // Can be used to enforce strict ordering.
  CameraViewOrdering cameraViewOrdering = CameraViewOrdering::None;
  const char *mainViewNsName = "view0";
};

struct GtaoRenderParameters
{
  TargetDesc target;
  ResourceNames names;
  // See SsaoRenderParameters::materialGbufferBeforeRootPublish.
  bool materialGbufferBeforeRootPublish = false;

  bool isMainView = true;
  bool setViewFromMultiplexing = false;

  eastl::array<const char *, 3> waitTokenNames = {};
  const char *doneTokenName = nullptr;
};

struct SsaoFilterParameters
{
  TargetDesc target;
  ResourceNames names;
};

struct GtaoFilterParameters
{
  TargetDesc target;
  ResourceNames names;
  // See SsaoRenderParameters::materialGbufferBeforeRootPublish.
  bool materialGbufferBeforeRootPublish = false;
};

// Makes node that creates ssao_tex.
dafg::NodeHandle make_ssao_prepare_node(const dafg::NameSpace &ns, const SsaoPrepareParameters &params);
// Makes node that creates ssao_tex.
dafg::NodeHandle make_gtao_prepare_node(const dafg::NameSpace &ns, const GtaoPrepareParameters &params);

dafg::NodeHandle make_ssao_filter_node(const dafg::NameSpace &ns, const SsaoFilterParameters &params);
dafg::NodeHandle make_gtao_filter_node(const dafg::NameSpace &ns, const GtaoFilterParameters &params);

namespace detail
{
struct SsaoNodeState
{
  ssao_pass::RenderResources resources;
  ViewDependentResource<ao_common::FrameState, 2> frameState;
};

struct GtaoNodeState
{
  gtao_pass::RenderResources resources;
  ViewDependentResource<ao_common::FrameState, 2> frameState;
};

struct SsaoRenderState
{
  eastl::optional<dafg::VirtualResourceHandle<const SubFrameSample, false, true>> subFrameSample;
  dafg::VirtualResourceHandle<SsaoNodeState *const, false, false> nodeState;
  bool setViewFromMultiplexing = false;
  bool isMainView = true;
};

using CameraHandle = dafg::VirtualResourceHandle<const CameraParams, false, false>;
CameraHandle read_current_camera(dafg::Registry registry);
CameraHandle read_prev_camera(dafg::Registry registry);

eastl::unique_ptr<const SsaoRenderState> declare_ssao_render(dafg::Registry registry, const SsaoRenderParameters &params);
void render_ssao(const SsaoRenderState &state, dafg::multiplexing::Index multiplexing_index, const CameraParams &view);

struct GtaoRenderState
{
  eastl::optional<dafg::VirtualResourceHandle<const SubFrameSample, false, true>> subFrameSample;
  // Only set for the compute path.
  eastl::optional<dafg::VirtualResourceHandle<BaseTexture, true, false>> aoTex;
  dafg::VirtualResourceHandle<GtaoNodeState *const, false, false> nodeState;
  dafg::AutoResolutionRequest<2> resolution;
  bool setViewFromMultiplexing = false;
  bool isMainView = true;
};

eastl::unique_ptr<const GtaoRenderState> declare_gtao_render(dafg::Registry registry, const GtaoRenderParameters &params);
void render_gtao(const GtaoRenderState &state, dafg::multiplexing::Index multiplexing_index, const CameraParams &view);

} // namespace detail

// Token is the ordering token blob type the project uses. Context opens
// a scope around the render call, constructed from (current CameraParams, prev CameraParams, isMainView).
template <class Token, class Context>
dafg::NodeHandle make_ssao_render_node(const dafg::NameSpace &ns, const SsaoRenderParameters &params)
{
  return ns.registerNode("ssao_render", DAFG_PP_NODE_SRC,
    [params = eastl::make_unique<const SsaoRenderParameters>(params)](dafg::Registry registry) {
      auto state = detail::declare_ssao_render(registry, *params);
      if (params->doneTokenName)
        registry.createBlob<Token>(params->doneTokenName);
      if (params->cameraViewOrdering == CameraViewOrdering::MainView)
        registry.createBlob<Token>(camera_view_strict_ordering_token);
      else if (params->cameraViewOrdering == CameraViewOrdering::SecondaryView)
        (registry.getParentNameSpace() / params->mainViewNsName).readBlob(camera_view_strict_ordering_token);
      auto cameraHndl = detail::read_current_camera(registry);
      auto prevCameraHndl = detail::read_prev_camera(registry);

      return [state = eastl::move(state), cameraHndl, prevCameraHndl, isMainView = params->isMainView](
               dafg::multiplexing::Index multiplexing_index) {
        const CameraParams &camera = cameraHndl.ref();
        Context context{camera, prevCameraHndl.ref(), isMainView};
        detail::render_ssao(*state, multiplexing_index, camera);
      };
    });
}

// Token is the ordering token blob type the project uses. Context opens
// a scope around the render call, constructed from (current CameraParams, prev CameraParams, isMainView).
template <class Token, class Context>
dafg::NodeHandle make_gtao_render_node(const dafg::NameSpace &ns, const GtaoRenderParameters &params)
{
  return ns.registerNode("gtao_render", DAFG_PP_NODE_SRC,
    [params = eastl::make_unique<const GtaoRenderParameters>(params)](dafg::Registry registry) {
      auto state = detail::declare_gtao_render(registry, *params);
      if (params->doneTokenName)
        registry.createBlob<Token>(params->doneTokenName);
      auto cameraHndl = detail::read_current_camera(registry);
      auto prevCameraHndl = detail::read_prev_camera(registry);

      return [state = eastl::move(state), cameraHndl, prevCameraHndl, isMainView = params->isMainView](
               dafg::multiplexing::Index multiplexing_index) {
        const CameraParams &camera = cameraHndl.ref();
        Context context{camera, prevCameraHndl.ref(), isMainView};
        detail::render_gtao(*state, multiplexing_index, camera);
      };
    });
}

} // namespace ambient_occlusion
