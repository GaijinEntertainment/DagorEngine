// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "treeWindAnimFadeES.cpp.inl"
ECS_DEF_PULL_VAR(treeWindAnimFade);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc tree_wind_anim_fade_settings_es_comps[] ={};
static void tree_wind_anim_fade_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  tree_wind_anim_fade_settings_es(evt
        );
}
static ecs::EntitySystemDesc tree_wind_anim_fade_settings_es_es_desc
(
  "tree_wind_anim_fade_settings_es",
  "prog/daNetGame/render/treeWindAnimFadeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_wind_anim_fade_settings_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc tree_wind_anim_fade_convar_es_comps[] ={};
static void tree_wind_anim_fade_convar_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  tree_wind_anim_fade_convar_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc tree_wind_anim_fade_convar_es_es_desc
(
  "tree_wind_anim_fade_convar_es",
  "prog/daNetGame/render/treeWindAnimFadeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_wind_anim_fade_convar_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
