#include "underwaterPostfxES.cpp.inl"
ECS_DEF_PULL_VAR(underwaterPostfx);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc underwater_update_shadervars_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("underwater_postfx__vignette_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("underwater_postfx__chromatic_aberration"), ecs::ComponentTypeInfo<Point3>()}
};
static void underwater_update_shadervars_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    underwater_update_shadervars_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(underwater_update_shadervars_es_comps, "underwater_postfx__vignette_strength", float)
    , ECS_RO_COMP(underwater_update_shadervars_es_comps, "underwater_postfx__chromatic_aberration", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc underwater_update_shadervars_es_es_desc
(
  "underwater_update_shadervars_es",
  "prog/daNetGameLibs/underwater_postfx/render/underwaterPostfxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, underwater_update_shadervars_es_all_events),
  empty_span(),
  make_span(underwater_update_shadervars_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"update_water_level_values_es");
