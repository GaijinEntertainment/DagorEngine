#include "rainRipplesES.cpp.inl"
ECS_DEF_PULL_VAR(rainRipples);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc rain_ripples_appear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("rain_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void rain_ripples_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  rain_ripples_appear_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc rain_ripples_appear_es_es_desc
(
  "rain_ripples_appear_es",
  "prog/daNetGame/render/weather/rainRipplesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rain_ripples_appear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(rain_ripples_appear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc rain_ripples_disappear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("rain_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void rain_ripples_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  rain_ripples_disappear_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc rain_ripples_disappear_es_es_desc
(
  "rain_ripples_disappear_es",
  "prog/daNetGame/render/weather/rainRipplesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rain_ripples_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(rain_ripples_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc rain_ripples_toggle_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("render_settings__fftWaterQuality"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("render_settings__fullDeferred"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__rain_ripples_enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()}
};
static void rain_ripples_toggle_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    rain_ripples_toggle_es(evt
        , ECS_RO_COMP(rain_ripples_toggle_es_comps, "render_settings__fftWaterQuality", ecs::string)
    , ECS_RO_COMP(rain_ripples_toggle_es_comps, "render_settings__fullDeferred", bool)
    , ECS_RO_COMP(rain_ripples_toggle_es_comps, "render_settings__rain_ripples_enabled", bool)
    , ECS_RO_COMP(rain_ripples_toggle_es_comps, "render_settings__bare_minimum", bool)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc rain_ripples_toggle_es_es_desc
(
  "rain_ripples_toggle_es",
  "prog/daNetGame/render/weather/rainRipplesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rain_ripples_toggle_es_all_events),
  empty_span(),
  make_span(rain_ripples_toggle_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRainToggle>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc render_rain_ripples_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("zn_zfar"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("rain_ripples_shader"), ecs::ComponentTypeInfo<PostFxRenderer>()}
};
static void render_rain_ripples_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    render_rain_ripples_es(evt
        , ECS_RO_COMP(render_rain_ripples_es_comps, "zn_zfar", ShaderVar)
    , ECS_RO_COMP(render_rain_ripples_es_comps, "rain_ripples_shader", PostFxRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc render_rain_ripples_es_es_desc
(
  "render_rain_ripples_es",
  "prog/daNetGame/render/weather/rainRipplesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, render_rain_ripples_es_all_events),
  empty_span(),
  make_span(render_rain_ripples_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderDecals>::build(),
  0
);
static constexpr ecs::ComponentDesc set_rain_ripples_size_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("rain_ripples__size"), ecs::ComponentTypeInfo<float>()}
};
static void set_rain_ripples_size_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_rain_ripples_size_es(evt
        , ECS_RO_COMP(set_rain_ripples_size_es_comps, "rain_ripples__size", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_rain_ripples_size_es_es_desc
(
  "set_rain_ripples_size_es",
  "prog/daNetGame/render/weather/rainRipplesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_rain_ripples_size_es_all_events),
  empty_span(),
  make_span(set_rain_ripples_size_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
