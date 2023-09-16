#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/gameLibs/ecs/camera/cameraView.cpp.inl", "", 0);
#include "cameraView.cpp.inl"
ECS_DEF_PULL_VAR(cameraView);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc process_active_camera_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("camera__accuratePos"), ecs::ComponentTypeInfo<DPoint3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("fov"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("znear"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("zfar"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("camera__fovHorPlus"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("camera__fovHybrid"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [9]
  {ECS_HASH("camera_view"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc process_active_camera_ecs_query_desc
(
  "process_active_camera_ecs_query",
  empty_span(),
  make_span(process_active_camera_ecs_query_comps+0, 9)/*ro*/,
  make_span(process_active_camera_ecs_query_comps+9, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void process_active_camera_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, process_active_camera_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(process_active_camera_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              components.manager()
            , ECS_RO_COMP(process_active_camera_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(process_active_camera_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_PTR(process_active_camera_ecs_query_comps, "camera__accuratePos", DPoint3)
            , ECS_RO_COMP_OR(process_active_camera_ecs_query_comps, "fov", float(90.0f))
            , ECS_RO_COMP_OR(process_active_camera_ecs_query_comps, "znear", float(0.1f))
            , ECS_RO_COMP_OR(process_active_camera_ecs_query_comps, "zfar", float(5000.f))
            , ECS_RO_COMP_OR(process_active_camera_ecs_query_comps, "camera__fovHorPlus", bool(true))
            , ECS_RO_COMP_OR(process_active_camera_ecs_query_comps, "camera__fovHybrid", bool(false))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
