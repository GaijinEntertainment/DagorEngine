#include "effectAreaES.cpp.inl"
ECS_DEF_PULL_VAR(effectArea);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_effect_area_on_pause_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("effect_area"), ecs::ComponentTypeInfo<EffectArea>()},
  {ECS_HASH("effect_area__isPaused"), ecs::ComponentTypeInfo<bool>()}
};
static void update_effect_area_on_pause_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RW_COMP(update_effect_area_on_pause_es_comps, "effect_area__isPaused", bool)) )
      continue;
    update_effect_area_on_pause_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(update_effect_area_on_pause_es_comps, "effect_area", EffectArea)
    , ECS_RW_COMP(update_effect_area_on_pause_es_comps, "effect_area__isPaused", bool)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_effect_area_on_pause_es_es_desc
(
  "update_effect_area_on_pause_es",
  "prog/daNetGameLibs/effect_area/render/effectAreaES.cpp.inl",
  ecs::EntitySystemOps(update_effect_area_on_pause_es_all),
  make_span(update_effect_area_on_pause_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_net_phys_sync");
static constexpr ecs::ComponentDesc update_effect_area_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("effect_area"), ecs::ComponentTypeInfo<EffectArea>()},
  {ECS_HASH("effect_area__isPaused"), ecs::ComponentTypeInfo<bool>()},
//start of 4 ro components at [2]
  {ECS_HASH("effect_area__frequency"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect_area__pauseChance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("effect_area__pauseLengthMax"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void update_effect_area_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(!ECS_RW_COMP(update_effect_area_es_comps, "effect_area__isPaused", bool)) )
      continue;
    update_effect_area_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(update_effect_area_es_comps, "effect_area", EffectArea)
    , ECS_RO_COMP(update_effect_area_es_comps, "effect_area__frequency", float)
    , ECS_RO_COMP(update_effect_area_es_comps, "effect_area__pauseChance", float)
    , ECS_RO_COMP(update_effect_area_es_comps, "effect_area__pauseLengthMax", float)
    , ECS_RW_COMP(update_effect_area_es_comps, "effect_area__isPaused", bool)
    , ECS_RO_COMP(update_effect_area_es_comps, "transform", TMatrix)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_effect_area_es_es_desc
(
  "update_effect_area_es",
  "prog/daNetGameLibs/effect_area/render/effectAreaES.cpp.inl",
  ecs::EntitySystemOps(update_effect_area_es_all),
  make_span(update_effect_area_es_comps+0, 2)/*rw*/,
  make_span(update_effect_area_es_comps+2, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_net_phys_sync");
static constexpr ecs::ComponentDesc effect_area_effect_name_change_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect_area"), ecs::ComponentTypeInfo<EffectArea>()},
//start of 1 ro components at [1]
  {ECS_HASH("effect_area__effectName"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void effect_area_effect_name_change_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    effect_area_effect_name_change_es_event_handler(evt
        , ECS_RO_COMP(effect_area_effect_name_change_es_event_handler_comps, "effect_area__effectName", ecs::string)
    , ECS_RW_COMP(effect_area_effect_name_change_es_event_handler_comps, "effect_area", EffectArea)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc effect_area_effect_name_change_es_event_handler_es_desc
(
  "effect_area_effect_name_change_es",
  "prog/daNetGameLibs/effect_area/render/effectAreaES.cpp.inl",
  ecs::EntitySystemOps(nullptr, effect_area_effect_name_change_es_event_handler_all_events),
  make_span(effect_area_effect_name_change_es_event_handler_comps+0, 1)/*rw*/,
  make_span(effect_area_effect_name_change_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"effect_area__effectName");
