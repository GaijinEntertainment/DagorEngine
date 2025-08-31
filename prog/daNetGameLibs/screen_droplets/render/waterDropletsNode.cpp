// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/screenDroplets.h>


dafg::NodeHandle makeWaterDropletsNode()
{
  auto dropletMgr = get_screen_droplets_mgr();
  if (!dropletMgr)
    return {};

  dafg::set_resolution("droplets", dropletMgr->getResolution());

  return dafg::register_node("water_droplets_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("final_target_with_motion_blur").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.requestState().setFrameBlock("global_frame");

    auto screenDropletRTHndl = registry
                                 .createTexture2d("screen_droplets_tex", dafg::History::No,
                                   {TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("droplets"), 2})
                                 .atStage(dafg::Stage::POST_RASTER)
                                 .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                 .handle();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("screen_droplets_sampler", dafg::History::No).blob(d3d::request_sampler(smpInfo));
    }

    return [opaqueFinalTargetHndl, screenDropletRTHndl]() {
      get_screen_droplets_mgr()->render(screenDropletRTHndl.view(), opaqueFinalTargetHndl.d3dResId());
    };
  });
}
