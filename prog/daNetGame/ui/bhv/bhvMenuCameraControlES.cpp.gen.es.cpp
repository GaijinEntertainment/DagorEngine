// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bhvMenuCameraControlES.cpp.inl"
ECS_DEF_PULL_VAR(bhvMenuCameraControl);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc rotate_menu_cam_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("menu_cam__angles"), ecs::ComponentTypeInfo<Point2>()},
//start of 3 ro components at [1]
  {ECS_HASH("menu_cam__limitYaw"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__limitPitch"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__shouldRotateTarget"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc rotate_menu_cam_ecs_query_desc
(
  "rotate_menu_cam_ecs_query",
  make_span(rotate_menu_cam_ecs_query_comps+0, 1)/*rw*/,
  make_span(rotate_menu_cam_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void rotate_menu_cam_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, rotate_menu_cam_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP(rotate_menu_cam_ecs_query_comps, "menu_cam__shouldRotateTarget", bool)) )
            continue;
          function(
              ECS_RO_COMP(rotate_menu_cam_ecs_query_comps, "menu_cam__limitYaw", Point2)
            , ECS_RO_COMP(rotate_menu_cam_ecs_query_comps, "menu_cam__limitPitch", Point2)
            , ECS_RW_COMP(rotate_menu_cam_ecs_query_comps, "menu_cam__angles", Point2)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc rotate_menu_cam_target_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("menu_cam__angles"), ecs::ComponentTypeInfo<Point2>()},
//start of 4 ro components at [1]
  {ECS_HASH("menu_cam__limitYaw"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__limitPitch"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__dirInited"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("menu_cam__shouldRotateTarget"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc rotate_menu_cam_target_ecs_query_desc
(
  "rotate_menu_cam_target_ecs_query",
  make_span(rotate_menu_cam_target_ecs_query_comps+0, 1)/*rw*/,
  make_span(rotate_menu_cam_target_ecs_query_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void rotate_menu_cam_target_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, rotate_menu_cam_target_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(rotate_menu_cam_target_ecs_query_comps, "menu_cam__dirInited", bool) && ECS_RO_COMP(rotate_menu_cam_target_ecs_query_comps, "menu_cam__shouldRotateTarget", bool)) )
            continue;
          function(
              ECS_RO_COMP(rotate_menu_cam_target_ecs_query_comps, "menu_cam__limitYaw", Point2)
            , ECS_RO_COMP(rotate_menu_cam_target_ecs_query_comps, "menu_cam__limitPitch", Point2)
            , ECS_RW_COMP(rotate_menu_cam_target_ecs_query_comps, "menu_cam__angles", Point2)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc rotate_spectator_cam_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spectator__ang"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("spectator__dir"), ecs::ComponentTypeInfo<Point3>()},
//start of 2 ro components at [2]
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("specator_cam__smoothDiv"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc rotate_spectator_cam_ecs_query_desc
(
  "rotate_spectator_cam_ecs_query",
  make_span(rotate_spectator_cam_ecs_query_comps+0, 2)/*rw*/,
  make_span(rotate_spectator_cam_ecs_query_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void rotate_spectator_cam_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, rotate_spectator_cam_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(rotate_spectator_cam_ecs_query_comps, "spectator__ang", Point2)
            , ECS_RW_COMP(rotate_spectator_cam_ecs_query_comps, "spectator__dir", Point3)
            , ECS_RO_COMP(rotate_spectator_cam_ecs_query_comps, "camera__active", bool)
            , ECS_RO_COMP_OR(rotate_spectator_cam_ecs_query_comps, "specator_cam__smoothDiv", float(400.f))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc rotate_spectator_free_cam_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("replay_camera__tpsInputAngle"), ecs::ComponentTypeInfo<Point2>()},
//start of 2 ro components at [1]
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("specator_cam__smoothDiv"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [3]
  {ECS_HASH("replay_camera__tpsFree"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc rotate_spectator_free_cam_ecs_query_desc
(
  "rotate_spectator_free_cam_ecs_query",
  make_span(rotate_spectator_free_cam_ecs_query_comps+0, 1)/*rw*/,
  make_span(rotate_spectator_free_cam_ecs_query_comps+1, 2)/*ro*/,
  make_span(rotate_spectator_free_cam_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void rotate_spectator_free_cam_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, rotate_spectator_free_cam_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(rotate_spectator_free_cam_ecs_query_comps, "camera__active", bool)
            , ECS_RW_COMP(rotate_spectator_free_cam_ecs_query_comps, "replay_camera__tpsInputAngle", Point2)
            , ECS_RO_COMP_OR(rotate_spectator_free_cam_ecs_query_comps, "specator_cam__smoothDiv", float(400.f))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc change_spectator_camera_tps_speed_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("free_cam__move_speed"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [1]
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("replay_camera__tpsFree"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc change_spectator_camera_tps_speed_ecs_query_desc
(
  "change_spectator_camera_tps_speed_ecs_query",
  make_span(change_spectator_camera_tps_speed_ecs_query_comps+0, 1)/*rw*/,
  make_span(change_spectator_camera_tps_speed_ecs_query_comps+1, 1)/*ro*/,
  make_span(change_spectator_camera_tps_speed_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void change_spectator_camera_tps_speed_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, change_spectator_camera_tps_speed_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(change_spectator_camera_tps_speed_ecs_query_comps, "camera__active", bool)
            , ECS_RW_COMP(change_spectator_camera_tps_speed_ecs_query_comps, "free_cam__move_speed", float)
            );

        }while (++comp != compE);
    }
  );
}
