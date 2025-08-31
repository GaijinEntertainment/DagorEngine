// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/hdrRender.h>
#include <shaders/dag_computeShaders.h>
#include <render/daFrameGraph/daFG.h>

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

eastl::fixed_vector<dafg::NodeHandle, 3> makeFsrNodes()
{
  eastl::fixed_vector<dafg::NodeHandle, 3> result;

  result.push_back(dafg::register_node("fsr_upscale_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto sourceHndl = registry.readTexture("postfxed_frame").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto displayRes = registry.getResolution<2>("display");

    auto targetHndl = registry.create("fsr_upscaled_frame", dafg::History::No)
                        .texture(dafg::Texture2dCreateInfo{determine_format() | TEXCF_UNORDERED, displayRes, 1})
                        .atStage(dafg::Stage::CS)
                        .useAs(dafg::Usage::SHADER_RESOURCE)
                        .handle();
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Linear;

    return [sourceHndl, targetHndl, displayRes, fsrUpsample = ComputeShader("fsr_upsampling"), smp = d3d::request_sampler(smpInfo)] {
      // TODO: move FSR to ECS and store persistent cbufs there
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      d3d::set_rwtex(STAGE_CS, 0, targetHndl.get(), 0, 0);
      d3d::set_tex(STAGE_CS, 0, sourceHndl.view().getBaseTex());
      d3d::set_sampler(STAGE_CS, 0, smp);
      d3d::set_const_buffer(STAGE_CS, 1, wr.superResolutionUpscalingConsts.getBuf());

      auto [displayWidth, displayHeight] = displayRes.get();
      fsrUpsample.dispatchGroups((displayWidth + FSR_TILE_SIZE - 1) / FSR_TILE_SIZE,
        (displayHeight + FSR_TILE_SIZE - 1) / FSR_TILE_SIZE, 1);

      d3d::set_const_buffer(STAGE_CS, 1, nullptr);
      d3d::set_tex(STAGE_CS, 0, nullptr);
      d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    };
  }));

  result.push_back(dafg::register_node("fsr_sharpen_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto sourceHndl =
      registry.read("fsr_upscaled_frame").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto targetHndl =
      hdrrender::is_hdr_enabled()
        ? registry.modify("frame_after_postfx").texture().atStage(dafg::Stage::UNKNOWN).useAs(dafg::Usage::UNKNOWN).handle()
        : registry.create("fsr_sharpened_frame", dafg::History::No)
            .texture(dafg::Texture2dCreateInfo{determine_format() | TEXCF_UNORDERED, registry.getResolution<2>("display"), 1})
            .atStage(dafg::Stage::CS)
            .useAs(dafg::Usage::SHADER_RESOURCE)
            .handle();

    auto displayResHdnl = registry.getResolution<2>("display");
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Linear;

    return
      [sourceHndl, targetHndl, displayResHdnl, fsrSharpen = ComputeShader("fsr_sharpening"), smp = d3d::request_sampler(smpInfo)]() {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        d3d::set_rwtex(STAGE_CS, 0, targetHndl.get(), 0, 0);
        d3d::set_tex(STAGE_CS, 0, sourceHndl.view().getTex2D());
        d3d::set_sampler(STAGE_CS, 0, smp);
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
    result.push_back(dafg::register_node("fsr_blit_result_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto sourceHndl =
        registry.read("fsr_sharpened_frame").texture().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::BLIT).handle();

      auto targetHndl =
        registry.modify("frame_after_postfx").texture().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::BLIT).handle();

      return [sourceHndl, targetHndl] { d3d::stretch_rect(sourceHndl.view().getTex2D(), targetHndl.get()); };
    }));

  return result;
}
