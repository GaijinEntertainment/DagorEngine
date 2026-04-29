// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "targetRenameBeforeMotionBlurNodeES.cpp.inl"
ECS_DEF_PULL_VAR(targetRenameBeforeMotionBlurNode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_target_rename_before_motion_blur_node_es_comps[] ={};
static void create_target_rename_before_motion_blur_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_target_rename_before_motion_blur_node_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_target_rename_before_motion_blur_node_es_es_desc
(
  "create_target_rename_before_motion_blur_node_es",
  "prog/daNetGame/render/world/frameGraphNodes/targetRenameBeforeMotionBlurNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_target_rename_before_motion_blur_node_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
