// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cameraIndoorES.cpp.inl"
ECS_DEF_PULL_VAR(cameraIndoor);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc camera_indoor_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("camera_indoor__pos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("camera_indoor__target"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera_indoor__value"), ecs::ComponentTypeInfo<float>()}
};
static void camera_indoor_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    camera_indoor_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RW_COMP(camera_indoor_es_comps, "camera_indoor__pos", Point3)
    , ECS_RW_COMP(camera_indoor_es_comps, "camera_indoor__target", float)
    , ECS_RW_COMP(camera_indoor_es_comps, "camera_indoor__value", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camera_indoor_es_es_desc
(
  "camera_indoor_es",
  "prog/daNetGame/render/cameraIndoorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, camera_indoor_es_all_events),
  make_span(camera_indoor_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc camera_indoor_level_unload_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("camera_indoor__value"), ecs::ComponentTypeInfo<float>()}
};
static void camera_indoor_level_unload_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  camera_indoor_level_unload_es(evt
        );
}
static ecs::EntitySystemDesc camera_indoor_level_unload_es_es_desc
(
  "camera_indoor_level_unload_es",
  "prog/daNetGame/render/cameraIndoorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, camera_indoor_level_unload_es_all_events),
  empty_span(),
  empty_span(),
  make_span(camera_indoor_level_unload_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
