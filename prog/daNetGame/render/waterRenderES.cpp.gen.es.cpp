// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterRenderES.cpp.inl"
ECS_DEF_PULL_VAR(waterRender);
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
static constexpr ecs::ComponentDesc retrieve_dof_entity_for_underwater_dof_ecs_query_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("dof__on"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dof__is_filmic"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dof__nearDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__nearDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__nearDofAmountPercent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofAmountPercent"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc retrieve_dof_entity_for_underwater_dof_ecs_query_desc
(
  "retrieve_dof_entity_for_underwater_dof_ecs_query",
  make_span(retrieve_dof_entity_for_underwater_dof_ecs_query_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void retrieve_dof_entity_for_underwater_dof_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, retrieve_dof_entity_for_underwater_dof_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__on", bool)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__is_filmic", bool)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__nearDofStart", float)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__nearDofEnd", float)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__nearDofAmountPercent", float)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__farDofStart", float)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__farDofEnd", float)
            , ECS_RW_COMP(retrieve_dof_entity_for_underwater_dof_ecs_query_comps, "dof__farDofAmountPercent", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc toggle_underwater_dof_ecs_query_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("savedDof__on"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("savedDof__is_filmic"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("savedDof__nearDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("savedDof__nearDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("savedDof__nearDofAmountPercent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("savedDof__farDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("savedDof__farDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("savedDof__farDofAmountPercent"), ecs::ComponentTypeInfo<float>()},
//start of 6 ro components at [8]
  {ECS_HASH("underwaterDof__nearDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("underwaterDof__nearDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("underwaterDof__nearDofAmountPercent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("underwaterDof__farDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("underwaterDof__farDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("underwaterDof__farDofAmountPercent"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc toggle_underwater_dof_ecs_query_desc
(
  "toggle_underwater_dof_ecs_query",
  make_span(toggle_underwater_dof_ecs_query_comps+0, 8)/*rw*/,
  make_span(toggle_underwater_dof_ecs_query_comps+8, 6)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void toggle_underwater_dof_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, toggle_underwater_dof_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__on", bool)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__is_filmic", bool)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__nearDofStart", float)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__nearDofEnd", float)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__nearDofAmountPercent", float)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__farDofStart", float)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__farDofEnd", float)
            , ECS_RW_COMP(toggle_underwater_dof_ecs_query_comps, "savedDof__farDofAmountPercent", float)
            , ECS_RO_COMP(toggle_underwater_dof_ecs_query_comps, "underwaterDof__nearDofStart", float)
            , ECS_RO_COMP(toggle_underwater_dof_ecs_query_comps, "underwaterDof__nearDofEnd", float)
            , ECS_RO_COMP(toggle_underwater_dof_ecs_query_comps, "underwaterDof__nearDofAmountPercent", float)
            , ECS_RO_COMP(toggle_underwater_dof_ecs_query_comps, "underwaterDof__farDofStart", float)
            , ECS_RO_COMP(toggle_underwater_dof_ecs_query_comps, "underwaterDof__farDofEnd", float)
            , ECS_RO_COMP(toggle_underwater_dof_ecs_query_comps, "underwaterDof__farDofAmountPercent", float)
            );

        }while (++comp != compE);
    }
  );
}
