// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>

#include <render/downsampleDepth.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>

#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>

static bool use_explicit_depth_write()
{
  // format conversion via copy results in invalid htile texture on xbox (even after resummarize)
  // no depth format conversion via blit/copy on vulkan
  return d3d::get_driver_code().is(d3d::xboxOne || d3d::scarlett || d3d::vulkan || d3d::metal);
}

eastl::array<dabfg::NodeHandle, 3> makeDownsampleDepthNodes(const DownsampleNodeParams &params)
{
  eastl::array<dabfg::NodeHandle, 3> nodes;
  nodes[0] = dabfg::register_node("downsample_depth_node", DABFG_PP_NODE_SRC, [params](dabfg::Registry registry) {
    registry.read("gbuf_0").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("albedo_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("albedo_gbuf_samplerstate");
    registry.read("gbuf_2").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("material_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate");

    auto gbufDepthHndl =
      registry.read("gbuf_depth").texture().atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

    const auto halfMainViewResolution = registry.getResolution<2>("main_view", 0.5f);

    auto checkerboardDepthOptHndl = params.needCheckerboard
                                      ? eastl::make_optional(registry.create("checkerboard_depth", dabfg::History::No)
                                                               .texture({TEXFMT_R32F | TEXCF_RTARGET, halfMainViewResolution})
                                                               .atStage(dabfg::Stage::PS_OR_CS)
                                                               .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                                               .handle())
                                      : eastl::nullopt;

    if (params.needCheckerboard)
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      registry.create("checkerboard_depth_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }

    auto closeDepthHndl =
      registry.create("close_depth", dabfg::History::DiscardOnFirstFrame)
        .texture({TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED, halfMainViewResolution, params.downsampledTexturesMipCount})
        .atStage(dabfg::Stage::PS_OR_CS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      registry.create("close_depth_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }

    const uint32_t esramFlags = params.storeDownsampledTexturesInEsram ? TEXCF_ESRAM_ONLY : 0;
    // this history is only used in skies and in volfog. Check if really needed!
    auto farDownsampledDepthHndl = registry.create("far_downsampled_depth", dabfg::History::DiscardOnFirstFrame)
                                     .texture({TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED | esramFlags, halfMainViewResolution,
                                       params.downsampledTexturesMipCount})
                                     .atStage(dabfg::Stage::PS_OR_CS)
                                     .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                     .handle();

    registry.readTextureHistory("close_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("prev_downsampled_close_depth_tex");

    // history is only needed for SSR/GI above onlyAO
    auto downsampledNormalsOptHndl =
      params.needNormals ? eastl::make_optional(registry.create("downsampled_normals", dabfg::History::DiscardOnFirstFrame)
                                                  .texture({TEXCF_RTARGET | esramFlags, halfMainViewResolution})
                                                  .atStage(dabfg::Stage::PS_OR_CS)
                                                  .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                                  .handle())
                         : eastl::nullopt;

    if (params.needNormals)
    {
      registry.readTextureHistory("downsampled_normals").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("prev_downsampled_normals");

      registry.read("gbuf_1").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("normal_gbuf");
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("downsampled_normals_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }

    auto downsampledMotionVectorsOptHndl =
      params.needMotionVectors
        ? eastl::make_optional(registry.create("downsampled_motion_vectors_tex", dabfg::History::DiscardOnFirstFrame)
                                 .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, halfMainViewResolution})
                                 .atStage(dabfg::Stage::PS_OR_CS)
                                 .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                 .handle())
        : eastl::nullopt;
    if (params.needMotionVectors)
    {
      registry.read("motion_vecs").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf");
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate");
      registry.readTextureHistory("downsampled_motion_vectors_tex")
        .atStage(dabfg::Stage::PS_OR_CS)
        .bindToShaderVar("prev_downsampled_motion_vectors_tex");
      registry.create("downsampled_motion_vectors_tex_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler({}))
        .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate");
    }

    const auto mainViewResolution = registry.getResolution<2>("main_view");

    // TODO: this hack should be removed as soon as we refactor samplers
    // to be separate from texture objects (which we should have done years ago)
    struct Handles
    {
      decltype(checkerboardDepthOptHndl) checkerboardDepthOptHandle;
      decltype(mainViewResolution) mainViewRes;
    };

    return [gbufDepthHndl, closeDepthHndl, farDownsampledDepthHndl, downsampledNormalsOptHndl, downsampledMotionVectorsOptHndl,
             hndls = eastl::unique_ptr<Handles>(new Handles{checkerboardDepthOptHndl, mainViewResolution})] {
      ManagedTexView checkerboardDepth =
        hndls->checkerboardDepthOptHandle.has_value() ? hndls->checkerboardDepthOptHandle.value().view() : ManagedTexView{};

      if (checkerboardDepth)
      {
        checkerboardDepth->disableSampler();
      }

      ManagedTexView closeDepth = closeDepthHndl.view();
      closeDepth->disableSampler();

      ManagedTexView farDownsampledDepth = farDownsampledDepthHndl.view();
      farDownsampledDepth->disableSampler();

      ManagedTexView downsampledNormals =
        downsampledNormalsOptHndl.has_value() ? downsampledNormalsOptHndl.value().view() : ManagedTexView{};

      if (downsampledNormals)
        downsampledNormals->disableSampler();

      auto [renderingWidth, renderingHeight] = hndls->mainViewRes.get();

      downsample_depth::downsample(gbufDepthHndl.view(), renderingWidth, renderingHeight, farDownsampledDepth, closeDepth,
        downsampledNormals,
        downsampledMotionVectorsOptHndl.has_value() ? downsampledMotionVectorsOptHndl.value().view() : ManagedTexView{},
        checkerboardDepth);
    };
  });
  if (use_explicit_depth_write())
    nodes[1] = dabfg::register_node("copy_depth", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      registry.requestRenderPass().depthRw(registry.create("downsampled_depth", dabfg::History::No)
                                             .texture({TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)}));
      auto farDownsampledDepthHndl =
        registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
      return [farDownsampledDepthHndl, renderer = PostFxRenderer("copy_depth")] {
        d3d::settex(15, farDownsampledDepthHndl.view().getTex2D());
        renderer.render();
      };
    });
  else
    nodes[1] = dabfg::register_node("copy_depth", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      auto downsampledDepthHndl = registry.create("downsampled_depth", dabfg::History::No)
                                    .texture({TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)})
                                    .atStage(dabfg::Stage::TRANSFER)
                                    .useAs(dabfg::Usage::COPY)
                                    .handle();
      auto farDownsampledDepthHndl =
        registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();
      return [downsampledDepthHndl, farDownsampledDepthHndl] {
        TextureInfo texinfo;
        ManagedTexView downsampledDepth = downsampledDepthHndl.view();
        downsampledDepth.getTex2D()->getinfo(texinfo, 0);
        downsampledDepth.getTex2D()->updateSubRegion(farDownsampledDepthHndl.view().getTex2D(), 0, 0, 0, 0, texinfo.w, texinfo.h, 1, 0,
          0, 0, 0);
        d3d::resummarize_htile(downsampledDepth.getTex2D());
      };
    });
  nodes[2] = dabfg::register_node("depth_samplers_init", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    registry.create("far_downsampled_depth_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
  });
  return nodes;
}
