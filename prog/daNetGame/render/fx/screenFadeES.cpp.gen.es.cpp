// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "screenFadeES.cpp.inl"
ECS_DEF_PULL_VAR(screenFade);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc screen_fade_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("screen_fade"), ecs::ComponentTypeInfo<float>()}
};
static void screen_fade_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    screen_fade_es_event_handler(evt
        , ECS_RO_COMP(screen_fade_es_event_handler_comps, "screen_fade", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc screen_fade_es_event_handler_es_desc
(
  "screen_fade_es",
  "prog/daNetGame/render/fx/screenFadeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_fade_es_event_handler_all_events),
  empty_span(),
  make_span(screen_fade_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,"screen_fade");
