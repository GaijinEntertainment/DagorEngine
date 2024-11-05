// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/hdrRender.h>
#include <shaders/dag_postFxRenderer.h>

#include "frameGraphNodes.h"
#include "main/watchdog.h"
#include "render/renderer.h"
#include <drv/3d/dag_texture.h>

static shaders::OverrideStateId gen_add_blend_override()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  return shaders::overrides::create(state);
}

eastl::fixed_vector<dabfg::NodeHandle, 5, false> makeSubsamplingNodes(bool sub_sampling, bool super_sampling)
{
  const char *inputTextureName = "frame_with_debug";
  const char *subsampledTextureName = "subsampled_frame";
  const char *supersampledTextureName = "supersampled_frame";
  const char *finalTextureName = "frame_to_present";
  eastl::fixed_vector<dabfg::NodeHandle, 5, false> nodes;
  if (!sub_sampling && !super_sampling)
  {
    nodes.emplace_back(dabfg::register_node("finish_frame_with_debug", DABFG_PP_NODE_SRC,
      [inputTextureName, finalTextureName](dabfg::Registry registry) {
        registry.renameTexture(inputTextureName, finalTextureName, dabfg::History::No);
        registry.multiplex(dabfg::multiplexing::Mode::Viewport);
      }));
    return nodes;
  }
  if (sub_sampling)
  {
    nodes.emplace_back(
      dabfg::register_node("init_subsampling_frame", DABFG_PP_NODE_SRC, [subsampledTextureName](dabfg::Registry registry) {
        registry
          .createTexture2d(subsampledTextureName, dabfg::History::No,
            {(uint32_t)(TEXCF_RTARGET | (hdrrender::is_hdr_enabled() ? TEXFMT_A16B16G16R16F : TEXFMT_A16B16G16R16)),
              registry.getResolution<2>("display")})
          .atStage(dabfg::Stage::PS) // We set stage and usage here, because framegraph requires it.
          .useAs(dabfg::Usage::COLOR_ATTACHMENT);
        registry.multiplex(dabfg::multiplexing::Mode::Viewport | dabfg::multiplexing::Mode::SuperSampling);
        registry.orderMeBefore("multisampling_setup_node");
        return [force_ignore_historyVarId = get_shader_variable_id("force_ignore_history", true)] {
          ShaderGlobal::set_int(force_ignore_historyVarId, 1);
        };
      }));
    nodes.emplace_back(dabfg::register_node("fill_subsampling_frame", DABFG_PP_NODE_SRC,
      [inputTextureName, subsampledTextureName](dabfg::Registry registry) {
        registry.requestRenderPass().color({subsampledTextureName});
        auto superSubPixelsHndl = registry.readBlob<IPoint2>("super_sub_pixels").handle();
        auto frameHndl =
          registry.readTexture(inputTextureName).atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
        registry.readTexture("depth_for_postfx").atStage(dabfg::Stage::PS).bindToShaderVar("depth_gbuf");
        return [superSubPixelsHndl, frameHndl, sub_pixelsVarId = ::get_shader_variable_id("sub_pixels", true),
                 additiveBlendStateId = gen_add_blend_override(),
                 screenshot_composer = PostFxRenderer("screenshot_composer")](dabfg::multiplexing::Index multiplexing_index) {
          watchdog_kick();
          ShaderGlobal::set_int(sub_pixelsVarId, superSubPixelsHndl.ref().y);

          const bool firstSubPixel = multiplexing_index.subSample == 0;
          if (!firstSubPixel)
            shaders::overrides::set(additiveBlendStateId);
          d3d::settex(1, frameHndl.view().getBaseTex());
          screenshot_composer.render();
          if (!firstSubPixel)
            shaders::overrides::reset();
          ShaderElement::invalidate_cached_state_block();
        };
      }));
    if (!super_sampling)
    {
      nodes.emplace_back(dabfg::register_node("finish_subsampling_frame", DABFG_PP_NODE_SRC,
        [subsampledTextureName, finalTextureName](dabfg::Registry registry) {
          auto subsamplingFrameHndl =
            registry.readTexture(subsampledTextureName).atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();
          registry.multiplex(dabfg::multiplexing::Mode::Viewport | dabfg::multiplexing::Mode::SuperSampling);
          registry.modifyBlob<bool>("fake_blob");
          auto frameToPresentHndl =
            registry.modifyTexture(finalTextureName).atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();
          return [subsamplingFrameHndl, frameToPresentHndl,
                   force_ignore_historyVarId = get_shader_variable_id("force_ignore_history", true)] {
            d3d::stretch_rect(subsamplingFrameHndl.view().getTex2D(), frameToPresentHndl.view().getTex2D());
            ShaderGlobal::set_int(force_ignore_historyVarId, 0);
          };
        }));
    }
  }
  if (super_sampling)
  {
    nodes.emplace_back(
      dabfg::register_node("init_supersampling_frame", DABFG_PP_NODE_SRC, [supersampledTextureName](dabfg::Registry registry) {
        registry.multiplex(dabfg::multiplexing::Mode::Viewport);
        registry.registerTexture2d(supersampledTextureName,
          [](auto) -> ManagedTexView { return get_world_renderer()->getSuperResolutionScreenshot(); });
        return [] {};
      }));

    const char *texToSupersampleName = sub_sampling ? subsampledTextureName : inputTextureName;
    nodes.emplace_back(dabfg::register_node("supersampling_apply_node", DABFG_PP_NODE_SRC,
      [supersampledTextureName, texToSupersampleName](dabfg::Registry registry) {
        registry.multiplex(dabfg::multiplexing::Mode::Viewport | dabfg::multiplexing::Mode::SuperSampling);
        registry.requestRenderPass().color({supersampledTextureName});
        auto superSubPixelsHndl = registry.readBlob<IPoint2>("super_sub_pixels").handle();
        auto sourceFrameHndl =
          registry.readTexture(texToSupersampleName).atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Point;

        return [sourceFrameHndl, superSubPixelsHndl, super_pixelsVarId = ::get_shader_variable_id("super_pixels"),
                 interleave_pixelsVarId = ::get_shader_variable_id("interleave_pixels"),
                 interleaveSamples = PostFxRenderer("interleave_samples_renderer"),
                 smp = d3d::request_sampler(smpInfo)](dabfg::multiplexing::Index multiplexing_index) {
          watchdog_kick();
          auto [superPixels, subPixels] = superSubPixelsHndl.ref();
          auto superSample = multiplexing_index.superSample;
          const uint32_t superx = superSample / superPixels;
          const uint32_t supery = superSample % superPixels;

          ShaderGlobal::set_int(super_pixelsVarId, superPixels);

          ShaderGlobal::set_color4(interleave_pixelsVarId, superx, supery, superPixels, 0);
          Texture *tex = sourceFrameHndl.view().getTex2D();
          d3d::settex(0, tex);
          d3d::set_sampler(STAGE_PS, 0, smp);
          interleaveSamples.render();
          ShaderElement::invalidate_cached_state_block();
        };
      }));

    nodes.emplace_back(dabfg::register_node("downscale_supersampling", DABFG_PP_NODE_SRC,
      [supersampledTextureName, finalTextureName](dabfg::Registry registry) {
        registry.multiplex(dabfg::multiplexing::Mode::Viewport);
        registry.readTexture(supersampledTextureName).atStage(dabfg::Stage::PS).bindToShaderVar("super_screenshot_tex");
        registry.requestRenderPass().color({finalTextureName});
        return [force_ignore_historyVarId = get_shader_variable_id("force_ignore_history", true),
                 downscaleSuperresScreenshot = PostFxRenderer("downscale_superres_screenshot")] {
          downscaleSuperresScreenshot.render();
          ShaderGlobal::set_int(force_ignore_historyVarId, 0);
        };
      }));
  }
  return nodes;
}
