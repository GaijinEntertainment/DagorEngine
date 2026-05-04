// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "auroraBorealisES.cpp.inl"
ECS_DEF_PULL_VAR(auroraBorealis);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc aurora_borealis_switch_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("aurora_borealis__is_night"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("aurora_borealis__night_sun_cos"), ecs::ComponentTypeInfo<float>()}
};
static void aurora_borealis_switch_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    aurora_borealis_switch_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(aurora_borealis_switch_es_comps, "aurora_borealis__is_night", bool)
    , ECS_RO_COMP(aurora_borealis_switch_es_comps, "aurora_borealis__night_sun_cos", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc aurora_borealis_switch_es_es_desc
(
  "aurora_borealis_switch_es",
  "prog/daNetGame/render/fx/auroraBorealisES.cpp.inl",
  ecs::EntitySystemOps(aurora_borealis_switch_es_all),
  make_span(aurora_borealis_switch_es_comps+0, 1)/*rw*/,
  make_span(aurora_borealis_switch_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc aurora_borealis_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("aurora_borealis"), ecs::ComponentTypeInfo<AuroraBorealis>()},
  {ECS_HASH("aurora_borealis__init"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("aurora_borealis__render"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("aurora_borealis__apply"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 13 ro components at [4]
  {ECS_HASH("aurora_borealis__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aurora_borealis__is_night"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aurora_borealis__tex_res_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__top_height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__top_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("aurora_borealis__bottom_height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__bottom_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("aurora_borealis__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__luminance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__ripples_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__ripples_speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aurora_borealis__ripples_strength"), ecs::ComponentTypeInfo<float>()}
};
static void aurora_borealis_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    aurora_borealis_es(evt
        , ECS_RW_COMP(aurora_borealis_es_comps, "aurora_borealis", AuroraBorealis)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__enabled", bool)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__is_night", bool)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__tex_res_multiplier", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__top_height", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__top_color", Point3)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__bottom_height", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__bottom_color", Point3)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__brightness", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__luminance", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__speed", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__ripples_scale", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__ripples_speed", float)
    , ECS_RO_COMP(aurora_borealis_es_comps, "aurora_borealis__ripples_strength", float)
    , ECS_RW_COMP(aurora_borealis_es_comps, "aurora_borealis__init", dafg::NodeHandle)
    , ECS_RW_COMP(aurora_borealis_es_comps, "aurora_borealis__render", dafg::NodeHandle)
    , ECS_RW_COMP(aurora_borealis_es_comps, "aurora_borealis__apply", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc aurora_borealis_es_es_desc
(
  "aurora_borealis_es",
  "prog/daNetGame/render/fx/auroraBorealisES.cpp.inl",
  ecs::EntitySystemOps(nullptr, aurora_borealis_es_all_events),
  make_span(aurora_borealis_es_comps+0, 4)/*rw*/,
  make_span(aurora_borealis_es_comps+4, 13)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
