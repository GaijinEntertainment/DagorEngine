#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_get_type();
static ecs::LTComponentList animchar_component(ECS_HASH("animchar"), animchar_get_type(), "prog/gameLibs/ecs/game/actions/actionES.cpp.inl", "", 0);
static constexpr ecs::component_t phys_vars_get_type();
static ecs::LTComponentList phys_vars_component(ECS_HASH("phys_vars"), phys_vars_get_type(), "prog/gameLibs/ecs/game/actions/actionES.cpp.inl", "", 0);
#include "actionES.cpp.inl"
ECS_DEF_PULL_VAR(action);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc actions_updater_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("actions"), ecs::ComponentTypeInfo<EntityActions>()},
  {ECS_HASH("phys_vars"), ecs::ComponentTypeInfo<PhysVars>()},
//start of 2 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("actions__animLayer"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static void actions_updater_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateActionsEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    actions_updater_es(static_cast<const UpdateActionsEvent&>(evt)
        , ECS_RO_COMP(actions_updater_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(actions_updater_es_comps, "actions", EntityActions)
    , ECS_RW_COMP(actions_updater_es_comps, "phys_vars", PhysVars)
    , ECS_RO_COMP_PTR(actions_updater_es_comps, "actions__animLayer", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc actions_updater_es_es_desc
(
  "actions_updater_es",
  "prog/gameLibs/ecs/game/actions/actionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, actions_updater_es_all_events),
  make_span(actions_updater_es_comps+0, 2)/*rw*/,
  make_span(actions_updater_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateActionsEvent>::build(),
  0
);
static constexpr ecs::component_t animchar_get_type(){return ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>::type; }
static constexpr ecs::component_t phys_vars_get_type(){return ecs::ComponentTypeInfo<PhysVars>::type; }
