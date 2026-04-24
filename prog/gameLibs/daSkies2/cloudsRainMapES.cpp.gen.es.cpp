// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cloudsRainMapES.cpp.inl"
ECS_DEF_PULL_VAR(cloudsRainMap);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_cloud_rain_map_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("clouds_rain_map"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("clouds_rain_map_texel_size"), ecs::ComponentTypeInfo<float>()}
};
static void init_cloud_rain_map_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_cloud_rain_map_es(evt
        , components.manager()
    , ECS_RO_COMP(init_cloud_rain_map_es_comps, "clouds_rain_map", bool)
    , ECS_RO_COMP(init_cloud_rain_map_es_comps, "clouds_rain_map_texel_size", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_cloud_rain_map_es_es_desc
(
  "init_cloud_rain_map_es",
  "prog/gameLibs/daSkies2/cloudsRainMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_cloud_rain_map_es_all_events),
  empty_span(),
  make_span(init_cloud_rain_map_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc destroy_cloud_rain_map_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("clouds_rain_map"), ecs::ComponentTypeInfo<bool>()}
};
static void destroy_cloud_rain_map_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  destroy_cloud_rain_map_es(evt
        );
}
static ecs::EntitySystemDesc destroy_cloud_rain_map_es_es_desc
(
  "destroy_cloud_rain_map_es",
  "prog/gameLibs/daSkies2/cloudsRainMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_cloud_rain_map_es_all_events),
  empty_span(),
  empty_span(),
  make_span(destroy_cloud_rain_map_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc on_volfog_setting_change_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__volumeFogQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void on_volfog_setting_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_volfog_setting_change_es(evt
        , ECS_RO_COMP(on_volfog_setting_change_es_comps, "render_settings__volumeFogQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_volfog_setting_change_es_es_desc
(
  "on_volfog_setting_change_es",
  "prog/gameLibs/daSkies2/cloudsRainMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_volfog_setting_change_es_all_events),
  empty_span(),
  make_span(on_volfog_setting_change_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","render_settings__volumeFogQuality");
static constexpr ecs::ComponentDesc volfog_quality_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__volumeFogQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc volfog_quality_ecs_query_desc
(
  "volfog_quality_ecs_query",
  empty_span(),
  make_span(volfog_quality_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void volfog_quality_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volfog_quality_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volfog_quality_ecs_query_comps, "render_settings__volumeFogQuality", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
