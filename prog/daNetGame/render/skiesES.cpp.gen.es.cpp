// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "skiesES.cpp.inl"
ECS_DEF_PULL_VAR(skies);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc force_panoramic_sky_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("clouds_settings__force_panorama"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void force_panoramic_sky_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_panoramic_sky_es_event_handler(evt
        , ECS_RO_COMP(force_panoramic_sky_es_event_handler_comps, "clouds_settings__force_panorama", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_panoramic_sky_es_event_handler_es_desc
(
  "force_panoramic_sky_es",
  "prog/daNetGame/render/skiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_panoramic_sky_es_event_handler_all_events),
  empty_span(),
  make_span(force_panoramic_sky_es_event_handler_comps+0, 1)/*ro*/,
  make_span(force_panoramic_sky_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventSkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*","clouds_form_es,clouds_settings_es");
static constexpr ecs::ComponentDesc sky_enable_black_sky_rendering_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("sky_disable_sky_influence"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void sky_enable_black_sky_rendering_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    sky_enable_black_sky_rendering_es(evt
        , ECS_RO_COMP_OR(sky_enable_black_sky_rendering_es_comps, "sky_disable_sky_influence", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sky_enable_black_sky_rendering_es_es_desc
(
  "sky_enable_black_sky_rendering_es",
  "prog/daNetGame/render/skiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sky_enable_black_sky_rendering_es_all_events),
  empty_span(),
  make_span(sky_enable_black_sky_rendering_es_comps+0, 1)/*ro*/,
  make_span(sky_enable_black_sky_rendering_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventSkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc sky_settings_altitude_ofs_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("sky_coord_frame__altitude_offset"), ecs::ComponentTypeInfo<float>()}
};
static void sky_settings_altitude_ofs_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    sky_settings_altitude_ofs_es_event_handler(evt
        , ECS_RO_COMP(sky_settings_altitude_ofs_es_event_handler_comps, "sky_coord_frame__altitude_offset", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sky_settings_altitude_ofs_es_event_handler_es_desc
(
  "sky_settings_altitude_ofs_es",
  "prog/daNetGame/render/skiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sky_settings_altitude_ofs_es_event_handler_all_events),
  empty_span(),
  make_span(sky_settings_altitude_ofs_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventSkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"sky_coord_frame__altitude_offset");
//static constexpr ecs::ComponentDesc invalidate_skies_es_comps[] ={};
static void invalidate_skies_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<CmdSkiesInvalidate>());
  invalidate_skies_es(static_cast<const CmdSkiesInvalidate&>(evt)
        );
}
static ecs::EntitySystemDesc invalidate_skies_es_es_desc
(
  "invalidate_skies_es",
  "prog/daNetGame/render/skiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_skies_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdSkiesInvalidate>::build(),
  0
);
