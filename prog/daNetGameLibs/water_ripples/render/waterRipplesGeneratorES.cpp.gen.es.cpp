// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterRipplesGeneratorES.cpp.inl"
ECS_DEF_PULL_VAR(waterRipplesGenerator);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc attempt_to_enable_water_ripples_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("render_settings__fftWaterQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void attempt_to_enable_water_ripples_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  attempt_to_enable_water_ripples_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc attempt_to_enable_water_ripples_es_es_desc
(
  "attempt_to_enable_water_ripples_es",
  "prog/daNetGameLibs/water_ripples/render/waterRipplesGeneratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attempt_to_enable_water_ripples_es_all_events),
  empty_span(),
  empty_span(),
  make_span(attempt_to_enable_water_ripples_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","render_settings__fftWaterQuality");
static constexpr ecs::ComponentDesc set_ripples_settings_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("water_ripples_drop_force"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("water_ripples_back_force"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("water_ripples_damping"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("water_ripples_distortion_params_3d"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("water_ripples_distortion_params_puddles"), ecs::ComponentTypeInfo<Point4>()}
};
static void set_ripples_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_ripples_settings_es(evt
        , ECS_RW_COMP(set_ripples_settings_es_comps, "water_ripples_drop_force", Point4)
    , ECS_RW_COMP(set_ripples_settings_es_comps, "water_ripples_back_force", Point4)
    , ECS_RW_COMP(set_ripples_settings_es_comps, "water_ripples_damping", Point4)
    , ECS_RW_COMP(set_ripples_settings_es_comps, "water_ripples_distortion_params_3d", Point4)
    , ECS_RW_COMP(set_ripples_settings_es_comps, "water_ripples_distortion_params_puddles", Point4)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_ripples_settings_es_es_desc
(
  "set_ripples_settings_es",
  "prog/daNetGameLibs/water_ripples/render/waterRipplesGeneratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_ripples_settings_es_all_events),
  make_span(set_ripples_settings_es_comps+0, 5)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","water_ripples_back_force,water_ripples_damping,water_ripples_distortion_params_3d,water_ripples_distortion_params_puddles,water_ripples_drop_force");
static constexpr ecs::ComponentDesc disable_water_ripples_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void disable_water_ripples_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  disable_water_ripples_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc disable_water_ripples_es_es_desc
(
  "disable_water_ripples_es",
  "prog/daNetGameLibs/water_ripples/render/waterRipplesGeneratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_water_ripples_es_all_events),
  empty_span(),
  empty_span(),
  make_span(disable_water_ripples_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc reset_water_ripples_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_ripples"), ecs::ComponentTypeInfo<WaterRipples>()}
};
static void reset_water_ripples_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reset_water_ripples_es(evt
        , ECS_RW_COMP(reset_water_ripples_es_comps, "water_ripples", WaterRipples)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reset_water_ripples_es_es_desc
(
  "reset_water_ripples_es",
  "prog/daNetGameLibs/water_ripples/render/waterRipplesGeneratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_water_ripples_es_all_events),
  make_span(reset_water_ripples_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_water_ripples_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("water_ripples"), ecs::ComponentTypeInfo<WaterRipples>()},
  {ECS_HASH("water_ripples__origin_bbox"), ecs::ComponentTypeInfo<BBox3>()}
};
static void update_water_ripples_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_water_ripples_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RW_COMP(update_water_ripples_es_comps, "water_ripples", WaterRipples)
    , ECS_RW_COMP(update_water_ripples_es_comps, "water_ripples__origin_bbox", BBox3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_water_ripples_es_es_desc
(
  "update_water_ripples_es",
  "prog/daNetGameLibs/water_ripples/render/waterRipplesGeneratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_water_ripples_es_all_events),
  make_span(update_water_ripples_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc get_fft_water_quality_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__fftWaterQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc get_fft_water_quality_ecs_query_desc
(
  "get_fft_water_quality_ecs_query",
  empty_span(),
  make_span(get_fft_water_quality_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_fft_water_quality_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_fft_water_quality_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_fft_water_quality_ecs_query_comps, "render_settings__fftWaterQuality", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_water_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static ecs::CompileTimeQueryDesc get_water_ecs_query_desc
(
  "get_water_ecs_query",
  make_span(get_water_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_water_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  return perform_query(&manager, eid, get_water_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_water_ecs_query_comps, "water", FFTWater)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc effect_ripple_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("water_ripple__requireStart"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("water_ripple__mass"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water_ripple__radius"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc effect_ripple_ecs_query_desc
(
  "effect_ripple_ecs_query",
  make_span(effect_ripple_ecs_query_comps+0, 3)/*rw*/,
  make_span(effect_ripple_ecs_query_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void effect_ripple_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, effect_ripple_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(effect_ripple_ecs_query_comps, "transform", TMatrix)
            , ECS_RW_COMP(effect_ripple_ecs_query_comps, "water_ripple__requireStart", bool)
            , ECS_RW_COMP(effect_ripple_ecs_query_comps, "water_ripple__mass", float)
            , ECS_RW_COMP(effect_ripple_ecs_query_comps, "water_ripple__radius", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_water_ripples_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_ripples"), ecs::ComponentTypeInfo<WaterRipples>()},
//start of 1 ro components at [1]
  {ECS_HASH("water_ripples__origin_bbox"), ecs::ComponentTypeInfo<BBox3>()}
};
static ecs::CompileTimeQueryDesc get_water_ripples_ecs_query_desc
(
  "get_water_ripples_ecs_query",
  make_span(get_water_ripples_ecs_query_comps+0, 1)/*rw*/,
  make_span(get_water_ripples_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_water_ripples_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  return perform_query(&manager, eid, get_water_ripples_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_water_ripples_ecs_query_comps, "water_ripples__origin_bbox", BBox3)
            , ECS_RW_COMP(get_water_ripples_ecs_query_comps, "water_ripples", WaterRipples)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc set_fft_water_quality_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("render_settings__fftWaterQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc set_fft_water_quality_ecs_query_desc
(
  "set_fft_water_quality_ecs_query",
  make_span(set_fft_water_quality_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_fft_water_quality_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, set_fft_water_quality_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_fft_water_quality_ecs_query_comps, "render_settings__fftWaterQuality", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
