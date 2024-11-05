// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "motionBlurECS.h"
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityId.h>
#include <ecs/render/updateStageRender.h>
#include <util/dag_console.h>
#include <render/world/frameGraphNodes/motionBlurNode.h>
#include <render/world/wrDispatcher.h>

ECS_REGISTER_BOXED_TYPE(MotionBlur, nullptr)

static void enable_motion_blur_nodes_based_on_scale(float scale, MotionBlurNodePointers pointers)
{
  if (scale >= MotionBlur::SCALE_EPS)
    enable_motion_blur_node(pointers);
  else
    disable_motion_blur_node(pointers);
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void motion_blur_update_es(const UpdateStageInfoBeforeRender &evt, MotionBlur &motion_blur)
{
  motion_blur.update(evt.realDt, evt.camPos);
}

template <typename Callable>
static void query_motion_blur_ecs_query(Callable c);

eastl::pair<MotionBlur *, MotionBlurMode> query_motion_blur()
{
  MotionBlur *ptr = nullptr;
  MotionBlurMode mode = MotionBlurMode::MOTION_VECTOR;
  query_motion_blur_ecs_query([&](MotionBlur &motion_blur, int motion_blur__mode) {
    ptr = &motion_blur;
    mode = static_cast<MotionBlurMode>(clamp(motion_blur__mode, 0, 2));
  });
  return {ptr, mode};
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(motion_blur__scale,
  motion_blur__velocityMultiplier,
  motion_blur__maxVelocity,
  motion_blur__alphaMulOnApply,
  motion_blur__overscanMul,
  motion_blur__forwardBlur)
static void motion_blur_settings_es(const ecs::Event &,
  MotionBlur &motion_blur,
  const float motion_blur__scale,
  const float motion_blur__velocityMultiplier,
  const float motion_blur__maxVelocity,
  const float motion_blur__alphaMulOnApply,
  const float motion_blur__overscanMul,
  const float motion_blur__forwardBlur)
{
  MotionBlur::Settings settings;
  settings.scale = motion_blur__scale;
  settings.velocityMultiplier = motion_blur__velocityMultiplier;
  settings.maxVelocity = motion_blur__maxVelocity;
  settings.alphaMulOnApply = motion_blur__alphaMulOnApply;
  settings.overscanMul = motion_blur__overscanMul;
  settings.forwardBlur = motion_blur__forwardBlur;
  motion_blur.useSettings(settings);
  enable_motion_blur_nodes_based_on_scale(motion_blur__scale, WRDispatcher::getMotionBlurNodePointers());
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(MotionBlur motion_blur)
static void motion_blur_destroyed_es(const ecs::Event &) { disable_motion_blur_node(WRDispatcher::getMotionBlurNodePointers()); }

template <typename Callable>
ECS_REQUIRE(MotionBlur motion_blur)
static void query_motion_blur_scale_ecs_query(Callable c);

void recreate_motion_blur_nodes(MotionBlurNodePointers pointers)
{
  float scale = 0;
  query_motion_blur_scale_ecs_query([&scale](float motion_blur__scale) { scale = motion_blur__scale; });
  enable_motion_blur_nodes_based_on_scale(scale, pointers);
}

template <typename Callable>
static void set_motion_blur_mode_ecs_query(Callable c);

template <typename Callable>
ECS_REQUIRE(MotionBlur motion_blur)
static void delete_motion_blur_ecs_query(Callable c);

static bool motion_blur_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("motion_blur", "use_mode", 2, 2)
  {
    int modeRaw = console::to_int(argv[1]);
    if (modeRaw >= 0 && modeRaw <= 2)
    {
      MotionBlurMode mode = static_cast<MotionBlurMode>(modeRaw);
      switch (mode)
      {
        case MotionBlurMode::RAW_DEPTH: console::print_d("Using mode RAW_DEPTH"); break;
        case MotionBlurMode::LINEAR_DEPTH: console::print_d("Using mode LINEAR_DEPTH"); break;
        default: console::print_d("Using mode MOTION_VECTOR"); break;
      }
      bool entitySet = false;
      set_motion_blur_mode_ecs_query([modeRaw, &entitySet](int &motion_blur__mode) {
        entitySet = true;
        motion_blur__mode = modeRaw;
      });
      if (!entitySet)
      {
        console::print_d("Creating motion blur entity...");
        ecs::ComponentsInitializer init;
        init[ECS_HASH("motion_blur__mode")] = modeRaw;
        g_entity_mgr->createEntityAsync("motion_blur", eastl::move(init));
      }
    }
    else
    {
      console::print_d("Deleting motion blur entity...");
      delete_motion_blur_ecs_query([](ecs::EntityId eid) { g_entity_mgr->destroyEntity(eid); });
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(motion_blur_console_handler);