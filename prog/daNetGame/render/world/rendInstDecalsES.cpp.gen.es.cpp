#include "rendInstDecalsES.cpp.inl"
ECS_DEF_PULL_VAR(rendInstDecals);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc rend_inst_decals_render_es_comps[] ={};
static void rend_inst_decals_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnRenderDecals>());
  rend_inst_decals_render_es(static_cast<const OnRenderDecals&>(evt)
        );
}
static ecs::EntitySystemDesc rend_inst_decals_render_es_es_desc
(
  "rend_inst_decals_render_es",
  "prog/daNetGame/render/world/rendInstDecalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rend_inst_decals_render_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderDecals>::build(),
  0
);
