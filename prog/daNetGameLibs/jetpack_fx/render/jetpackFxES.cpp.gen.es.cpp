// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "jetpackFxES.cpp.inl"
ECS_DEF_PULL_VAR(jetpackFx);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc jetpack_fx_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jetpack_exhaust"), ecs::ComponentTypeInfo<JetpackExhaust>()},
//start of 4 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("jetpack__active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("jetpack_exhaust__modAngles"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("jetpack_exhaust__nodeOffset"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL}
};
static void jetpack_fx_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    jetpack_fx_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(jetpack_fx_es_comps, "jetpack_exhaust", JetpackExhaust)
    , ECS_RO_COMP(jetpack_fx_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(jetpack_fx_es_comps, "jetpack__active", bool)
    , ECS_RO_COMP_OR(jetpack_fx_es_comps, "jetpack_exhaust__modAngles", Point3(Point3(0.f, 0.f, 0.f)))
    , ECS_RO_COMP_OR(jetpack_fx_es_comps, "jetpack_exhaust__nodeOffset", Point3(Point3(0.f, 0.f, 0.f)))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc jetpack_fx_es_es_desc
(
  "jetpack_fx_es",
  "prog/daNetGameLibs/jetpack_fx/render/jetpackFxES.cpp.inl",
  ecs::EntitySystemOps(jetpack_fx_es_all),
  make_span(jetpack_fx_es_comps+0, 1)/*rw*/,
  make_span(jetpack_fx_es_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"remove_unowned_items");
static constexpr ecs::ComponentDesc clear_jetpack_fx_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jetpack_exhaust"), ecs::ComponentTypeInfo<JetpackExhaust>()}
};
static void clear_jetpack_fx_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    clear_jetpack_fx_es_event_handler(evt
        , ECS_RW_COMP(clear_jetpack_fx_es_event_handler_comps, "jetpack_exhaust", JetpackExhaust)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc clear_jetpack_fx_es_event_handler_es_desc
(
  "clear_jetpack_fx_es",
  "prog/daNetGameLibs/jetpack_fx/render/jetpackFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clear_jetpack_fx_es_event_handler_all_events),
  make_span(clear_jetpack_fx_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
