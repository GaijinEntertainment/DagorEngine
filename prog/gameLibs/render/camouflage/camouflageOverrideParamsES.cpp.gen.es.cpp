// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "camouflageOverrideParamsES.cpp.inl"
ECS_DEF_PULL_VAR(camouflageOverrideParams);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dynamic_sheen_camo_override_params_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("camouflage_override"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void dynamic_sheen_camo_override_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_sheen_camo_override_params_es_event_handler(evt
        , ECS_RW_COMP(dynamic_sheen_camo_override_params_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(dynamic_sheen_camo_override_params_es_event_handler_comps, "camouflage_override", ecs::Array)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_sheen_camo_override_params_es_event_handler_es_desc
(
  "dynamic_sheen_camo_override_params_es",
  "prog/gameLibs/render/camouflage/camouflageOverrideParamsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_sheen_camo_override_params_es_event_handler_all_events),
  make_span(dynamic_sheen_camo_override_params_es_event_handler_comps+0, 1)/*rw*/,
  make_span(dynamic_sheen_camo_override_params_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","camouflage_override");
