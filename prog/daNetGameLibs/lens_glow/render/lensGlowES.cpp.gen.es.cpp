// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lensGlowES.cpp.inl"
ECS_DEF_PULL_VAR(lensGlow);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc lens_glow_apply_settings_es_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lens_glow_config__bloom_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__bloom_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__lens_flare_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__lens_flare_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__texture_intensity_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__texture_tint"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("lens_glow_config__texture_variants"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void lens_glow_apply_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_glow_apply_settings_es(evt
        , components.manager()
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "enabled", bool)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__bloom_offset", float)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__bloom_multiplier", float)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__lens_flare_offset", float)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__lens_flare_multiplier", float)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__texture_intensity_multiplier", float)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__texture_tint", Point3)
    , ECS_RO_COMP(lens_glow_apply_settings_es_comps, "lens_glow_config__texture_variants", ecs::Array)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_glow_apply_settings_es_es_desc
(
  "lens_glow_apply_settings_es",
  "prog/daNetGameLibs/lens_glow/render/lensGlowES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_glow_apply_settings_es_all_events),
  empty_span(),
  make_span(lens_glow_apply_settings_es_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","enabled,lens_glow_config__bloom_multiplier,lens_glow_config__bloom_offset,lens_glow_config__lens_flare_multiplier,lens_glow_config__lens_flare_offset,lens_glow_config__texture_intensity_multiplier,lens_glow_config__texture_tint,lens_glow_config__texture_variants");
static constexpr ecs::ComponentDesc lens_glow_on_settings_changed_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__lensFlares"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()}
};
static void lens_glow_on_settings_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_glow_on_settings_changed_es(evt
        , components.manager()
    , ECS_RO_COMP(lens_glow_on_settings_changed_es_comps, "render_settings__lensFlares", bool)
    , ECS_RO_COMP(lens_glow_on_settings_changed_es_comps, "render_settings__bare_minimum", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_glow_on_settings_changed_es_es_desc
(
  "lens_glow_on_settings_changed_es",
  "prog/daNetGameLibs/lens_glow/render/lensGlowES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_glow_on_settings_changed_es_all_events),
  empty_span(),
  make_span(lens_glow_on_settings_changed_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__bare_minimum,render_settings__lensFlares");
static constexpr ecs::ComponentDesc lens_glow_disable_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("lens_glow_config__texture_variants"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void lens_glow_disable_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  lens_glow_disable_es(evt
        );
}
static ecs::EntitySystemDesc lens_glow_disable_es_es_desc
(
  "lens_glow_disable_es",
  "prog/daNetGameLibs/lens_glow/render/lensGlowES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_glow_disable_es_all_events),
  empty_span(),
  empty_span(),
  make_span(lens_glow_disable_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc lens_glow_enabled_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__lensFlares"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc lens_glow_enabled_ecs_query_desc
(
  "lens_glow_enabled_ecs_query",
  empty_span(),
  make_span(lens_glow_enabled_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void lens_glow_enabled_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, lens_glow_enabled_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(lens_glow_enabled_ecs_query_comps, "render_settings__lensFlares", bool)
            , ECS_RO_COMP(lens_glow_enabled_ecs_query_comps, "render_settings__bare_minimum", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc lens_glow_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lens_glow_config__bloom_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__bloom_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__lens_flare_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__lens_flare_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__texture_variants"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("lens_glow_config__texture_intensity_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_glow_config__texture_tint"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc lens_glow_ecs_query_desc
(
  "lens_glow_ecs_query",
  empty_span(),
  make_span(lens_glow_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void lens_glow_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, lens_glow_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(lens_glow_ecs_query_comps, "enabled", bool)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__bloom_offset", float)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__bloom_multiplier", float)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__lens_flare_offset", float)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__lens_flare_multiplier", float)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__texture_variants", ecs::Array)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__texture_intensity_multiplier", float)
            , ECS_RO_COMP(lens_glow_ecs_query_comps, "lens_glow_config__texture_tint", Point3)
            );

        }while (++comp != compE);
    }
  );
}
