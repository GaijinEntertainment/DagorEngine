#include "shaderVarToggleKeybindES.cpp.inl"
ECS_DEF_PULL_VAR(shaderVarToggleKeybind);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc toggle_shadervars_es_event_handler_comps[] ={};
static void toggle_shadervars_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventDebugKeyboardPressed>());
  toggle_shadervars_es_event_handler(static_cast<const EventDebugKeyboardPressed&>(evt)
        );
}
static ecs::EntitySystemDesc toggle_shadervars_es_event_handler_es_desc
(
  "toggle_shadervars_es",
  "prog/daNetGame/render/shaderVarToggleKeybindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, toggle_shadervars_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventDebugKeyboardPressed>::build(),
  0
,"dev,render");
