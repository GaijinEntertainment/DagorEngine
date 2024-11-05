#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t floatingRiGroup__density_get_type();
static ecs::LTComponentList floatingRiGroup__density_component(ECS_HASH("floatingRiGroup__density"), floatingRiGroup__density_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__densityRandRange_get_type();
static ecs::LTComponentList floatingRiGroup__densityRandRange_component(ECS_HASH("floatingRiGroup__densityRandRange"), floatingRiGroup__densityRandRange_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__elasticity_get_type();
static ecs::LTComponentList floatingRiGroup__elasticity_component(ECS_HASH("floatingRiGroup__elasticity"), floatingRiGroup__elasticity_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__inertiaMult_get_type();
static ecs::LTComponentList floatingRiGroup__inertiaMult_component(ECS_HASH("floatingRiGroup__inertiaMult"), floatingRiGroup__inertiaMult_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__interactionDistSq_get_type();
static ecs::LTComponentList floatingRiGroup__interactionDistSq_component(ECS_HASH("floatingRiGroup__interactionDistSq"), floatingRiGroup__interactionDistSq_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__interactionType_get_type();
static ecs::LTComponentList floatingRiGroup__interactionType_component(ECS_HASH("floatingRiGroup__interactionType"), floatingRiGroup__interactionType_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__maxShiftDist_get_type();
static ecs::LTComponentList floatingRiGroup__maxShiftDist_component(ECS_HASH("floatingRiGroup__maxShiftDist"), floatingRiGroup__maxShiftDist_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__minDistToGround_get_type();
static ecs::LTComponentList floatingRiGroup__minDistToGround_component(ECS_HASH("floatingRiGroup__minDistToGround"), floatingRiGroup__minDistToGround_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__physUpdateDt_get_type();
static ecs::LTComponentList floatingRiGroup__physUpdateDt_component(ECS_HASH("floatingRiGroup__physUpdateDt"), floatingRiGroup__physUpdateDt_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__resName_get_type();
static ecs::LTComponentList floatingRiGroup__resName_component(ECS_HASH("floatingRiGroup__resName"), floatingRiGroup__resName_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__updateDistSq_get_type();
static ecs::LTComponentList floatingRiGroup__updateDistSq_component(ECS_HASH("floatingRiGroup__updateDistSq"), floatingRiGroup__updateDistSq_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__useBoxInertia_get_type();
static ecs::LTComponentList floatingRiGroup__useBoxInertia_component(ECS_HASH("floatingRiGroup__useBoxInertia"), floatingRiGroup__useBoxInertia_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__viscosity_get_type();
static ecs::LTComponentList floatingRiGroup__viscosity_component(ECS_HASH("floatingRiGroup__viscosity"), floatingRiGroup__viscosity_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__volumePresetName_get_type();
static ecs::LTComponentList floatingRiGroup__volumePresetName_component(ECS_HASH("floatingRiGroup__volumePresetName"), floatingRiGroup__volumePresetName_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__volumesCount_get_type();
static ecs::LTComponentList floatingRiGroup__volumesCount_component(ECS_HASH("floatingRiGroup__volumesCount"), floatingRiGroup__volumesCount_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
static constexpr ecs::component_t floatingRiGroup__wreckageFloatDuration_get_type();
static ecs::LTComponentList floatingRiGroup__wreckageFloatDuration_component(ECS_HASH("floatingRiGroup__wreckageFloatDuration"), floatingRiGroup__wreckageFloatDuration_get_type(), "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl", "", 0);
#include "rendinstFloatingES.cpp.inl"
ECS_DEF_PULL_VAR(rendinstFloating);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc rendinst_floating_render_debug_es_comps[] ={};
static void rendinst_floating_render_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    rendinst_floating_render_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc rendinst_floating_render_debug_es_es_desc
(
  "rendinst_floating_render_debug_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(rendinst_floating_render_debug_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
//static constexpr ecs::ComponentDesc floating_phys_ripples_render_debug_es_comps[] ={};
static void floating_phys_ripples_render_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    floating_phys_ripples_render_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc floating_phys_ripples_render_debug_es_es_desc
(
  "floating_phys_ripples_render_debug_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(floating_phys_ripples_render_debug_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
//static constexpr ecs::ComponentDesc move_floating_rendinsts_es_comps[] ={};
static void move_floating_rendinsts_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventMoveRiEx>());
  move_floating_rendinsts_es(static_cast<const EventMoveRiEx&>(evt)
        );
}
static ecs::EntitySystemDesc move_floating_rendinsts_es_es_desc
(
  "move_floating_rendinsts_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, move_floating_rendinsts_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventMoveRiEx>::build(),
  0
,"gameClient");
static constexpr ecs::ComponentDesc update_floating_rendinsts_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("floatingRiSystem__randomWavesAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiSystem__randomWavesLength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiSystem__randomWavesPeriod"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiSystem__randomWavesVelocity"), ecs::ComponentTypeInfo<Point2>()}
};
static void update_floating_rendinsts_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_floating_rendinsts_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RO_COMP(update_floating_rendinsts_es_comps, "floatingRiSystem__randomWavesAmplitude", float)
    , ECS_RO_COMP(update_floating_rendinsts_es_comps, "floatingRiSystem__randomWavesLength", float)
    , ECS_RO_COMP(update_floating_rendinsts_es_comps, "floatingRiSystem__randomWavesPeriod", float)
    , ECS_RO_COMP(update_floating_rendinsts_es_comps, "floatingRiSystem__randomWavesVelocity", Point2)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_floating_rendinsts_es_es_desc
(
  "update_floating_rendinsts_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_floating_rendinsts_es_all_events),
  empty_span(),
  make_span(update_floating_rendinsts_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"gameClient");
//static constexpr ecs::ComponentDesc keep_floatable_destructables_es_comps[] ={};
static void keep_floatable_destructables_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  keep_floatable_destructables_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc keep_floatable_destructables_es_es_desc
(
  "keep_floatable_destructables_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, keep_floatable_destructables_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"gameClient",nullptr,"start_async_phys_sim_es");
static constexpr ecs::ComponentDesc init_floating_rendinst_res_group_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("floatingRiGroup__riPhysFloatingModel"), ecs::ComponentTypeInfo<rendinstfloating::PhysFloatingModel>()},
//start of 8 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("floatingRiGroup__resName"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("floatingRiGroup__inertiaMult"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("floatingRiGroup__volumesCount"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("floatingRiGroup__volumePresetName"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("floatingRiGroup__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__densityRandRange"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__useBoxInertia"), ecs::ComponentTypeInfo<bool>()}
};
static void init_floating_rendinst_res_group_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_floating_rendinst_res_group_es_event_handler(evt
        , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__resName", ecs::string)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__inertiaMult", Point3)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__volumesCount", int)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__volumePresetName", ecs::string)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__density", float)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__densityRandRange", float)
    , ECS_RO_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__useBoxInertia", bool)
    , ECS_RW_COMP(init_floating_rendinst_res_group_es_event_handler_comps, "floatingRiGroup__riPhysFloatingModel", rendinstfloating::PhysFloatingModel)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_floating_rendinst_res_group_es_event_handler_es_desc
(
  "init_floating_rendinst_res_group_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_floating_rendinst_res_group_es_event_handler_all_events),
  make_span(init_floating_rendinst_res_group_es_event_handler_comps+0, 1)/*rw*/,
  make_span(init_floating_rendinst_res_group_es_event_handler_comps+1, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"gameClient","floatingRiGroup__density,floatingRiGroup__densityRandRange,floatingRiGroup__inertiaMult,floatingRiGroup__resName,floatingRiGroup__useBoxInertia,floatingRiGroup__volumePresetName,floatingRiGroup__volumesCount");
static constexpr ecs::ComponentDesc init_floating_volume_presets_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("floatingRiSystem__volumePresets"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("floatingRiSystem__userVolumePresets"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void init_floating_volume_presets_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_floating_volume_presets_es_event_handler(evt
        , ECS_RW_COMP(init_floating_volume_presets_es_event_handler_comps, "floatingRiSystem__volumePresets", ecs::Object)
    , ECS_RW_COMP(init_floating_volume_presets_es_event_handler_comps, "floatingRiSystem__userVolumePresets", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_floating_volume_presets_es_event_handler_es_desc
(
  "init_floating_volume_presets_es",
  "prog/gameLibs/ecs/rendInst/./rendinstFloatingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_floating_volume_presets_es_event_handler_all_events),
  make_span(init_floating_volume_presets_es_event_handler_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"gameClient");
static constexpr ecs::ComponentDesc get_obstacles_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("deform_bbox__bmin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("deform_bbox__bmax"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc get_obstacles_ecs_query_desc
(
  "get_obstacles_ecs_query",
  empty_span(),
  make_span(get_obstacles_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_obstacles_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_obstacles_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_obstacles_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(get_obstacles_ecs_query_comps, "deform_bbox__bmin", Point3)
            , ECS_RO_COMP(get_obstacles_ecs_query_comps, "deform_bbox__bmax", Point3)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_camera_pos_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("camera_view"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc get_camera_pos_ecs_query_desc
(
  "get_camera_pos_ecs_query",
  empty_span(),
  make_span(get_camera_pos_ecs_query_comps+0, 2)/*ro*/,
  make_span(get_camera_pos_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void get_camera_pos_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_camera_pos_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(get_camera_pos_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              ECS_RO_COMP(get_camera_pos_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc find_floatable_group_for_res_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("floatingRiGroup__resName"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc find_floatable_group_for_res_ecs_query_desc
(
  "find_floatable_group_for_res_ecs_query",
  empty_span(),
  make_span(find_floatable_group_for_res_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void find_floatable_group_for_res_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, find_floatable_group_for_res_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(find_floatable_group_for_res_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(find_floatable_group_for_res_ecs_query_comps, "floatingRiGroup__resName", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc update_floating_rendinst_instances_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("floatingRiGroup__riPhysFloatingModel"), ecs::ComponentTypeInfo<rendinstfloating::PhysFloatingModel>()},
//start of 9 ro components at [1]
  {ECS_HASH("floatingRiGroup__wreckageFloatDuration"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__updateDistSq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__interactionType"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("floatingRiGroup__interactionDistSq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__elasticity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__physUpdateDt"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__maxShiftDist"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__viscosity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiGroup__minDistToGround"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc update_floating_rendinst_instances_ecs_query_desc
(
  "update_floating_rendinst_instances_ecs_query",
  make_span(update_floating_rendinst_instances_ecs_query_comps+0, 1)/*rw*/,
  make_span(update_floating_rendinst_instances_ecs_query_comps+1, 9)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_floating_rendinst_instances_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_floating_rendinst_instances_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__wreckageFloatDuration", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__updateDistSq", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__interactionType", int)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__interactionDistSq", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__elasticity", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__physUpdateDt", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__maxShiftDist", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__viscosity", float)
            , ECS_RO_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__minDistToGround", float)
            , ECS_RW_COMP(update_floating_rendinst_instances_ecs_query_comps, "floatingRiGroup__riPhysFloatingModel", rendinstfloating::PhysFloatingModel)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc construct_floating_volumes_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("floatingRiSystem__volumePresets"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc construct_floating_volumes_ecs_query_desc
(
  "construct_floating_volumes_ecs_query",
  empty_span(),
  make_span(construct_floating_volumes_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void construct_floating_volumes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, construct_floating_volumes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(construct_floating_volumes_ecs_query_comps, "floatingRiSystem__volumePresets", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc draw_rendinst_floating_volumes_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("floatingRiGroup__riPhysFloatingModel"), ecs::ComponentTypeInfo<rendinstfloating::PhysFloatingModel>()}
};
static ecs::CompileTimeQueryDesc draw_rendinst_floating_volumes_ecs_query_desc
(
  "draw_rendinst_floating_volumes_ecs_query",
  empty_span(),
  make_span(draw_rendinst_floating_volumes_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void draw_rendinst_floating_volumes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, draw_rendinst_floating_volumes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(draw_rendinst_floating_volumes_ecs_query_comps, "floatingRiGroup__riPhysFloatingModel", rendinstfloating::PhysFloatingModel)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc draw_floating_phys_ripples_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("floatingRiSystem__randomWavesAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiSystem__randomWavesLength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiSystem__randomWavesPeriod"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("floatingRiSystem__randomWavesVelocity"), ecs::ComponentTypeInfo<Point2>()}
};
static ecs::CompileTimeQueryDesc draw_floating_phys_ripples_ecs_query_desc
(
  "draw_floating_phys_ripples_ecs_query",
  empty_span(),
  make_span(draw_floating_phys_ripples_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void draw_floating_phys_ripples_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, draw_floating_phys_ripples_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(draw_floating_phys_ripples_ecs_query_comps, "floatingRiSystem__randomWavesAmplitude", float)
            , ECS_RO_COMP(draw_floating_phys_ripples_ecs_query_comps, "floatingRiSystem__randomWavesLength", float)
            , ECS_RO_COMP(draw_floating_phys_ripples_ecs_query_comps, "floatingRiSystem__randomWavesPeriod", float)
            , ECS_RO_COMP(draw_floating_phys_ripples_ecs_query_comps, "floatingRiSystem__randomWavesVelocity", Point2)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t floatingRiGroup__density_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__densityRandRange_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__elasticity_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__inertiaMult_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t floatingRiGroup__interactionDistSq_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__interactionType_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t floatingRiGroup__maxShiftDist_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__minDistToGround_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__physUpdateDt_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__resName_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
static constexpr ecs::component_t floatingRiGroup__updateDistSq_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__useBoxInertia_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t floatingRiGroup__viscosity_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t floatingRiGroup__volumePresetName_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
static constexpr ecs::component_t floatingRiGroup__volumesCount_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t floatingRiGroup__wreckageFloatDuration_get_type(){return ecs::ComponentTypeInfo<float>::type; }
