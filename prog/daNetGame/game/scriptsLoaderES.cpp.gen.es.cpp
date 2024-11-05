#include "scriptsLoaderES.cpp.inl"
ECS_DEF_PULL_VAR(scriptsLoader);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc game_mode_scripts_loader_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_mode_loader__dasScript"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void game_mode_scripts_loader_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    game_mode_scripts_loader_es_event_handler(evt
        , ECS_RW_COMP(game_mode_scripts_loader_es_event_handler_comps, "game_mode_loader__dasScript", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc game_mode_scripts_loader_es_event_handler_es_desc
(
  "game_mode_scripts_loader_es",
  "prog/daNetGame/game/scriptsLoaderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, game_mode_scripts_loader_es_event_handler_all_events),
  make_span(game_mode_scripts_loader_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc game_mode_scripts_unloader_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("game_mode_loader__dasScript"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void game_mode_scripts_unloader_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  game_mode_scripts_unloader_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc game_mode_scripts_unloader_es_event_handler_es_desc
(
  "game_mode_scripts_unloader_es",
  "prog/daNetGame/game/scriptsLoaderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, game_mode_scripts_unloader_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(game_mode_scripts_unloader_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
