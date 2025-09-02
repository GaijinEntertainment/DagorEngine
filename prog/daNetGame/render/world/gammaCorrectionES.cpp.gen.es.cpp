// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gammaCorrectionES.cpp.inl"
ECS_DEF_PULL_VAR(gammaCorrection);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gamma_correction_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__is_hdr_enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__gamma_correction"), ecs::ComponentTypeInfo<float>()}
};
static void gamma_correction_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gamma_correction_es(evt
        , ECS_RO_COMP(gamma_correction_es_comps, "render_settings__is_hdr_enabled", bool)
    , ECS_RO_COMP(gamma_correction_es_comps, "render_settings__gamma_correction", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gamma_correction_es_es_desc
(
  "gamma_correction_es",
  "prog/daNetGame/render/world/gammaCorrectionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gamma_correction_es_all_events),
  empty_span(),
  make_span(gamma_correction_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,nullptr,"render_settings__gamma_correction,render_settings__is_hdr_enabled");
