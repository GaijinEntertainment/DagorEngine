// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "worldRendererGIES.cpp.inl"
ECS_DEF_PULL_VAR(worldRendererGI);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc wr_handle_on_hero_teleport_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("hero"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void wr_handle_on_hero_teleport_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  wr_handle_on_hero_teleport_es(evt
        );
}
static ecs::EntitySystemDesc wr_handle_on_hero_teleport_es_es_desc
(
  "wr_handle_on_hero_teleport_es",
  "prog/daNetGame/render/world/worldRendererGIES.cpp.inl",
  ecs::EntitySystemOps(nullptr, wr_handle_on_hero_teleport_es_all_events),
  empty_span(),
  empty_span(),
  make_span(wr_handle_on_hero_teleport_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventOnEntityTeleported>::build(),
  0
,"render",nullptr,"*");
