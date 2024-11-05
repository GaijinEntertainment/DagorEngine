#include "waterRenderES.cpp.inl"
ECS_DEF_PULL_VAR(waterRender);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_water_level_values_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()},
  {ECS_HASH("is_underwater"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("water_level"), ecs::ComponentTypeInfo<float>()}
};
static void update_water_level_values_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_water_level_values_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(update_water_level_values_es_comps, "water", FFTWater)
    , ECS_RW_COMP(update_water_level_values_es_comps, "is_underwater", bool)
    , ECS_RW_COMP(update_water_level_values_es_comps, "water_level", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_water_level_values_es_es_desc
(
  "update_water_level_values_es",
  "prog/daNetGame/render/waterRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_water_level_values_es_all_events),
  make_span(update_water_level_values_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc get_is_underwater_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("is_underwater"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_is_underwater_ecs_query_desc
(
  "get_is_underwater_ecs_query",
  empty_span(),
  make_span(get_is_underwater_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_is_underwater_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_is_underwater_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_is_underwater_ecs_query_comps, "is_underwater", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc is_water_hidden_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("water_hidden"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc is_water_hidden_ecs_query_desc
(
  "is_water_hidden_ecs_query",
  empty_span(),
  make_span(is_water_hidden_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_water_hidden_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_water_hidden_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_water_hidden_ecs_query_comps, "water_hidden", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_waterlevel_for_camera_pos_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("water_level"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_waterlevel_for_camera_pos_ecs_query_desc
(
  "get_waterlevel_for_camera_pos_ecs_query",
  empty_span(),
  make_span(get_waterlevel_for_camera_pos_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_waterlevel_for_camera_pos_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_waterlevel_for_camera_pos_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_waterlevel_for_camera_pos_ecs_query_comps, "water_level", float)
            );

        }while (++comp != compE);
    }
  );
}
