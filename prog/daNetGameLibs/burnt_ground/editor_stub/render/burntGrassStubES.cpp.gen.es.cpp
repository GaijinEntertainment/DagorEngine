// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "burntGrassStubES.cpp.inl"
ECS_DEF_PULL_VAR(burntGrassStub);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc burnt_grass_renderer_stub_lazy_setup_es_comps[] ={};
static void burnt_grass_renderer_stub_lazy_setup_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  burnt_grass_renderer_stub_lazy_setup_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc burnt_grass_renderer_stub_lazy_setup_es_es_desc
(
  "burnt_grass_renderer_stub_lazy_setup_es",
  "prog/daNetGameLibs/burnt_ground/editor_stub/render/burntGrassStubES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_grass_renderer_stub_lazy_setup_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
