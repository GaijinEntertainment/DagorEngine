// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "beamTracerClientES.cpp.inl"
ECS_DEF_PULL_VAR(beamTracerClient);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc tac_laser_init_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("tac_laser_render__min_intensity_angle"), ecs::ComponentTypeInfo<float>()}
};
static void tac_laser_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  tac_laser_init_es(evt
        );
}
static ecs::EntitySystemDesc tac_laser_init_es_es_desc
(
  "tac_laser_init_es",
  "prog/gameLibs/render/beamTracer/render/beamTracerClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tac_laser_init_es_all_events),
  empty_span(),
  empty_span(),
  make_span(tac_laser_init_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render",nullptr,"tac_laser_update_render_settings_es");
static constexpr ecs::ComponentDesc tac_laser_close_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("tac_laser_render__min_intensity_angle"), ecs::ComponentTypeInfo<float>()}
};
static void tac_laser_close_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  tac_laser_close_es(evt
        );
}
static ecs::EntitySystemDesc tac_laser_close_es_es_desc
(
  "tac_laser_close_es",
  "prog/gameLibs/render/beamTracer/render/beamTracerClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tac_laser_close_es_all_events),
  empty_span(),
  empty_span(),
  make_span(tac_laser_close_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc tac_laser_update_render_settings_es_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("tac_laser_render__min_intensity_angle"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("tac_laser_render__min_intensity_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("tac_laser_render__max_intensity_angle"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("tac_laser_render__max_intensity_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("tac_laser_render__fog_intensity_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("tac_laser_render__fade_dist"), ecs::ComponentTypeInfo<float>()}
};
static void tac_laser_update_render_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tac_laser_update_render_settings_es(evt
        , ECS_RO_COMP(tac_laser_update_render_settings_es_comps, "tac_laser_render__min_intensity_angle", float)
    , ECS_RO_COMP(tac_laser_update_render_settings_es_comps, "tac_laser_render__min_intensity_mul", float)
    , ECS_RO_COMP(tac_laser_update_render_settings_es_comps, "tac_laser_render__max_intensity_angle", float)
    , ECS_RO_COMP(tac_laser_update_render_settings_es_comps, "tac_laser_render__max_intensity_mul", float)
    , ECS_RO_COMP(tac_laser_update_render_settings_es_comps, "tac_laser_render__fog_intensity_mul", float)
    , ECS_RO_COMP(tac_laser_update_render_settings_es_comps, "tac_laser_render__fade_dist", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tac_laser_update_render_settings_es_es_desc
(
  "tac_laser_update_render_settings_es",
  "prog/gameLibs/render/beamTracer/render/beamTracerClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tac_laser_update_render_settings_es_all_events),
  empty_span(),
  make_span(tac_laser_update_render_settings_es_comps+0, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc beam_tracer_projectile_init_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("beam_tracer_projectile"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void beam_tracer_projectile_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  beam_tracer_projectile_init_es(evt
        );
}
static ecs::EntitySystemDesc beam_tracer_projectile_init_es_es_desc
(
  "beam_tracer_projectile_init_es",
  "prog/gameLibs/render/beamTracer/render/beamTracerClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, beam_tracer_projectile_init_es_all_events),
  empty_span(),
  empty_span(),
  make_span(beam_tracer_projectile_init_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc beam_tracer_projectile_close_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("beam_tracer_projectile"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void beam_tracer_projectile_close_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  beam_tracer_projectile_close_es(evt
        );
}
static ecs::EntitySystemDesc beam_tracer_projectile_close_es_es_desc
(
  "beam_tracer_projectile_close_es",
  "prog/gameLibs/render/beamTracer/render/beamTracerClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, beam_tracer_projectile_close_es_all_events),
  empty_span(),
  empty_span(),
  make_span(beam_tracer_projectile_close_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
