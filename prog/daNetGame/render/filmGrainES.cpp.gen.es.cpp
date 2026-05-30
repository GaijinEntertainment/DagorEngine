// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "filmGrainES.cpp.inl"
ECS_DEF_PULL_VAR(filmGrain);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc film_grain_lut_after_device_reset_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("film_grain_lut"), ecs::ComponentTypeInfo<FilmGrainLutHolder>()}
};
static void film_grain_lut_after_device_reset_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    film_grain_lut_after_device_reset_es_event_handler(evt
        , ECS_RW_COMP(film_grain_lut_after_device_reset_es_event_handler_comps, "film_grain_lut", FilmGrainLutHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc film_grain_lut_after_device_reset_es_event_handler_es_desc
(
  "film_grain_lut_after_device_reset_es",
  "prog/daNetGame/render/filmGrainES.cpp.inl",
  ecs::EntitySystemOps(nullptr, film_grain_lut_after_device_reset_es_event_handler_all_events),
  make_span(film_grain_lut_after_device_reset_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc film_grain_lut_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("film_grain_lut"), ecs::ComponentTypeInfo<FilmGrainLutHolder>()},
//start of 3 ro components at [1]
  {ECS_HASH("film_grain_lut__wh"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("film_grain_lut__d"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("film_grain_lut__gen_params"), ecs::ComponentTypeInfo<Point4>(), ecs::CDF_OPTIONAL}
};
static void film_grain_lut_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    film_grain_lut_init_es_event_handler(evt
        , ECS_RW_COMP(film_grain_lut_init_es_event_handler_comps, "film_grain_lut", FilmGrainLutHolder)
    , ECS_RO_COMP_OR(film_grain_lut_init_es_event_handler_comps, "film_grain_lut__wh", int(256))
    , ECS_RO_COMP_OR(film_grain_lut_init_es_event_handler_comps, "film_grain_lut__d", int(64))
    , ECS_RO_COMP_OR(film_grain_lut_init_es_event_handler_comps, "film_grain_lut__gen_params", Point4(Point4(0, 0, 0, 0)))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc film_grain_lut_init_es_event_handler_es_desc
(
  "film_grain_lut_init_es",
  "prog/daNetGame/render/filmGrainES.cpp.inl",
  ecs::EntitySystemOps(nullptr, film_grain_lut_init_es_event_handler_all_events),
  make_span(film_grain_lut_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(film_grain_lut_init_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsUpdated,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","film_grain_lut__d,film_grain_lut__gen_params,film_grain_lut__wh");
static constexpr ecs::ComponentDesc film_grain_lut_generate_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("film_grain_lut"), ecs::ComponentTypeInfo<FilmGrainLutHolder>()}
};
static void film_grain_lut_generate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    film_grain_lut_generate_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(film_grain_lut_generate_es_comps, "film_grain_lut", FilmGrainLutHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc film_grain_lut_generate_es_es_desc
(
  "film_grain_lut_generate_es",
  "prog/daNetGame/render/filmGrainES.cpp.inl",
  ecs::EntitySystemOps(nullptr, film_grain_lut_generate_es_all_events),
  make_span(film_grain_lut_generate_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc film_grain_lut_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("film_grain_lut"), ecs::ComponentTypeInfo<FilmGrainLutHolder>()},
//start of 3 ro components at [1]
  {ECS_HASH("film_grain_lut__wh"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("film_grain_lut__d"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("film_grain_lut__gen_params"), ecs::ComponentTypeInfo<Point4>()}
};
static ecs::CompileTimeQueryDesc film_grain_lut_ecs_query_desc
(
  "film_grain_lut_ecs_query",
  make_span(film_grain_lut_ecs_query_comps+0, 1)/*rw*/,
  make_span(film_grain_lut_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void film_grain_lut_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, film_grain_lut_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(film_grain_lut_ecs_query_comps, "film_grain_lut", FilmGrainLutHolder)
            , ECS_RO_COMP(film_grain_lut_ecs_query_comps, "film_grain_lut__wh", int)
            , ECS_RO_COMP(film_grain_lut_ecs_query_comps, "film_grain_lut__d", int)
            , ECS_RO_COMP(film_grain_lut_ecs_query_comps, "film_grain_lut__gen_params", Point4)
            );

        }while (++comp != compE);
    }
  );
}
