// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/hdrRender.h>
#include <shaders/dag_computeShaders.h>
#include <render/daBfg/bfg.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>


static unsigned determine_format()
{
  unsigned texFmt = TEXFMT_R8G8B8A8;
  if (hdrrender::is_hdr_enabled())
  {
    TextureInfo textureInfo;
    hdrrender::get_render_target_tex()->getinfo(textureInfo);
    texFmt = textureInfo.cflg & TEXFMT_MASK;
  }
  return texFmt;
}

// This makes literally 0 sense, as the actual numthreads is (64, 1, 1).
// But was copied over from AMD's sample code and it works, so don't
// touch it.
static constexpr int FSR_TILE_SIZE = 16;

eastl::fixed_vector<dabfg::NodeHandle, 3> makeFsrNodes()
{
  eastl::fixed_vector<dabfg::NodeHandle, 3> result;

  result.push_back(dabfg::register_node("fsr_upscale_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeBefore("after_post_fx_ecs_event_node");
    auto sourceHndl = registry.readTexture("postfxed_frame").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto displayRes = registry.getResolution<2>("display");

    auto targetHndl = registry.create("fsr_upscaled_frame", dabfg::History::No)
                        .texture(dabfg::Texture2dCreateInfo{determine_format() | TEXCF_UNORDERED, displayRes, 1})
                        .atStage(dabfg::Stage::CS)
                        .useAs(dabfg::Usage::SHADER_RESOURCE)
                        .handle();

    return [sourceHndl, targetHndl, displayRes, fsrUpsample = ComputeShader("fsr_upsampling")] {
      // TODO: move FSR to ECS and store persistent cbufs there
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      d3d::set_rwtex(STAGE_CS, 0, targetHndl.get(), 0, 0);
      d3d::set_tex(STAGE_CS, 0, sourceHndl.view().getBaseTex());
      d3d::set_const_buffer(STAGE_CS, 1, wr.superResolutionUpscalingConsts.getBuf());

      auto [displayWidth, displayHeight] = displayRes.get();
      fsrUpsample.dispatchGroups((displayWidth + FSR_TILE_SIZE - 1) / FSR_TILE_SIZE,
        (displayHeight + FSR_TILE_SIZE - 1) / FSR_TILE_SIZE, 1);

      d3d::set_const_buffer(STAGE_CS, 1, nullptr);
      d3d::set_tex(STAGE_CS, 0, nullptr);
      d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    };
  }));

  result.push_back(dabfg::register_node("fsr_sharpen_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeBefore("after_post_fx_ecs_event_node");
    auto sourceHndl =
      registry.read("fsr_upscaled_frame").texture().atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto targetHndl =
      hdrrender::is_hdr_enabled()
        ? registry.modify("frame_after_postfx").texture().atStage(dabfg::Stage::UNKNOWN).useAs(dabfg::Usage::UNKNOWN).handle()
        : registry.create("fsr_sharpened_frame", dabfg::History::No)
            .texture(dabfg::Texture2dCreateInfo{determine_format() | TEXCF_UNORDERED, registry.getResolution<2>("display"), 1})
            .atStage(dabfg::Stage::CS)
            .useAs(dabfg::Usage::SHADER_RESOURCE)
            .handle();

    auto displayResHdnl = registry.getResolution<2>("display");

    return [sourceHndl, targetHndl, displayResHdnl, fsrSharpen = ComputeShader("fsr_sharpening")] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      d3d::set_rwtex(STAGE_CS, 0, targetHndl.get(), 0, 0);
      d3d::set_tex(STAGE_CS, 0, sourceHndl.view().getTex2D());
      d3d::set_const_buffer(STAGE_CS, 1, wr.superResolutionSharpeningConsts.getBuf());

      auto [displayWidth, displayHeight] = displayResHdnl.get();
      fsrSharpen.dispatchGroups((displayWidth + FSR_TILE_SIZE - 1) / FSR_TILE_SIZE,
        (displayHeight + FSR_TILE_SIZE - 1) / FSR_TILE_SIZE, 1);

      d3d::set_const_buffer(STAGE_CS, 1, nullptr);
      d3d::set_tex(STAGE_CS, 0, nullptr);
      d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    };
  }));

  if (!hdrrender::is_hdr_enabled())
    result.push_back(dabfg::register_node("fsr_blit_result_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      registry.orderMeBefore("after_post_fx_ecs_event_node");
      auto sourceHndl =
        registry.read("fsr_sharpened_frame").texture().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::BLIT).handle();

      auto targetHndl =
        registry.modify("frame_after_postfx").texture().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::BLIT).handle();

      return [sourceHndl, targetHndl] { d3d::stretch_rect(sourceHndl.view().getTex2D(), targetHndl.get()); };
    }));

  return result;
}
