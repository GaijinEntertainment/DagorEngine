// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gtaoSettingsES.cpp.inl"
ECS_DEF_PULL_VAR(gtaoSettings);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gtao_screen_radius_settings_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__gtao_screen_radius"), ecs::ComponentTypeInfo<float>()}
};
static void gtao_screen_radius_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gtao_screen_radius_settings_es(evt
        , ECS_RO_COMP(gtao_screen_radius_settings_es_comps, "render_settings__gtao_screen_radius", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gtao_screen_radius_settings_es_es_desc
(
  "gtao_screen_radius_settings_es",
  "prog/daNetGame/render/world/gtaoSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gtao_screen_radius_settings_es_all_events),
  empty_span(),
  make_span(gtao_screen_radius_settings_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__gtao_screen_radius");
