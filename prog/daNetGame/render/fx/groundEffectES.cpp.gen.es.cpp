// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "groundEffectES.cpp.inl"
ECS_DEF_PULL_VAR(groundEffect);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_ground_effects_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ground_effect__manager"), ecs::ComponentTypeInfo<GroundEffectManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void update_ground_effects_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    update_ground_effects_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(update_ground_effects_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(update_ground_effects_es_comps, "ground_effect__manager", GroundEffectManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_ground_effects_es_es_desc
(
  "update_ground_effects_es",
  "prog/daNetGame/render/fx/groundEffectES.cpp.inl",
  ecs::EntitySystemOps(update_ground_effects_es_all),
  make_span(update_ground_effects_es_comps+0, 1)/*rw*/,
  make_span(update_ground_effects_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"camera_animator_update_es");
static constexpr ecs::ComponentDesc update_ground_effect_params_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ground_effect__manager"), ecs::ComponentTypeInfo<GroundEffectManager>()},
//start of 18 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("ground_effect__fx_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ground_effect__biome_group_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ground_effect__biome_ids"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("ground_effect__grid_cell_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ground_effect__grid_world_origin"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__vis_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ground_effect__biome_weight_for_active_thr"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ground_effect__fx_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ground_effect__rot_x"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__rot_y"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__rot_z"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__offset_x"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__offset_y"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__offset_z"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__scale_x"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__scale_y"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ground_effect__scale_z"), ecs::ComponentTypeInfo<Point2>()}
};
static void update_ground_effect_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_ground_effect_params_es_event_handler(evt
        , components.manager()
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__manager", GroundEffectManager)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__fx_name", ecs::string)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__biome_group_name", ecs::string)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__biome_ids", ecs::Array)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__grid_cell_size", float)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__grid_world_origin", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__vis_radius", float)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__biome_weight_for_active_thr", float)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__fx_radius", float)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__rot_x", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__rot_y", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__rot_z", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__offset_x", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__offset_y", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__offset_z", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__scale_x", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__scale_y", Point2)
    , ECS_RO_COMP(update_ground_effect_params_es_event_handler_comps, "ground_effect__scale_z", Point2)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_ground_effect_params_es_event_handler_es_desc
(
  "update_ground_effect_params_es",
  "prog/daNetGame/render/fx/groundEffectES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_ground_effect_params_es_event_handler_all_events),
  make_span(update_ground_effect_params_es_event_handler_comps+0, 1)/*rw*/,
  make_span(update_ground_effect_params_es_event_handler_comps+1, 18)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc ground_fx_stop_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()}
};
static ecs::CompileTimeQueryDesc ground_fx_stop_ecs_query_desc
(
  "ground_fx_stop_ecs_query",
  make_span(ground_fx_stop_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void ground_fx_stop_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, ground_fx_stop_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(ground_fx_stop_ecs_query_comps, "effect", TheEffect)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_camera_position_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc get_camera_position_ecs_query_desc
(
  "get_camera_position_ecs_query",
  empty_span(),
  make_span(get_camera_position_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_camera_position_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_camera_position_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_camera_position_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
