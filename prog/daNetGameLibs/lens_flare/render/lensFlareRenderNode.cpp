// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareNodes.h"

#include <render/lensFlare/render/lensFlareRenderer.h>
#include <render/daFrameGraph/daFG.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraInCamera.h>
#include <EASTL/shared_ptr.h>

dafg::NodeHandle create_lens_flare_per_camera_res_node(const LensFlareQualityParameters &quality)
{
  return get_lens_flare_namespace().registerNode("per_camera_res", DAFG_PP_NODE_SRC, [quality](dafg::Registry registry) {
    const auto resolution = registry.getResolution<2>("post_fx", quality.resolutionScaling);
    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
      rtFmt = TEXFMT_A16B16G16R16F;
    const uint32_t texFmt = rtFmt | TEXCF_RTARGET;
    registry.createTexture2d("lens_flares", {texFmt, resolution}).clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

    auto hasLensFlaresHndl = registry.createBlob<int>("has_lens_flares").handle();
    auto hasLensFlaresInViewHndl = registry.createBlob<int>("has_lens_flares_view_0").handle();

    auto mainViewOnlyHndl = registry.createBlob<bool>("main_view_only").handle();

    return [mainViewOnlyHndl, hasLensFlaresHndl, hasLensFlaresInViewHndl]() {
      mainViewOnlyHndl.ref() = !camera_in_camera::is_lens_render_active();
      hasLensFlaresHndl.ref() = 0;
      hasLensFlaresInViewHndl.ref() = 0;
    };
  });
}

dafg::NodeHandle create_lens_flare_render_node(
  const LensFlareRenderer *renderer, const LensFlareQualityParameters &quality, const int view_id)
{
  char name[] = "render#";
  name[eastl::size(name) - 2] = '0' + view_id;

  return get_lens_flare_namespace().registerNode(name, DAFG_PP_NODE_SRC, [renderer, quality, view_id](dafg::Registry registry) {
    const auto resolution = registry.getResolution<2>("post_fx", quality.resolutionScaling);

    const bool isMainView = view_id == 0 || !renderer_has_feature(CAMERA_IN_CAMERA);
    auto hasLensFlaresHndl = registry.modifyBlob<int>("has_lens_flares").handle();
    auto hasLensFlaresPerViewHndl = registry.readBlob<int>(isMainView ? "has_lens_flares_view_0" : "has_lens_flares_view_1").handle();

    registry.requestRenderPass().color({"lens_flares"});
    registry.requestState().setFrameBlock("global_frame");

    return [renderer, resolution, hasLensFlaresHndl, hasLensFlaresPerViewHndl]() {
      if (!hasLensFlaresPerViewHndl.ref())
        return;

      hasLensFlaresHndl.ref() |= hasLensFlaresPerViewHndl.ref();

      renderer->render(resolution.get());
    };
  });
}
