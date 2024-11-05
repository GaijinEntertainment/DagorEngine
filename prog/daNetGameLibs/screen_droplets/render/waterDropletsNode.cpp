// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/screenDroplets.h>


dabfg::NodeHandle makeWaterDropletsNode()
{
  auto dropletMgr = get_screen_droplets_mgr();
  if (!dropletMgr)
    return {};

  dabfg::set_resolution("droplets", dropletMgr->getResolution());

  return dabfg::register_node("water_droplets_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("final_target_with_motion_blur").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    registry.requestState().setFrameBlock("global_frame");

    auto screenDropletRTHndl = registry
                                 .createTexture2d("screen_droplets_tex", dabfg::History::No,
                                   {TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("droplets"), 2})
                                 .atStage(dabfg::Stage::POST_RASTER)
                                 .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                 .handle();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("screen_droplets_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
    }

    return [opaqueFinalTargetHndl, screenDropletRTHndl]() {
      get_screen_droplets_mgr()->render(screenDropletRTHndl.view(), opaqueFinalTargetHndl.d3dResId());
    };
  });
}
