// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lightsES.cpp.inl"
ECS_DEF_PULL_VAR(lights);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc shadows_quality_changed_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("render_settings__shadowsQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void shadows_quality_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  shadows_quality_changed_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc shadows_quality_changed_es_es_desc
(
  "shadows_quality_changed_es",
  "prog/daNetGame/render/fx/lightsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, shadows_quality_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(shadows_quality_changed_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__shadowsQuality",nullptr,"init_shadows_es");
//static constexpr ecs::ComponentDesc render_features_changed_es_comps[] ={};
static void render_features_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  render_features_changed_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc render_features_changed_es_es_desc
(
  "render_features_changed_es",
  "prog/daNetGame/render/fx/lightsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, render_features_changed_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel,
                       ChangeRenderFeatures>::build(),
  0
);
//static constexpr ecs::ComponentDesc update_high_priority_lights_es_comps[] ={};
static void update_high_priority_lights_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnGameUpdateAfterGameLogic>());
  update_high_priority_lights_es(static_cast<const EventOnGameUpdateAfterGameLogic&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc update_high_priority_lights_es_es_desc
(
  "update_high_priority_lights_es",
  "prog/daNetGame/render/fx/lightsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_high_priority_lights_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameUpdateAfterGameLogic>::build(),
  0
);
