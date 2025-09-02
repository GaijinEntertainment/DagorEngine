// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "effectEntityES.cpp.inl"
ECS_DEF_PULL_VAR(effectEntity);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc effect_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 3 no components at [2]
  {ECS_HASH("effect_animation__transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("attachedEffect"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("staticEffect"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void effect_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    effect_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(effect_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_es_es_desc
(
  "effect_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(effect_es_all),
  make_span(effect_es_comps+0, 1)/*rw*/,
  make_span(effect_es_comps+1, 1)/*ro*/,
  empty_span(),
  make_span(effect_es_comps+2, 3)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc effect_debug_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 2 rq components at [2]
  {ECS_HASH("daeditor__selected"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("staticEffect"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 2 no components at [4]
  {ECS_HASH("effect_animation__transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("attachedEffect"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void effect_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    effect_debug_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(effect_debug_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_debug_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_debug_es_es_desc
(
  "effect_debug_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(effect_debug_es_all),
  make_span(effect_debug_es_comps+0, 1)/*rw*/,
  make_span(effect_debug_es_comps+1, 1)/*ro*/,
  make_span(effect_debug_es_comps+2, 2)/*rq*/,
  make_span(effect_debug_es_comps+4, 2)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc effect_attached_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("attachedEffect"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 1 no components at [3]
  {ECS_HASH("effect_animation__transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void effect_attached_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    effect_attached_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(effect_attached_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_attached_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_attached_es_es_desc
(
  "effect_attached_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(effect_attached_es_all),
  make_span(effect_attached_es_comps+0, 1)/*rw*/,
  make_span(effect_attached_es_comps+1, 1)/*ro*/,
  make_span(effect_attached_es_comps+2, 1)/*rq*/,
  make_span(effect_attached_es_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc effect_anim_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect_animation__transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 2 no components at [2]
  {ECS_HASH("attachedEffect"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("attachedNoScaleEffect"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void effect_anim_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    effect_anim_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(effect_anim_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_anim_es_comps, "effect_animation__transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_anim_es_es_desc
(
  "effect_anim_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(effect_anim_es_all),
  make_span(effect_anim_es_comps+0, 1)/*rw*/,
  make_span(effect_anim_es_comps+1, 1)/*ro*/,
  empty_span(),
  make_span(effect_anim_es_comps+2, 2)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"effect_attached_es");
static constexpr ecs::ComponentDesc effect_attached_anim_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect_animation__transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("attachedEffect"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void effect_attached_anim_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    effect_attached_anim_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(effect_attached_anim_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_attached_anim_es_comps, "effect_animation__transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_attached_anim_es_es_desc
(
  "effect_attached_anim_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(effect_attached_anim_es_all),
  make_span(effect_attached_anim_es_comps+0, 1)/*rw*/,
  make_span(effect_attached_anim_es_comps+1, 1)/*ro*/,
  make_span(effect_attached_anim_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"effect_attached_es");
static constexpr ecs::ComponentDesc bound_camera_effect_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("effect__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("camera_prev_pos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("camera_prev_vel"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 3 ro components at [4]
  {ECS_HASH("effect__smooth_coef"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect__bias_coef"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect__use_deprecated_bound_camera_logic"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void bound_camera_effect_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    bound_camera_effect_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(bound_camera_effect_es_comps, "effect__offset", Point3)
    , ECS_RW_COMP(bound_camera_effect_es_comps, "camera_prev_pos", Point3)
    , ECS_RW_COMP(bound_camera_effect_es_comps, "camera_prev_vel", Point3)
    , ECS_RW_COMP(bound_camera_effect_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(bound_camera_effect_es_comps, "effect__smooth_coef", float)
    , ECS_RO_COMP(bound_camera_effect_es_comps, "effect__bias_coef", float)
    , ECS_RO_COMP_OR(bound_camera_effect_es_comps, "effect__use_deprecated_bound_camera_logic", bool(true))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bound_camera_effect_es_es_desc
(
  "bound_camera_effect_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(bound_camera_effect_es_all),
  make_span(bound_camera_effect_es_comps+0, 4)/*rw*/,
  make_span(bound_camera_effect_es_comps+4, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"effect_es","camera_animator_update_es");
static constexpr ecs::ComponentDesc biome_query_update_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("biome_query__state"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("biome_query__color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("biome_query__groupId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("biome_query__liveFrames"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [4]
  {ECS_HASH("biome_query__id"), ecs::ComponentTypeInfo<int>()}
};
static void biome_query_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    biome_query_update_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(biome_query_update_es_comps, "biome_query__state", int)
    , ECS_RO_COMP(biome_query_update_es_comps, "biome_query__id", int)
    , ECS_RW_COMP(biome_query_update_es_comps, "biome_query__color", Point4)
    , ECS_RW_COMP(biome_query_update_es_comps, "biome_query__groupId", int)
    , ECS_RW_COMP(biome_query_update_es_comps, "biome_query__liveFrames", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc biome_query_update_es_es_desc
(
  "biome_query_update_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(biome_query_update_es_all),
  make_span(biome_query_update_es_comps+0, 4)/*rw*/,
  make_span(biome_query_update_es_comps+4, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc pause_effects_es_comps[] ={};
static void pause_effects_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    pause_effects_es(*info.cast<ecs::UpdateStageInfoAct>()
    , components.manager()
    );
}
static ecs::EntitySystemDesc pause_effects_es_es_desc
(
  "pause_effects_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(pause_effects_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"after_camera_sync");
static constexpr ecs::ComponentDesc fx_magnification_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("camera__totalMagnification"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void fx_magnification_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    fx_magnification_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(fx_magnification_es_comps, "camera__totalMagnification", float)
    , ECS_RO_COMP(fx_magnification_es_comps, "camera__active", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc fx_magnification_es_es_desc
(
  "fx_magnification_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(fx_magnification_es_all),
  empty_span(),
  make_span(fx_magnification_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc alwasy_in_viewport_effects_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [1]
  {ECS_HASH("alwaysInViewportEffect"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void alwasy_in_viewport_effects_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    alwasy_in_viewport_effects_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(alwasy_in_viewport_effects_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc alwasy_in_viewport_effects_es_es_desc
(
  "alwasy_in_viewport_effects_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(alwasy_in_viewport_effects_es_all),
  make_span(alwasy_in_viewport_effects_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(alwasy_in_viewport_effects_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"effect_es");
static constexpr ecs::ComponentDesc effect_scale_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 2 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect__scale"), ecs::ComponentTypeInfo<float>()}
};
static void effect_scale_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_scale_es_event_handler(evt
        , ECS_RW_COMP(effect_scale_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_scale_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(effect_scale_es_event_handler_comps, "effect__scale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_scale_es_event_handler_es_desc
(
  "effect_scale_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_scale_es_event_handler_all_events),
  make_span(effect_scale_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_scale_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__scale,transform");
static constexpr ecs::ComponentDesc effect_velocity_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect__velocity"), ecs::ComponentTypeInfo<Point3>()}
};
static void effect_velocity_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_velocity_es_event_handler(evt
        , ECS_RW_COMP(effect_velocity_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_velocity_es_event_handler_comps, "effect__velocity", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_velocity_es_event_handler_es_desc
(
  "effect_velocity_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_velocity_es_event_handler_all_events),
  make_span(effect_velocity_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_velocity_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__velocity");
static constexpr ecs::ComponentDesc effect_fake_brightness_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect__background_pos"), ecs::ComponentTypeInfo<Point3>()}
};
static void effect_fake_brightness_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_fake_brightness_es_event_handler(evt
        , ECS_RW_COMP(effect_fake_brightness_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_fake_brightness_es_event_handler_comps, "effect__background_pos", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_fake_brightness_es_event_handler_es_desc
(
  "effect_fake_brightness_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_fake_brightness_es_event_handler_all_events),
  make_span(effect_fake_brightness_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_fake_brightness_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__background_pos");
static constexpr ecs::ComponentDesc effect_velocity_scale_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect__distance_scale"), ecs::ComponentTypeInfo<float>()}
};
static void effect_velocity_scale_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_velocity_scale_es_event_handler(evt
        , ECS_RW_COMP(effect_velocity_scale_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_velocity_scale_es_event_handler_comps, "effect__distance_scale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_velocity_scale_es_event_handler_es_desc
(
  "effect_velocity_scale_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_velocity_scale_es_event_handler_all_events),
  make_span(effect_velocity_scale_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_velocity_scale_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__distance_scale");
static constexpr ecs::ComponentDesc effect_wind_scale_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect__windScale"), ecs::ComponentTypeInfo<float>()}
};
static void effect_wind_scale_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_wind_scale_es_event_handler(evt
        , ECS_RW_COMP(effect_wind_scale_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_wind_scale_es_event_handler_comps, "effect__windScale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_wind_scale_es_event_handler_es_desc
(
  "effect_wind_scale_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_wind_scale_es_event_handler_all_events),
  make_span(effect_wind_scale_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_wind_scale_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__windScale");
static constexpr ecs::ComponentDesc effect_colorMult_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect__colorMult"), ecs::ComponentTypeInfo<E3DCOLOR>()}
};
static void effect_colorMult_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_colorMult_es_event_handler(evt
        , ECS_RW_COMP(effect_colorMult_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_colorMult_es_event_handler_comps, "effect__colorMult", E3DCOLOR)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_colorMult_es_event_handler_es_desc
(
  "effect_colorMult_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_colorMult_es_event_handler_all_events),
  make_span(effect_colorMult_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_colorMult_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__colorMult");
static constexpr ecs::ComponentDesc effect_spawnrate_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect__spawnRate"), ecs::ComponentTypeInfo<float>()}
};
static void effect_spawnrate_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_spawnrate_es_event_handler(evt
        , ECS_RW_COMP(effect_spawnrate_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_spawnrate_es_event_handler_comps, "effect__spawnRate", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_spawnrate_es_event_handler_es_desc
(
  "effect_spawnrate_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_spawnrate_es_event_handler_all_events),
  make_span(effect_spawnrate_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_spawnrate_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect__spawnRate");
static constexpr ecs::ComponentDesc effect_box_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 4 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect__box"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect__use_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("effect__automatic_box"), ecs::ComponentTypeInfo<bool>()}
};
static void effect_box_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_box_es_event_handler(evt
        , ECS_RW_COMP(effect_box_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(effect_box_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(effect_box_es_event_handler_comps, "effect__box", TMatrix)
    , ECS_RO_COMP(effect_box_es_event_handler_comps, "effect__use_box", bool)
    , ECS_RO_COMP(effect_box_es_event_handler_comps, "effect__automatic_box", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_box_es_event_handler_es_desc
(
  "effect_box_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_box_es_event_handler_all_events),
  make_span(effect_box_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_box_es_event_handler_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateEffectRestrictionBoxes>::build(),
  0
,nullptr,"effect__automatic_box,effect__box,effect__use_box,transform");
static constexpr ecs::ComponentDesc biome_query_init_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("biome_query__state"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("biome_query__id"), ecs::ComponentTypeInfo<int>()},
//start of 2 ro components at [2]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("biome_query__radius"), ecs::ComponentTypeInfo<float>()}
};
static void biome_query_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    biome_query_init_es(evt
        , ECS_RO_COMP(biome_query_init_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(biome_query_init_es_comps, "biome_query__state", int)
    , ECS_RW_COMP(biome_query_init_es_comps, "biome_query__id", int)
    , ECS_RO_COMP(biome_query_init_es_comps, "biome_query__radius", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc biome_query_init_es_es_desc
(
  "biome_query_init_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, biome_query_init_es_all_events),
  make_span(biome_query_init_es_comps+0, 2)/*rw*/,
  make_span(biome_query_init_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc biome_query_calculate_replacement_id_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("biome_query__biomeReplaceIdList"), ecs::ComponentTypeInfo<ecs::IntList>()},
//start of 1 ro components at [1]
  {ECS_HASH("biome_query__biomeReplaceNameList"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void biome_query_calculate_replacement_id_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    biome_query_calculate_replacement_id_es(evt
        , ECS_RO_COMP(biome_query_calculate_replacement_id_es_comps, "biome_query__biomeReplaceNameList", ecs::StringList)
    , ECS_RW_COMP(biome_query_calculate_replacement_id_es_comps, "biome_query__biomeReplaceIdList", ecs::IntList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc biome_query_calculate_replacement_id_es_es_desc
(
  "biome_query_calculate_replacement_id_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, biome_query_calculate_replacement_id_es_all_events),
  make_span(biome_query_calculate_replacement_id_es_comps+0, 1)/*rw*/,
  make_span(biome_query_calculate_replacement_id_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_fx_based_on_biome_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("biome_query__color"), ecs::ComponentTypeInfo<Point4>()},
//start of 6 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("biome_query__state"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("biome_query__groupId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("hit_fx_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("biome_query__desiredBiomeName"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static void create_fx_based_on_biome_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_fx_based_on_biome_es(evt
        , ECS_RO_COMP(create_fx_based_on_biome_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(create_fx_based_on_biome_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(create_fx_based_on_biome_es_comps, "biome_query__state", int)
    , ECS_RO_COMP(create_fx_based_on_biome_es_comps, "biome_query__groupId", int)
    , ECS_RW_COMP(create_fx_based_on_biome_es_comps, "biome_query__color", Point4)
    , ECS_RO_COMP(create_fx_based_on_biome_es_comps, "hit_fx_name", ecs::string)
    , ECS_RO_COMP_OR(create_fx_based_on_biome_es_comps, "biome_query__desiredBiomeName", ecs::string(""))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_fx_based_on_biome_es_es_desc
(
  "create_fx_based_on_biome_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_fx_based_on_biome_es_all_events),
  make_span(create_fx_based_on_biome_es_comps+0, 1)/*rw*/,
  make_span(create_fx_based_on_biome_es_comps+1, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","biome_query__state");
static constexpr ecs::ComponentDesc paint_hit_fx_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 3 ro components at [1]
  {ECS_HASH("paint_color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("paint_from_biome"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("usePaintColor"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void paint_hit_fx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    paint_hit_fx_es(evt
        , ECS_RW_COMP(paint_hit_fx_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(paint_hit_fx_es_comps, "paint_color", Point4)
    , ECS_RO_COMP_OR(paint_hit_fx_es_comps, "paint_from_biome", bool(false))
    , ECS_RO_COMP_OR(paint_hit_fx_es_comps, "usePaintColor", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc paint_hit_fx_es_es_desc
(
  "paint_hit_fx_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, paint_hit_fx_es_all_events),
  make_span(paint_hit_fx_es_comps+0, 1)/*rw*/,
  make_span(paint_hit_fx_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc enable_fx_imm_mode_es_comps[] ={};
static void enable_fx_imm_mode_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerBeforeClear>());
  enable_fx_imm_mode_es(static_cast<const ecs::EventEntityManagerBeforeClear&>(evt)
        );
}
static ecs::EntitySystemDesc enable_fx_imm_mode_es_es_desc
(
  "enable_fx_imm_mode_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, enable_fx_imm_mode_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerBeforeClear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc destroy_effects_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()}
};
static void destroy_effects_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerBeforeClear>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    destroy_effects_es(static_cast<const ecs::EventEntityManagerBeforeClear&>(evt)
        , ECS_RW_COMP(destroy_effects_es_comps, "effect", TheEffect)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc destroy_effects_es_es_desc
(
  "destroy_effects_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_effects_es_all_events),
  make_span(destroy_effects_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerBeforeClear>::build(),
  0
,"render",nullptr,nullptr,"enable_fx_imm_mode_es");
static constexpr ecs::ComponentDesc validate_emit_range_limit_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 rq components at [1]
  {ECS_HASH("autodeleteEffectEntity"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("effect__emit_range_limit"), ecs::ComponentTypeInfo<float>()}
};
static void validate_emit_range_limit_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_emit_range_limit_es(evt
        , ECS_RO_COMP(validate_emit_range_limit_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_emit_range_limit_es_es_desc
(
  "validate_emit_range_limit_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_emit_range_limit_es_all_events),
  empty_span(),
  make_span(validate_emit_range_limit_es_comps+0, 1)/*ro*/,
  make_span(validate_emit_range_limit_es_comps+1, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc pause_effects_set_sq_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("effect__emit_range_limit_start_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect__emit_range_limit_stop_sq"), ecs::ComponentTypeInfo<float>()},
//start of 3 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("effect__emit_range_limit"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect__emit_range_gap"), ecs::ComponentTypeInfo<float>()}
};
static void pause_effects_set_sq_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    pause_effects_set_sq_es(evt
        , ECS_RO_COMP(pause_effects_set_sq_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RO_COMP(pause_effects_set_sq_es_comps, "effect__emit_range_limit", float)
    , ECS_RO_COMP(pause_effects_set_sq_es_comps, "effect__emit_range_gap", float)
    , ECS_RW_COMP(pause_effects_set_sq_es_comps, "effect__emit_range_limit_start_sq", float)
    , ECS_RW_COMP(pause_effects_set_sq_es_comps, "effect__emit_range_limit_stop_sq", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc pause_effects_set_sq_es_es_desc
(
  "pause_effects_set_sq_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, pause_effects_set_sq_es_all_events),
  make_span(pause_effects_set_sq_es_comps+0, 2)/*rw*/,
  make_span(pause_effects_set_sq_es_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc recreate_effect_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void recreate_effect_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    recreate_effect_es(evt
        , components.manager()
    , ECS_RO_COMP(recreate_effect_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(recreate_effect_es_comps, "effect", TheEffect)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc recreate_effect_es_es_desc
(
  "recreate_effect_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, recreate_effect_es_all_events),
  make_span(recreate_effect_es_comps+0, 1)/*rw*/,
  make_span(recreate_effect_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RecreateEffectEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc effect_manager_settings_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__effectsShadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__effects__max_active_shadows"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [2]
  {ECS_HASH("render_settings__shadowsQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void effect_manager_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_manager_settings_es(evt
        , ECS_RO_COMP(effect_manager_settings_es_comps, "render_settings__effectsShadows", bool)
    , ECS_RO_COMP(effect_manager_settings_es_comps, "render_settings__effects__max_active_shadows", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_manager_settings_es_es_desc
(
  "effect_manager_settings_es",
  "prog/daNetGame/render/fx/effectEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_manager_settings_es_all_events),
  empty_span(),
  make_span(effect_manager_settings_es_comps+0, 2)/*ro*/,
  make_span(effect_manager_settings_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__effectsShadows,render_settings__effects__max_active_shadows,render_settings__shadowsQuality",nullptr,"init_shadows_es");
static constexpr ecs::ComponentDesc get_replacement_color_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("biome_query__biomeReplaceIdList"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("biome_query__mudBiomeColor"), ecs::ComponentTypeInfo<Point4>()}
};
static ecs::CompileTimeQueryDesc get_replacement_color_ecs_query_desc
(
  "get_replacement_color_ecs_query",
  empty_span(),
  make_span(get_replacement_color_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_replacement_color_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_replacement_color_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_replacement_color_ecs_query_comps, "biome_query__biomeReplaceIdList", ecs::IntList)
            , ECS_RO_COMP(get_replacement_color_ecs_query_comps, "biome_query__mudBiomeColor", Point4)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc pause_effects_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
  {ECS_HASH("effect__is_paused"), ecs::ComponentTypeInfo<bool>()},
//start of 4 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect__emit_range_limit_start_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect__emit_range_limit_stop_sq"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc pause_effects_ecs_query_desc
(
  "pause_effects_ecs_query",
  make_span(pause_effects_ecs_query_comps+0, 2)/*rw*/,
  make_span(pause_effects_ecs_query_comps+2, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void pause_effects_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, pause_effects_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(pause_effects_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(pause_effects_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(pause_effects_ecs_query_comps, "effect__emit_range_limit_start_sq", float)
            , ECS_RO_COMP(pause_effects_ecs_query_comps, "effect__emit_range_limit_stop_sq", float)
            , ECS_RW_COMP(pause_effects_ecs_query_comps, "effect", TheEffect)
            , ECS_RW_COMP(pause_effects_ecs_query_comps, "effect__is_paused", bool)
            );

        }while (++comp != compE);
    }
  );
}
