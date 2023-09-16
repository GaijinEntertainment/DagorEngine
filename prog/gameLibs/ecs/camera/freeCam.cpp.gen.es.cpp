#include "freeCam.cpp.inl"
ECS_DEF_PULL_VAR(freeCam);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc free_cam_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("free_cam"), ecs::ComponentTypeInfo<FreeCam>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("free_cam__mouse"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("free_cam__move"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("free_cam__rotate"), ecs::ComponentTypeInfo<Point2>()},
//start of 4 ro components at [5]
  {ECS_HASH("free_cam__shiftY"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("free_cam__turbo"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("free_cam__bank"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void free_cam_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(free_cam_es_comps, "camera__active", bool)) )
      continue;
    free_cam_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(free_cam_es_comps, "free_cam", FreeCam)
      , ECS_RW_COMP(free_cam_es_comps, "transform", TMatrix)
      , ECS_RW_COMP(free_cam_es_comps, "free_cam__mouse", Point2)
      , ECS_RW_COMP(free_cam_es_comps, "free_cam__move", Point2)
      , ECS_RW_COMP(free_cam_es_comps, "free_cam__rotate", Point2)
      , ECS_RO_COMP(free_cam_es_comps, "free_cam__shiftY", float)
      , ECS_RO_COMP(free_cam_es_comps, "free_cam__turbo", bool)
      , ECS_RO_COMP_OR(free_cam_es_comps, "free_cam__bank", float(0.f))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc free_cam_es_es_desc
(
  "free_cam_es",
  "prog/gameLibs/ecs/camera/freeCam.cpp.inl",
  ecs::EntitySystemOps(nullptr, free_cam_es_all_events),
  make_span(free_cam_es_comps+0, 5)/*rw*/,
  make_span(free_cam_es_comps+5, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"before_camera_sync","camera_set_sync");
static constexpr ecs::ComponentDesc free_cam_update_params_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("free_cam"), ecs::ComponentTypeInfo<FreeCam>()},
//start of 3 ro components at [1]
  {ECS_HASH("free_cam__angSpdScale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("free_cam__move_speed"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("free_cam__turbo_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void free_cam_update_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    free_cam_update_params_es_event_handler(evt
        , ECS_RW_COMP(free_cam_update_params_es_event_handler_comps, "free_cam", FreeCam)
    , ECS_RO_COMP(free_cam_update_params_es_event_handler_comps, "free_cam__angSpdScale", Point2)
    , ECS_RO_COMP_OR(free_cam_update_params_es_event_handler_comps, "free_cam__move_speed", float(5.f))
    , ECS_RO_COMP_OR(free_cam_update_params_es_event_handler_comps, "free_cam__turbo_scale", float(20.f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc free_cam_update_params_es_event_handler_es_desc
(
  "free_cam_update_params_es",
  "prog/gameLibs/ecs/camera/freeCam.cpp.inl",
  ecs::EntitySystemOps(nullptr, free_cam_update_params_es_event_handler_all_events),
  make_span(free_cam_update_params_es_event_handler_comps+0, 1)/*rw*/,
  make_span(free_cam_update_params_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","free_cam__angSpdScale,free_cam__move_speed","free_cam_es");
static constexpr ecs::ComponentDesc get_free_cam_speeds_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("free_cam"), ecs::ComponentTypeInfo<FreeCam>()},
//start of 2 ro components at [1]
  {ECS_HASH("free_cam__move_speed"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("free_cam__turbo_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_free_cam_speeds_ecs_query_desc
(
  "get_free_cam_speeds_ecs_query",
  make_span(get_free_cam_speeds_ecs_query_comps+0, 1)/*rw*/,
  make_span(get_free_cam_speeds_ecs_query_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_free_cam_speeds_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_free_cam_speeds_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_free_cam_speeds_ecs_query_comps, "free_cam", FreeCam)
            , ECS_RO_COMP_OR(get_free_cam_speeds_ecs_query_comps, "free_cam__move_speed", float(5.f))
            , ECS_RO_COMP_OR(get_free_cam_speeds_ecs_query_comps, "free_cam__turbo_scale", float(20.f))
            );

        }while (++comp != compE);
    }
  );
}
