// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "testLightSphereRenderES.cpp.inl"
ECS_DEF_PULL_VAR(testLightSphereRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc delete_sphere_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("triggerDeleteSphere"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("IsDebugGbufferSphereTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void delete_sphere_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(delete_sphere_es_comps, "triggerDeleteSphere", bool)) )
      continue;
    delete_sphere_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(delete_sphere_es_comps, "eid", ecs::EntityId)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc delete_sphere_es_es_desc
(
  "delete_sphere_es",
  "prog/daNetGame/render/world/testLightSphereRenderES.cpp.inl",
  ecs::EntitySystemOps(delete_sphere_es_all),
  empty_span(),
  make_span(delete_sphere_es_comps+0, 2)/*ro*/,
  make_span(delete_sphere_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc test_light_sphere_render_es_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("metallness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("reflectance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("smoothness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("albedo"), ecs::ComponentTypeInfo<Point3>()},
//start of 1 rq components at [6]
  {ECS_HASH("IsDebugGbufferSphereTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void test_light_sphere_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    test_light_sphere_render_es(static_cast<const UpdateStageInfoRender&>(evt)
        , ECS_RO_COMP(test_light_sphere_render_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(test_light_sphere_render_es_comps, "size", float)
    , ECS_RO_COMP(test_light_sphere_render_es_comps, "metallness", float)
    , ECS_RO_COMP(test_light_sphere_render_es_comps, "reflectance", float)
    , ECS_RO_COMP(test_light_sphere_render_es_comps, "smoothness", float)
    , ECS_RO_COMP(test_light_sphere_render_es_comps, "albedo", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc test_light_sphere_render_es_es_desc
(
  "test_light_sphere_render_es",
  "prog/daNetGame/render/world/testLightSphereRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, test_light_sphere_render_es_all_events),
  empty_span(),
  make_span(test_light_sphere_render_es_comps+0, 6)/*ro*/,
  make_span(test_light_sphere_render_es_comps+6, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc operate_camera_bounded_sphere_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 2 ro components at [1]
  {ECS_HASH("cameraOffset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("isFixedToCamera"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [3]
  {ECS_HASH("IsDebugGbufferSphereTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void operate_camera_bounded_sphere_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(operate_camera_bounded_sphere_es_comps, "isFixedToCamera", bool)) )
      continue;
    operate_camera_bounded_sphere_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(operate_camera_bounded_sphere_es_comps, "transform", TMatrix)
      , ECS_RO_COMP(operate_camera_bounded_sphere_es_comps, "cameraOffset", Point3)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc operate_camera_bounded_sphere_es_es_desc
(
  "operate_camera_bounded_sphere_es",
  "prog/daNetGame/render/world/testLightSphereRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, operate_camera_bounded_sphere_es_all_events),
  make_span(operate_camera_bounded_sphere_es_comps+0, 1)/*rw*/,
  make_span(operate_camera_bounded_sphere_es_comps+1, 2)/*ro*/,
  make_span(operate_camera_bounded_sphere_es_comps+3, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc new_sphere_created_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("IsDebugGbufferSphereTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void new_sphere_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  new_sphere_created_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc new_sphere_created_es_event_handler_es_desc
(
  "new_sphere_created_es",
  "prog/daNetGame/render/world/testLightSphereRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, new_sphere_created_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(new_sphere_created_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc sphere_deleted_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("IsDebugGbufferSphereTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void sphere_deleted_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  sphere_deleted_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc sphere_deleted_es_event_handler_es_desc
(
  "sphere_deleted_es",
  "prog/daNetGame/render/world/testLightSphereRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sphere_deleted_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(sphere_deleted_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc current_camera_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc current_camera_ecs_query_desc
(
  "current_camera_ecs_query",
  empty_span(),
  make_span(current_camera_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void current_camera_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, current_camera_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(current_camera_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
