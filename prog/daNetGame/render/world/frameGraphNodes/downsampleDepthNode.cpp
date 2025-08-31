// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include <render/downsampleDepth.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>

#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>

static bool use_explicit_depth_write(const bool has_stencil)
{
  // format conversion via copy results in invalid htile texture on xbox (even after resummarize)
  // no depth format conversion via blit/copy on vulkan
  return has_stencil || d3d::get_driver_code().is(d3d::xboxOne || d3d::scarlett || d3d::vulkan || d3d::metal);
}

eastl::array<dafg::NodeHandle, 3> makeDownsampleDepthNodes(const DownsampleNodeParams &params)
{
  eastl::array<dafg::NodeHandle, 3> nodes;
  nodes[0] = dafg::register_node("downsample_depth_node", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    registry.read("gbuf_0").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("albedo_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("albedo_gbuf_samplerstate");
    registry.read("gbuf_2").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("material_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate");

    auto gbufDepthHndl =
      registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

    const auto halfMainViewResolution = registry.getResolution<2>("main_view", 0.5f);

    auto checkerboardDepthOptHndl = params.needCheckerboard
                                      ? eastl::make_optional(registry.create("checkerboard_depth", dafg::History::No)
                                                               .texture({TEXFMT_R32F | TEXCF_RTARGET, halfMainViewResolution})
                                                               .atStage(dafg::Stage::PS_OR_CS)
                                                               .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                                               .handle())
                                      : eastl::nullopt;

    if (params.needCheckerboard)
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      registry.create("checkerboard_depth_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }

    auto closeDepthHndl =
      registry.create("close_depth", dafg::History::DiscardOnFirstFrame)
        .texture({TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED, halfMainViewResolution, params.downsampledTexturesMipCount})
        .atStage(dafg::Stage::PS_OR_CS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT)
        .handle();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      registry.create("close_depth_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }

    const uint32_t esramFlags = params.storeDownsampledTexturesInEsram ? TEXCF_ESRAM_ONLY : 0;
    // this history is only used in skies and in volfog. Check if really needed!
    auto farDownsampledDepthHndl = registry.create("far_downsampled_depth", dafg::History::DiscardOnFirstFrame)
                                     .texture({TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED | esramFlags, halfMainViewResolution,
                                       params.downsampledTexturesMipCount})
                                     .atStage(dafg::Stage::PS_OR_CS)
                                     .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                     .handle();

    registry.readTextureHistory("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("prev_downsampled_close_depth_tex");

    // history is only needed for SSR/GI above onlyAO
    auto downsampledNormalsOptHndl =
      params.needNormals ? eastl::make_optional(registry.create("downsampled_normals", dafg::History::DiscardOnFirstFrame)
                                                  .texture({TEXCF_RTARGET | esramFlags, halfMainViewResolution})
                                                  .atStage(dafg::Stage::PS_OR_CS)
                                                  .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                                  .handle())
                         : eastl::nullopt;

    if (params.needNormals)
    {
      registry.readTextureHistory("downsampled_normals").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("prev_downsampled_normals");

      registry.read("gbuf_1").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("normal_gbuf");
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("downsampled_normals_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }

    auto downsampledMotionVectorsOptHndl =
      params.needMotionVectors
        ? eastl::make_optional(registry.create("downsampled_motion_vectors_tex", dafg::History::DiscardOnFirstFrame)
                                 .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, halfMainViewResolution})
                                 .atStage(dafg::Stage::PS_OR_CS)
                                 .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                 .handle())
        : eastl::nullopt;
    if (params.needMotionVectors)
    {
      registry.read("motion_vecs").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf");
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate");
      registry.readTextureHistory("downsampled_motion_vectors_tex")
        .atStage(dafg::Stage::PS_OR_CS)
        .bindToShaderVar("prev_downsampled_motion_vectors_tex");
      registry.create("downsampled_motion_vectors_tex_sampler", dafg::History::No)
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
      ManagedTexView closeDepth = closeDepthHndl.view();

      ManagedTexView farDownsampledDepth = farDownsampledDepthHndl.view();

      ManagedTexView downsampledNormals =
        downsampledNormalsOptHndl.has_value() ? downsampledNormalsOptHndl.value().view() : ManagedTexView{};

      auto [renderingWidth, renderingHeight] = hndls->mainViewRes.get();

      downsample_depth::downsample(gbufDepthHndl.view(), renderingWidth, renderingHeight, farDownsampledDepth, closeDepth,
        downsampledNormals,
        downsampledMotionVectorsOptHndl.has_value() ? downsampledMotionVectorsOptHndl.value().view() : ManagedTexView{},
        checkerboardDepth);
    };
  });

  const bool hasStencil = renderer_has_feature(CAMERA_IN_CAMERA);
  const uint32_t downsampledDepthFmt = hasStencil ? TEXFMT_DEPTH32_S8 : TEXFMT_DEPTH32;

  if (use_explicit_depth_write(hasStencil))
    nodes[1] = dafg::register_node("copy_depth", DAFG_PP_NODE_SRC, [downsampledDepthFmt, hasStencil](dafg::Registry registry) {
      const char *depthName = hasStencil ? "downsampled_depth_no_stencil" : "downsampled_depth";
      registry.requestRenderPass().depthRw(
        registry.create(depthName, dafg::History::No)
          .texture({downsampledDepthFmt | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)}));
      auto farDownsampledDepthHndl =
        registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      return [farDownsampledDepthHndl, renderer = PostFxRenderer("copy_depth")] {
        d3d::settex(15, farDownsampledDepthHndl.view().getTex2D());
        d3d::set_sampler(STAGE_PS, 15, d3d::request_sampler({}));
        renderer.render();
      };
    });
  else
    nodes[1] = dafg::register_node("copy_depth", DAFG_PP_NODE_SRC, [downsampledDepthFmt](dafg::Registry registry) {
      auto downsampledDepthHndl = registry.create("downsampled_depth", dafg::History::No)
                                    .texture({downsampledDepthFmt | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)})
                                    .atStage(dafg::Stage::TRANSFER)
                                    .useAs(dafg::Usage::COPY)
                                    .handle();
      auto farDownsampledDepthHndl =
        registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();
      return [downsampledDepthHndl, farDownsampledDepthHndl] {
        TextureInfo texinfo;
        ManagedTexView downsampledDepth = downsampledDepthHndl.view();
        downsampledDepth.getTex2D()->getinfo(texinfo, 0);
        downsampledDepth.getTex2D()->updateSubRegion(farDownsampledDepthHndl.view().getTex2D(), 0, 0, 0, 0, texinfo.w, texinfo.h, 1, 0,
          0, 0, 0);
        d3d::resummarize_htile(downsampledDepth.getTex2D());
      };
    });
  nodes[2] = dafg::register_node("depth_samplers_init", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    registry.create("far_downsampled_depth_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
  });
  return nodes;
}
