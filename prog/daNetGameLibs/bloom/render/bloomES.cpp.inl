// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>

// for ECS events
#include "render/renderEvent.h"
#include "render/renderLibsAllowed.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>

#include <render/rendererFeatures.h>
#include <render/resolution.h>

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/bloomCore/bloomCore.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_renderStates.h>
#include <util/dag_convar.h>

#define GLOBAL_VARS_BLOOM \
  VAR(bloom_threshold)    \
  VAR(bloom_mul)          \
  VAR(bloom_radius)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_BLOOM
#undef VAR

static bool is_supported() { return is_render_lib_allowed("bloom") && renderer_has_feature(FeatureRenderFlags::BLOOM); }

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
      [](dag::Vector<dafg::NodeHandle> &bloom__downsample_chain, dag::Vector<dafg::NodeHandle> &bloom__upsample_chain) {
        bloom__downsample_chain.clear();
        bloom__upsample_chain.clear();
      });
}

ECS_TAG(render)
static void resize_bloom_es(const SetResolutionEvent &evt, ecs::EntityManager &manager)
{
  if (evt.type != SetResolutionEvent::Type::SETTINGS_CHANGED)
    return;
  if (!is_supported())
    return;
  init_bloom_ecs_query(manager.getSingletonEntity(ECS_HASH("bloom")),
    [postfx_resolution = evt.postFxResolution](dag::Vector<dafg::NodeHandle> &bloom__downsample_chain,
      dag::Vector<dafg::NodeHandle> &bloom__upsample_chain, float bloom__upSample,
      E3DCOLOR bloom__halation_color = E3DCOLOR(255, 0, 0, 0), float bloom__halation_brightness = 2,
      float bloom__halation_mip_factor = 2, int bloom__halation_end_mip = 1) {
      const uint32_t mipsCount = bloom::calculate_bloom_mips(postfx_resolution.x, postfx_resolution.y);
      bloom::regenerate_downsample_chain(bloom__downsample_chain, mipsCount);
      bloom::regenerate_upsample_chain(bloom__upsample_chain, mipsCount, bloom__upSample,
        color3(bloom__halation_color) * max(0.f, bloom__halation_brightness), max(0.f, bloom__halation_mip_factor),
        max(0, bloom__halation_end_mip));
    });
}

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void init_bloom_es(const ecs::Event &, ecs::EntityManager &manager)
{
  ecs::EntityId eid = manager.getOrCreateSingletonEntity(ECS_HASH("bloom"));
  if (!is_supported())
    return;
  init_bloom_ecs_query(eid,
    [](dag::Vector<dafg::NodeHandle> &bloom__downsample_chain, dag::Vector<dafg::NodeHandle> &bloom__upsample_chain,
      float bloom__upSample, E3DCOLOR bloom__halation_color = E3DCOLOR(255, 0, 0, 0), float bloom__halation_brightness = 2,
      float bloom__halation_mip_factor = 2, int bloom__halation_end_mip = 1) {
      IPoint2 postfx_resolution = get_postfx_resolution();
      const uint32_t mipsCount = bloom::calculate_bloom_mips(postfx_resolution.x, postfx_resolution.y);
      bloom::regenerate_downsample_chain(bloom__downsample_chain, mipsCount);
      bloom::regenerate_upsample_chain(bloom__upsample_chain, mipsCount, bloom__upSample,
        color3(bloom__halation_color) * max(0.f, bloom__halation_brightness), max(0.f, bloom__halation_mip_factor),
        max(0, bloom__halation_end_mip));
    });
}

ECS_TAG(render)
ECS_TRACK(bloom__upSample, bloom__halation_color, bloom__halation_brightness, bloom__halation_end_mip, bloom__halation_mip_factor)
static void change_bloom_params_with_fg_change_es(const ecs::Event &,
  dag::Vector<dafg::NodeHandle> &bloom__upsample_chain,
  float bloom__upSample,
  E3DCOLOR bloom__halation_color = E3DCOLOR(255, 0, 0, 0),
  float bloom__halation_brightness = 2,
  float bloom__halation_mip_factor = 2,
  int bloom__halation_end_mip = 1)
{
  IPoint2 postfx_resolution = get_postfx_resolution();
  const uint32_t mipsCount = bloom::calculate_bloom_mips(postfx_resolution.x, postfx_resolution.y);
  bloom::regenerate_upsample_chain(bloom__upsample_chain, mipsCount, bloom__upSample,
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
  bloom__downsample_node = resource_slot::register_access("bloom_downsample", DAFG_PP_NODE_SRC,
    {resource_slot::Read{"postfx_input_slot"}}, [](resource_slot::State slotsState, dafg::Registry registry) {
      registry.create("downsampled_color", dafg::History::No)
        .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_ESRAM_ONLY, registry.getResolution<2>("post_fx", 0.5f)});
      registry.requestRenderPass().color({"downsampled_color"});
      registry.read(slotsState.resourceToReadFrom("postfx_input_slot"))
        .texture()
        .atStage(dafg::Stage::PS)
        .bindToShaderVar("blur_src_tex");
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("bloom_clamp_sampler", dafg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("blur_src_tex_samplerstate");

      // @TODO: fix local tonemapping dependency on this node and disable it when bloom is not enabled
      //        now this node is the one that produces downsampled_color, which is the source for local tonemapping on low q
      return [renderer = is_render_lib_allowed("bloom") ? PostFxRenderer("frame_bloom_downsample") : PostFxRenderer()]() {
        renderer.render();
      };
    });
}
