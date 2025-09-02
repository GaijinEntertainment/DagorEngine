#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t camera__active_get_type();
static ecs::LTComponentList camera__active_component(ECS_HASH("camera__active"), camera__active_get_type(), "prog/daNetGame/camera/sceneCamES.cpp.inl", "", 0);
static constexpr ecs::component_t fov_get_type();
static ecs::LTComponentList fov_component(ECS_HASH("fov"), fov_get_type(), "prog/daNetGame/camera/sceneCamES.cpp.inl", "", 0);
static constexpr ecs::component_t spectator__target_get_type();
static ecs::LTComponentList spectator__target_component(ECS_HASH("spectator__target"), spectator__target_get_type(), "prog/daNetGame/camera/sceneCamES.cpp.inl", "", 0);
static constexpr ecs::component_t team_get_type();
static ecs::LTComponentList team_component(ECS_HASH("team"), team_get_type(), "prog/daNetGame/camera/sceneCamES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/camera/sceneCamES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "sceneCamES.cpp.inl"
ECS_DEF_PULL_VAR(sceneCam);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc scene_cam_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camera__accuratePos"), ecs::ComponentTypeInfo<DPoint3>(), ecs::CDF_OPTIONAL},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static void scene_cam_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    scene_cam_es_event_handler(evt
        , ECS_RO_COMP(scene_cam_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(scene_cam_es_event_handler_comps, "camera__active", bool)
    , ECS_RO_COMP_PTR(scene_cam_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP_PTR(scene_cam_es_event_handler_comps, "camera__accuratePos", DPoint3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc scene_cam_es_event_handler_es_desc
(
  "scene_cam_es",
  "prog/daNetGame/camera/sceneCamES.cpp.inl",
  ecs::EntitySystemOps(nullptr, scene_cam_es_event_handler_all_events),
  make_span(scene_cam_es_event_handler_comps+0, 1)/*rw*/,
  make_span(scene_cam_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc scene_cam_activate_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camera__accuratePos"), ecs::ComponentTypeInfo<DPoint3>(), ecs::CDF_OPTIONAL},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void scene_cam_activate_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    scene_cam_activate_es_event_handler(evt
        , ECS_RO_COMP(scene_cam_activate_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP_PTR(scene_cam_activate_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(scene_cam_activate_es_event_handler_comps, "camera__active", bool)
    , ECS_RW_COMP_PTR(scene_cam_activate_es_event_handler_comps, "camera__accuratePos", DPoint3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc scene_cam_activate_es_event_handler_es_desc
(
  "scene_cam_activate_es",
  "prog/daNetGame/camera/sceneCamES.cpp.inl",
  ecs::EntitySystemOps(nullptr, scene_cam_activate_es_event_handler_all_events),
  make_span(scene_cam_activate_es_event_handler_comps+0, 1)/*rw*/,
  make_span(scene_cam_activate_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"camera__active");
static constexpr ecs::ComponentDesc set_camera_active_flag_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("camera__input_enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc set_camera_active_flag_ecs_query_desc
(
  "set_camera_active_flag_ecs_query",
  make_span(set_camera_active_flag_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_camera_active_flag_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, set_camera_active_flag_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(set_camera_active_flag_ecs_query_comps, "camera__active", bool)
            , ECS_RW_COMP_PTR(set_camera_active_flag_ecs_query_comps, "camera__input_enabled", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc switch_active_camera_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc switch_active_camera_ecs_query_desc
(
  "switch_active_camera_ecs_query",
  empty_span(),
  make_span(switch_active_camera_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void switch_active_camera_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, switch_active_camera_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(switch_active_camera_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              ECS_RO_COMP(switch_active_camera_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::component_t camera__active_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t fov_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t spectator__target_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t team_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
