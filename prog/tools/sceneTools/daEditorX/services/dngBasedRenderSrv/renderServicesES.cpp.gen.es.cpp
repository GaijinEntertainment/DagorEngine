// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "renderServicesES.cpp.inl"
ECS_DEF_PULL_VAR(renderServices);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc render_services_debug_es_comps[] ={};
static void render_services_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    render_services_debug_es(*info.cast<UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc render_services_debug_es_es_desc
(
  "render_services_debug_es",
  "prog/tools/sceneTools/daEditorX/services/dngBasedRenderSrv/renderServicesES.cpp.inl",
  ecs::EntitySystemOps(render_services_debug_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc render_services_opaque_es_comps[] ={};
static void render_services_opaque_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  render_services_opaque_es(static_cast<const UpdateStageInfoRender&>(evt)
        );
}
static ecs::EntitySystemDesc render_services_opaque_es_es_desc
(
  "render_services_opaque_es",
  "prog/tools/sceneTools/daEditorX/services/dngBasedRenderSrv/renderServicesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, render_services_opaque_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc render_services_transp_es_comps[] ={};
static void render_services_transp_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRenderTrans>());
  render_services_transp_es(static_cast<const UpdateStageInfoRenderTrans&>(evt)
        );
}
static ecs::EntitySystemDesc render_services_transp_es_es_desc
(
  "render_services_transp_es",
  "prog/tools/sceneTools/daEditorX/services/dngBasedRenderSrv/renderServicesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, render_services_transp_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRenderTrans>::build(),
  0
,"render");
