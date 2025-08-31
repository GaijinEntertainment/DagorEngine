// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bloomCore.h"

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_renderStates.h>

#define GLOBAL_VARS_BLOOM VAR(bloom_upsample_mip_scale)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_BLOOM
#undef VAR

namespace bloom
{
static eastl::string bloomInputTexName = "downsampled_color";

void set_input_tex_name(const eastl::string &tex_name) { bloomInputTexName = tex_name; }

static eastl::string gen_downsample_source_tex_name(uint32_t mip_to_gen)
{
  if (mip_to_gen == 0)
    return bloomInputTexName;
  const uint32_t previous_mip = mip_to_gen - 1;
  return eastl::string("downsampled_bloom_mip_") + eastl::to_string(previous_mip);
}

static eastl::string gen_downsample_target_tex_name(uint32_t mip_to_gen)
{
  return eastl::string("raw_bloom_mip_") + eastl::to_string(mip_to_gen);
}

void regenerate_downsample_chain(dag::Vector<dafg::NodeHandle> &nodes, uint32_t mips_count, bool use_esram, RenderCallback cb)
{
  nodes.clear();

  const auto ns = dafg::root() / "bloom";
  nodes.push_back(ns.registerNode("setup_samplers", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("downsample_hq_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("downsample_lq_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      registry.create("blur_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("upsample_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
  }));

  for (uint32_t mip = 0; mip < mips_count; ++mip)
  {
    const eastl::string downsampleNodeName = eastl::string("bloom_downsample_") + eastl::to_string(mip);
    nodes.push_back(ns.registerNode(downsampleNodeName.c_str(), DAFG_PP_NODE_SRC, [mip, cb, use_esram](dafg::Registry registry) {
      const bool firstMip = mip == 0;
      const eastl::string sourceTexName = gen_downsample_source_tex_name(mip);
      const eastl::string targetTexName = gen_downsample_target_tex_name(mip);

      registry.read(sourceTexName.c_str()).texture().atStage(dafg::Stage::PS).bindToShaderVar("blur_src_tex");
      registry.create(targetTexName.c_str(), dafg::History::No)
        .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F | (use_esram ? TEXCF_ESRAM_ONLY : 0U),
          registry.getResolution<2>("post_fx", 0.25f / (1 << mip))});
      registry.requestRenderPass().color({targetTexName.c_str()});

      const char *shaderName = firstMip ? "bloom_downsample_hq" : "bloom_downsample_lq";
      const char *samplerName = firstMip ? "downsample_hq_sampler" : "downsample_lq_sampler";
      registry.read(samplerName).blob<d3d::SamplerHandle>().bindToShaderVar("blur_src_tex_samplerstate");

      return [renderer = PostFxRenderer(shaderName), cb] { cb([&renderer]() { renderer.render(); }); };
    }));
    for (uint32_t blur_axis = 0; blur_axis < 2; ++blur_axis)
    {
      const bool horizontal = blur_axis == 0;
      const char *blurNodeNameTemplate = horizontal ? "bloom_hor_blur_" : "bloom_ver_blur_";
      const eastl::string blurNodeName = eastl::string(blurNodeNameTemplate) + eastl::to_string(mip);
      nodes.push_back(
        ns.registerNode(blurNodeName.c_str(), DAFG_PP_NODE_SRC, [mip, mips_count, horizontal, cb, use_esram](dafg::Registry registry) {
          const char *startName = "raw_bloom_mip_";
          const char *intermediateName = "hor_blured_bloom_mip_";
          const char *finishName = mip + 1 == mips_count ? "upsampled_bloom_mip_" : "downsampled_bloom_mip_";
          const eastl::string sourceTexName = eastl::string(horizontal ? startName : intermediateName) + eastl::to_string(mip);
          const eastl::string targetTexName = eastl::string(horizontal ? intermediateName : finishName) + eastl::to_string(mip);

          registry.read(sourceTexName.c_str()).texture().atStage(dafg::Stage::PS).bindToShaderVar("blur_src_tex");
          registry.read("blur_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("blur_src_tex_samplerstate");
          registry.create(targetTexName.c_str(), dafg::History::No)
            .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F | (use_esram ? TEXCF_ESRAM_ONLY : 0U),
              registry.getResolution<2>("post_fx", 0.25f / (1 << mip))});
          registry.requestRenderPass().color({targetTexName.c_str()});

          return [renderer = PostFxRenderer(horizontal ? "bloom_horizontal_blur_4" : "bloom_vertical_blur_4"), cb] {
            cb([&renderer]() { renderer.render(); });
          };
        }));
    }
  }
}

void regenerate_upsample_chain(dag::Vector<dafg::NodeHandle> &nodes, uint32_t mips_count, float upsample_factor,
  const Color3 &halation_color, float halation_mip_factor, int halation_end_mip_ofs, RenderCallback cb)
{
  nodes.clear();

  const auto ns = dafg::root() / "bloom";

  for (uint32_t mip = mips_count - 1; mip > 0; --mip)
  {
    const eastl::string upsampleNodeName = eastl::string("bloom_upsample_") + eastl::to_string(mip);
    nodes.push_back(ns.registerNode(upsampleNodeName.c_str(), DAFG_PP_NODE_SRC,
      [mip, mips_count, upsample_factor, halation_color, halation_mip_factor, halation_end_mip_ofs, cb](dafg::Registry registry) {
        const eastl::string sourceTexName = eastl::string("upsampled_bloom_mip_") + eastl::to_string(mip);
        const eastl::string targetTexOriginName = eastl::string("downsampled_bloom_mip_") + eastl::to_string(mip - 1);
        const eastl::string targetTexFinalName = eastl::string("upsampled_bloom_mip_") + eastl::to_string(mip - 1);

        registry.read(sourceTexName.c_str()).texture().atStage(dafg::Stage::PS).bindToShaderVar("blur_src_tex");
        registry.read("upsample_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("blur_src_tex_samplerstate");

        registry.requestRenderPass().color(
          {registry.rename(targetTexOriginName.c_str(), targetTexFinalName.c_str(), dafg::History::No).texture()});

        return [mip, mips_count, upsample_factor, halation_color, halation_mip_factor, halation_end_mip_ofs,
                 renderer = PostFxRenderer("bloom_upsample"), cb] {
          const int lastMipWithHalation = clamp<int>(mips_count - halation_end_mip_ofs, 0, mips_count);
          const float dstMipAddTint = mip - 1 >= lastMipWithHalation ? 0 : exp2f(-halation_mip_factor * (mip - 1));
          Color3 dstColorTint = (Color3(1, 1, 1) + halation_color * dstMipAddTint) * (1 - upsample_factor);
          ShaderGlobal::set_color4(bloom_upsample_mip_scaleVarId, upsample_factor, upsample_factor, upsample_factor, 0);
          d3d::set_blend_factor(e3dcolor(dstColorTint, 255));

          cb([&renderer]() { renderer.render(); });
        };
      }));
  }
}

uint32_t calculate_bloom_mips(uint32_t width, uint32_t height, int max_mips)
{
  const int GAUSS_HALF_KERNEL = 7; // we perform 15x15 gauss on higher mips
  const uint32_t mipsCount = clamp((int)get_log2i(min(width / 4, height / 4) / GAUSS_HALF_KERNEL), 1, max_mips);
  return mipsCount;
}
} // namespace bloom