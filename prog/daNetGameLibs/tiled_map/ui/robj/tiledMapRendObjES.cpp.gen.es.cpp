// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "tiledMapRendObjES.cpp.inl"
ECS_DEF_PULL_VAR(tiledMapRendObj);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc register_tiled_map_rendobj_factories_es_event_handler_comps[] ={};
static void register_tiled_map_rendobj_factories_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventUiRegisterRendObjs>());
  register_tiled_map_rendobj_factories_es_event_handler(static_cast<const EventUiRegisterRendObjs&>(evt)
        );
}
static ecs::EntitySystemDesc register_tiled_map_rendobj_factories_es_event_handler_es_desc
(
  "register_tiled_map_rendobj_factories_es",
  "prog/daNetGameLibs/tiled_map/ui/robj/tiledMapRendObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_tiled_map_rendobj_factories_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventUiRegisterRendObjs>::build(),
  0
,"render");
