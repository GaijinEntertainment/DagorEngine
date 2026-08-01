// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dafgCameraRegistratorES.cpp.inl"
ECS_DEF_PULL_VAR(dafgCameraRegistrator);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dafg_camera_registrator_appear_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dafg_camera_registrator__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void dafg_camera_registrator_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dafg_camera_registrator_appear_es(evt
        , ECS_RO_COMP(dafg_camera_registrator_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(dafg_camera_registrator_appear_es_comps, "dafg_camera_registrator__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dafg_camera_registrator_appear_es_es_desc
(
  "dafg_camera_registrator_appear_es",
  "prog/daNetGame/render/world/dafgCameraRegistratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dafg_camera_registrator_appear_es_all_events),
  empty_span(),
  make_span(dafg_camera_registrator_appear_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnWorldRendererCreated,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dafg_camera_registrator_disappear_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dafg_camera_registrator__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void dafg_camera_registrator_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dafg_camera_registrator_disappear_es(evt
        , ECS_RO_COMP(dafg_camera_registrator_disappear_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(dafg_camera_registrator_disappear_es_comps, "dafg_camera_registrator__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dafg_camera_registrator_disappear_es_es_desc
(
  "dafg_camera_registrator_disappear_es",
  "prog/daNetGame/render/world/dafgCameraRegistratorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dafg_camera_registrator_disappear_es_all_events),
  empty_span(),
  make_span(dafg_camera_registrator_disappear_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
