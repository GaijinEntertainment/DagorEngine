// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/resourceSlot/registerAccess.h>
#include <shaders/dag_postFxRenderer.h>

template <typename Callable>
inline void start_flash_blind_ecs_query(Callable c);

template <typename Callable>
inline void stop_flash_blind_ecs_query(Callable c);

template <typename Callable>
inline void flash_blind_finish_init_ecs_query(Callable c);

static bool flash_blind_do_capture_query()
{
  bool doCapture = false;
  flash_blind_finish_init_ecs_query([&doCapture](bool &flash_blind__just_captured) {
    if (!flash_blind__just_captured)
    {
      flash_blind__just_captured = true;
      doCapture = true;
    }
  });
  return doCapture;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void flash_blind_es(const ecs::Event &, float flash_blind__intensity)
{
  start_flash_blind_ecs_query(
    [flash_blind__intensity](float flash_blind__decay_factor, resource_slot::NodeHandleWithSlotsAccess &flashBlind,
      float &flash_blind__remaining_time, bool &flash_blind__just_captured) {
      const float DECAY_LIMIT = 0.001f;
      const float flashTime = log(DECAY_LIMIT / flash_blind__intensity) / log(flash_blind__decay_factor);
      flash_blind__remaining_time = max(flash_blind__remaining_time, flashTime);
      flash_blind__just_captured = false;
      flashBlind = resource_slot::register_access("flash_blind", DAFG_PP_NODE_SRC, {resource_slot::Read{"postfx_input_slot"}},
        [](resource_slot::State slotsState, dafg::Registry registry) {
          registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot"))
            .atStage(dafg::Stage::PS)
            .bindToShaderVar("flash_blind_source_tex");
          registry.requestRenderPass().color({registry.createTexture2d("flash_blind_tex", dafg::History::ClearZeroOnFirstFrame,
            {TEXFMT_R11G11B10F | TEXCF_RTARGET, registry.getResolution<2>("post_fx", 0.5f)})});
          registry.readTextureHistory("flash_blind_tex").atStage(dafg::Stage::PS).bindToShaderVar("flash_blind_prev_tex");
          return [renderer = PostFxRenderer("flash_blind"), capture = PostFxRenderer("flash_blind_capture")] {
            if (flash_blind_do_capture_query())
              capture.render();
            else
              renderer.render();
          };
        });
    });
}

ECS_TAG(render)
ECS_REQUIRE(float flash_blind__intensity)
ECS_ON_EVENT(on_disappear)
static void flash_blind_stop_es(const ecs::Event &)
{
  stop_flash_blind_ecs_query([](resource_slot::NodeHandleWithSlotsAccess &flashBlind, float &flash_blind__remaining_time) {
    flashBlind.reset();
    flash_blind__remaining_time = 0.0;
    ShaderGlobal::set_real(::get_shader_variable_id("flash_blind_remaining_time", true), flash_blind__remaining_time);
  });
}