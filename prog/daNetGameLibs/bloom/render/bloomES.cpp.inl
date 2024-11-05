// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>

// for ECS events
#include "render/renderEvent.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>

#include <render/rendererFeatures.h>
#include <render/resolution.h>

#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_renderStates.h>
#include <util/dag_convar.h>

#define GLOBAL_VARS_BLOOM       \
  VAR(bloom_threshold)          \
  VAR(bloom_mul)                \
  VAR(bloom_upsample_mip_scale) \
  VAR(bloom_radius)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_BLOOM
#undef VAR

eastl::string gen_downsample_source_tex_name(uint32_t mip_to_gen)
{
  if (mip_to_gen == 0)
    return "downsampled_color";
  const uint32_t previous_mip = mip_to_gen - 1;
  return eastl::string("downsampled_bloom_mip_") + eastl::to_string(previous_mip);
}

eastl::string gen_downsample_target_tex_name(uint32_t mip_to_gen)
{
  return eastl::string("raw_bloom_mip_") + eastl::to_string(mip_to_gen);
}

static void regenerate_downsample_chain(dag::Vector<dabfg::NodeHandle> &nodes, uint32_t mips_count)
{
  nodes.clear();

  const auto ns = dabfg::root() / "bloom";
  nodes.push_back(ns.registerNode("setup_samplers", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("downsample_hq_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("downsample_lq_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      registry.create("blur_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("upsample_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
  }));

  for (uint32_t mip = 0; mip < mips_count; ++mip)
  {
    const eastl::string downsampleNodeName = eastl::string("bloom_downsample_") + eastl::to_string(mip);
    nodes.push_back(ns.registerNode(downsampleNodeName.c_str(), DABFG_PP_NODE_SRC, [mip](dabfg::Registry registry) {
      const bool firstMip = mip == 0;
      const eastl::string sourceTexName = gen_downsample_source_tex_name(mip);
      const eastl::string targetTexName = gen_downsample_target_tex_name(mip);

      registry.read(sourceTexName.c_str()).texture().atStage(dabfg::Stage::PS).bindToShaderVar("blur_src_tex");
      registry.create(targetTexName.c_str(), dabfg::History::No)
        .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_ESRAM_ONLY, registry.getResolution<2>("post_fx", 0.25f / (1 << mip))});
      registry.requestRenderPass().color({targetTexName.c_str()});

      const char *shaderName = firstMip ? "bloom_downsample_hq" : "bloom_downsample_lq";
      const char *samplerName = firstMip ? "downsample_hq_sampler" : "downsample_lq_sampler";
      registry.read(samplerName).blob<d3d::SamplerHandle>().bindToShaderVar("blur_src_tex_samplerstate");

      return [renderer = PostFxRenderer(shaderName)] { renderer.render(); };
    }));
    for (uint32_t blur_axis = 0; blur_axis < 2; ++blur_axis)
    {
      const bool horizontal = blur_axis == 0;
      const char *blurNodeNameTemplate = horizontal ? "bloom_hor_blur_" : "bloom_ver_blur_";
      const eastl::string blurNodeName = eastl::string(blurNodeNameTemplate) + eastl::to_string(mip);
      nodes.push_back(
        ns.registerNode(blurNodeName.c_str(), DABFG_PP_NODE_SRC, [mip, mips_count, horizontal](dabfg::Registry registry) {
          const char *startName = "raw_bloom_mip_";
          const char *intermediateName = "hor_blured_bloom_mip_";
          const char *finishName = mip + 1 == mips_count ? "upsampled_bloom_mip_" : "downsampled_bloom_mip_";
          const eastl::string sourceTexName = eastl::string(horizontal ? startName : intermediateName) + eastl::to_string(mip);
          const eastl::string targetTexName = eastl::string(horizontal ? intermediateName : finishName) + eastl::to_string(mip);

          registry.read(sourceTexName.c_str()).texture().atStage(dabfg::Stage::PS).bindToShaderVar("blur_src_tex");
          registry.read("blur_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("blur_src_tex_samplerstate");
          registry.create(targetTexName.c_str(), dabfg::History::No)
            .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_ESRAM_ONLY, registry.getResolution<2>("post_fx", 0.25f / (1 << mip))});
          registry.requestRenderPass().color({targetTexName.c_str()});

          return [renderer = PostFxRenderer(horizontal ? "bloom_horizontal_blur_4" : "bloom_vertical_blur_4")] { renderer.render(); };
        }));
    }
  }
}

static void regenerate_upsample_chain(dag::Vector<dabfg::NodeHandle> &nodes,
  uint32_t mips_count,
  float upsample_factor,
  const Color3 &halation_color,
  float halation_mip_factor,
  int halation_end_mip_ofs)
{
  nodes.clear();

  const auto ns = dabfg::root() / "bloom";

  for (uint32_t mip = mips_count - 1; mip > 0; --mip)
  {
    const eastl::string upsampleNodeName = eastl::string("bloom_upsample_") + eastl::to_string(mip);
    nodes.push_back(ns.registerNode(upsampleNodeName.c_str(), DABFG_PP_NODE_SRC,
      [mip, mips_count, upsample_factor, halation_color, halation_mip_factor, halation_end_mip_ofs](dabfg::Registry registry) {
        const eastl::string sourceTexName = eastl::string("upsampled_bloom_mip_") + eastl::to_string(mip);
        const eastl::string targetTexOriginName = eastl::string("downsampled_bloom_mip_") + eastl::to_string(mip - 1);
        const eastl::string targetTexFinalName = eastl::string("upsampled_bloom_mip_") + eastl::to_string(mip - 1);

        registry.read(sourceTexName.c_str()).texture().atStage(dabfg::Stage::PS).bindToShaderVar("blur_src_tex");
        registry.read("upsample_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("blur_src_tex_samplerstate");

        registry.requestRenderPass().color(
          {registry.rename(targetTexOriginName.c_str(), targetTexFinalName.c_str(), dabfg::History::No).texture()});

        return [mip, mips_count, upsample_factor, halation_color, halation_mip_factor, halation_end_mip_ofs,
                 renderer = PostFxRenderer("bloom_upsample")] {
          const int lastMipWithHalation = clamp<int>(mips_count - halation_end_mip_ofs, 0, mips_count);
          const float dstMipAddTint = mip - 1 >= lastMipWithHalation ? 0 : exp2f(-halation_mip_factor * (mip - 1));
          Color3 dstColorTint = (Color3(1, 1, 1) + halation_color * dstMipAddTint) * (1 - upsample_factor);
          ShaderGlobal::set_color4(bloom_upsample_mip_scaleVarId, upsample_factor, upsample_factor, upsample_factor, 0);
          d3d::set_blend_factor(e3dcolor(dstColorTint, 255));

          renderer.render();
        };
      }));
  }
}

static uint32_t calculate_bloom_mips(uint32_t width, uint32_t height)
{
  const int GAUSS_HALF_KERNEL = 7; // we perform 15x15 gauss on higher mips
  const int MAX_MIPS = 7;
  const uint32_t mipsCount = clamp((int)get_log2i(min(width / 4, height / 4) / GAUSS_HALF_KERNEL), 1, MAX_MIPS);
  return mipsCount;
}

template <typename Callable>
static void init_bloom_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
static void disable_bloom_ecs_query(ecs::EntityId, Callable);

ECS_TAG(render)
static void reset_bloom_es(const ChangeRenderFeatures &evt, ecs::EntityManager &manager)
{
  if (!evt.changedFeatureFlags.test(FeatureRenderFlags::BLOOM))
    return;
  if (!evt.newFeatureFlags.test(FeatureRenderFlags::BLOOM))
    disable_bloom_ecs_query(manager.getSingletonEntity(ECS_HASH("bloom")),
      [](dag::Vector<dabfg::NodeHandle> &bloom__downsample_chain, dag::Vector<dabfg::NodeHandle> &bloom__upsample_chain) {
        bloom__downsample_chain.clear();
        bloom__upsample_chain.clear();
      });
}

ECS_TAG(render)
static void resize_bloom_es(const SetResolutionEvent &evt, ecs::EntityManager &manager)
{
  if (evt.type != SetResolutionEvent::Type::SETTINGS_CHANGED)
    return;
  if (!renderer_has_feature(FeatureRenderFlags::BLOOM))
    return;
  init_bloom_ecs_query(manager.getSingletonEntity(ECS_HASH("bloom")),
    [postfx_resolution = evt.postFxResolution](dag::Vector<dabfg::NodeHandle> &bloom__downsample_chain,
      dag::Vector<dabfg::NodeHandle> &bloom__upsample_chain, float bloom__upSample,
      E3DCOLOR bloom__halation_color = E3DCOLOR(255, 0, 0, 0), float bloom__halation_brightness = 2,
      float bloom__halation_mip_factor = 2, int bloom__halation_end_mip = 1) {
      const uint32_t mipsCount = calculate_bloom_mips(postfx_resolution.x, postfx_resolution.y);
      regenerate_downsample_chain(bloom__downsample_chain, mipsCount);
      regenerate_upsample_chain(bloom__upsample_chain, mipsCount, bloom__upSample,
        color3(bloom__halation_color) * max(0.f, bloom__halation_brightness), max(0.f, bloom__halation_mip_factor),
        max(0, bloom__halation_end_mip));
    });
}

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void init_bloom_es(const ecs::Event &, ecs::EntityManager &manager)
{
  ecs::EntityId eid = manager.getOrCreateSingletonEntity(ECS_HASH("bloom"));
  if (!renderer_has_feature(FeatureRenderFlags::BLOOM))
    return;
  init_bloom_ecs_query(eid,
    [](dag::Vector<dabfg::NodeHandle> &bloom__downsample_chain, dag::Vector<dabfg::NodeHandle> &bloom__upsample_chain,
      float bloom__upSample, E3DCOLOR bloom__halation_color = E3DCOLOR(255, 0, 0, 0), float bloom__halation_brightness = 2,
      float bloom__halation_mip_factor = 2, int bloom__halation_end_mip = 1) {
      IPoint2 postfx_resolution = get_postfx_resolution();
      const uint32_t mipsCount = calculate_bloom_mips(postfx_resolution.x, postfx_resolution.y);
      regenerate_downsample_chain(bloom__downsample_chain, mipsCount);
      regenerate_upsample_chain(bloom__upsample_chain, mipsCount, bloom__upSample,
        color3(bloom__halation_color) * max(0.f, bloom__halation_brightness), max(0.f, bloom__halation_mip_factor),
        max(0, bloom__halation_end_mip));
    });
}

ECS_TAG(render)
ECS_TRACK(bloom__upSample, bloom__halation_color, bloom__halation_brightness, bloom__halation_end_mip, bloom__halation_mip_factor)
static void change_bloom_params_with_fg_change_es(const ecs::Event &,
  dag::Vector<dabfg::NodeHandle> &bloom__upsample_chain,
  float bloom__upSample,
  E3DCOLOR bloom__halation_color = E3DCOLOR(255, 0, 0, 0),
  float bloom__halation_brightness = 2,
  float bloom__halation_mip_factor = 2,
  int bloom__halation_end_mip = 1)
{
  IPoint2 postfx_resolution = get_postfx_resolution();
  const uint32_t mipsCount = calculate_bloom_mips(postfx_resolution.x, postfx_resolution.y);
  regenerate_upsample_chain(bloom__upsample_chain, mipsCount, bloom__upSample,
    color3(bloom__halation_color) * max(0.f, bloom__halation_brightness), max(0.f, bloom__halation_mip_factor),
    max(0, bloom__halation_end_mip));
}


ECS_TAG(render)
ECS_TRACK(bloom__threshold, bloom__radius, bloom__mul)
ECS_ON_EVENT(on_appear)
static void change_bloom_shader_vars_es(const ecs::Event &, float bloom__radius, float bloom__threshold = 1, float bloom__mul = 0.1)
{
  ShaderGlobal::set_real(bloom_radiusVarId, bloom__radius);
  ShaderGlobal::set_real(bloom_thresholdVarId, bloom__threshold);
  ShaderGlobal::set_real(bloom_mulVarId, bloom__mul);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_bloom_nodes_es(const ecs::Event &, resource_slot::NodeHandleWithSlotsAccess &bloom__downsample_node)
{
  bloom__downsample_node = resource_slot::register_access("bloom_downsample", DABFG_PP_NODE_SRC,
    {resource_slot::Read{"postfx_input_slot"}}, [](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.create("downsampled_color", dabfg::History::No)
        .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_ESRAM_ONLY, registry.getResolution<2>("post_fx", 0.5f)});
      registry.requestRenderPass().color({"downsampled_color"});
      registry.read(slotsState.resourceToReadFrom("postfx_input_slot"))
        .texture()
        .atStage(dabfg::Stage::PS)
        .bindToShaderVar("blur_src_tex");
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("bloom_clamp_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("blur_src_tex_samplerstate");
      return [renderer = PostFxRenderer("frame_bloom_downsample")] { renderer.render(); };
    });
}
