#include "minimapES.cpp.inl"
ECS_DEF_PULL_VAR(minimap);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc hud_minimap_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("minimap_context"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void hud_minimap_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RenderEventUI>());
  hud_minimap_es(static_cast<const RenderEventUI&>(evt)
        );
}
static ecs::EntitySystemDesc hud_minimap_es_es_desc
(
  "hud_minimap_es",
  "prog/daNetGameLibs/minimap/ui/minimapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hud_minimap_es_all_events),
  empty_span(),
  empty_span(),
  make_span(hud_minimap_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RenderEventUI>::build(),
  0
,"gameClient,render");
