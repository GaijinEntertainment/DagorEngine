// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <render/renderEvent.h>
#include <render/resourceSlot/registerAccess.h>
#include <shaders/dag_postFxRenderer.h>
#include <ecs/render/resPtr.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <ecs/render/updateStageRender.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/screen_effect.hlsli"


template <typename Callable>
static void screen_effect_renderer_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
static void get_screen_effect_renderer_ecs_query(Callable);

ECS_TAG(render)
static void screen_effect_renderer_init_es_event_handler(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  screen_effect_renderer_ecs_query(manager.getOrCreateSingletonEntity(ECS_HASH("screen_effect_renderer")),
    [&](ecs::EntityId eid, UniqueBufHolder &screen_effect__buffer, int &screen_effect__countVar,
      ecs::IntList &screen_effect__texVars) {
      screen_effect__texVars.reserve(MAX_SCREEN_EFFECTS);
      screen_effect__countVar = get_shader_variable_id("screen_effects_count");
      for (int i = 0; i < MAX_SCREEN_EFFECTS; i++)
      {
        screen_effect__texVars.push_back(get_shader_variable_id(String(0, "screen_effect_tex%d", i).c_str()));
      }

      screen_effect__buffer = dag::buffers::create_persistent_cb(dag::buffers::cb_array_reg_count<ScreenEffect>(MAX_SCREEN_EFFECTS),
        "screen_effects_buffer");

      if (!screen_effect__buffer)
      {
        logerr("failed init screen_effect_renderer, screen_effect__buffer (%s)", screen_effect__buffer ? "valid" : "invalid");
        manager.destroyEntity(eid);
      }
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void screen_effect_renderer_close_es(const ecs::Event &, const ecs::IntList &screen_effect__texVars)
{
  for (int var : screen_effect__texVars)
    ShaderGlobal::set_texture(var, BAD_TEXTUREID);
}

ECS_TAG(render)
ECS_NO_ORDER
static void screen_effect_render_es(const UpdateStageInfoBeforeRender &,
  const UniqueBufHolder &screen_effect__buffer,
  int screen_effect__countVar,
  const ecs::IntList &screen_effect__texVars,
  const bool screen_effect_renderer__enable,
  bool &screen_effect__is_active)
{
  if (!screen_effect_renderer__enable)
    return;

  eastl::fixed_vector<ScreenEffect, MAX_SCREEN_EFFECTS, false> effects;
  eastl::fixed_vector<TEXTUREID, MAX_SCREEN_EFFECTS, false> textures;
  float weightsSum = 0.f;

  get_screen_effect_renderer_ecs_query(
    [&](const SharedTex &screen_effect__texture, Point4 screen_effect__diffuse, float screen_effect__uvScale,
      float screen_effect__roughness, float screen_effect__opacity, float screen_effect__intensity, float screen_effect__weight,
      float screen_effect__borderOffset, float screen_effect__borderSaturation) {
      if (effects.size() == MAX_SCREEN_EFFECTS)
      {
        logerr("screen_effects count more then max allowed %d", MAX_SCREEN_EFFECTS);
        return;
      }
      if (screen_effect__weight < 0.f)
      {
        logerr("less zero screen_effect__weight = %f", screen_effect__weight);
        return;
      }
      TextureInfo info;
      screen_effect__texture.getTex2D()->getinfo(info);
      textures.push_back(screen_effect__texture.getTexId());
      ScreenEffect &effect = effects.emplace_back();
      effect.uv_scale = screen_effect__uvScale * float2(1.f / info.w, 1.f / info.h);
      effect.diffuse = screen_effect__diffuse;
      effect.roughness = screen_effect__roughness;
      effect.opacity = screen_effect__opacity;
      effect.intensity = screen_effect__intensity;
      effect.weight = screen_effect__weight;
      effect.border_offset = screen_effect__borderOffset;
      effect.border_saturation = screen_effect__borderSaturation;
      weightsSum += screen_effect__weight;
    });

  int effectsCount = effects.size();
  screen_effect__is_active = !(effectsCount == 0 || weightsSum < 1e-6);
  if (!screen_effect__is_active)
    return;

  for (ScreenEffect &effect : effects)
    effect.weight /= weightsSum;

  ShaderGlobal::set_int(screen_effect__countVar, effectsCount);

  for (int i = 0; i < effectsCount; i++)
    ShaderGlobal::set_texture(screen_effect__texVars[i], textures[i]);

  screen_effect__buffer.getBuf()->updateData(0, effectsCount * sizeof(ScreenEffect), effects.data(),
    VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

ECS_TAG(render)
ECS_TRACK(screen_effect__is_active)
ECS_REQUIRE(eastl::true_type screen_effect__is_active)
static void screen_effect_node_enable_es(const ecs::Event &, resource_slot::NodeHandleWithSlotsAccess &screenEffect)
{
  screenEffect = resource_slot::register_access("screen_effect", DABFG_PP_NODE_SRC,
    {resource_slot::Update{"postfx_input_slot", "screen_effects_frame", 200}},
    [](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.createTexture2d(slotsState.resourceToCreateFor("postfx_input_slot"), dabfg::History::No,
        {TEXFMT_R11G11B10F | TEXCF_RTARGET, registry.getResolution<2>("post_fx")});
      registry.requestRenderPass().color({slotsState.resourceToCreateFor("postfx_input_slot")});
      registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot")).atStage(dabfg::Stage::PS).bindToShaderVar("frame_tex");
      registry.read("downsampled_depth_with_late_water").texture().atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_depth");
      registry.read("prev_frame_tex").texture().atStage(dabfg::Stage::PS).bindToShaderVar();

      return [shader = PostFxRenderer("screen_effect")] { shader.render(); };
    });
}

ECS_TAG(render)
ECS_TRACK(screen_effect__is_active)
ECS_REQUIRE(eastl::false_type screen_effect__is_active)
static void screen_effect_node_disable_es(const ecs::Event &, resource_slot::NodeHandleWithSlotsAccess &screenEffect)
{
  screenEffect.reset();
}
