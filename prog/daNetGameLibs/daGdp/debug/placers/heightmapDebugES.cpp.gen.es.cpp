#include "heightmapDebugES.cpp.inl"
ECS_DEF_PULL_VAR(heightmapDebug);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc heightmap_debug_invalidate_views_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__heightmap_manager"), ecs::ComponentTypeInfo<dagdp::HeightmapManager>()}
};
static void heightmap_debug_invalidate_views_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventInvalidateViews>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::heightmap_debug_invalidate_views_es(static_cast<const dagdp::EventInvalidateViews&>(evt)
        , ECS_RW_COMP(heightmap_debug_invalidate_views_es_comps, "dagdp__heightmap_manager", dagdp::HeightmapManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc heightmap_debug_invalidate_views_es_es_desc
(
  "heightmap_debug_invalidate_views_es",
  "prog/daNetGameLibs/daGdp/debug/placers/heightmapDebugES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heightmap_debug_invalidate_views_es_all_events),
  make_span(heightmap_debug_invalidate_views_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventInvalidateViews>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc heightmap_debug_imgui_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()},
  {ECS_HASH("dagdp__heightmap_manager"), ecs::ComponentTypeInfo<dagdp::HeightmapManager>()}
};
static ecs::CompileTimeQueryDesc heightmap_debug_imgui_ecs_query_desc
(
  "dagdp::heightmap_debug_imgui_ecs_query",
  make_span(heightmap_debug_imgui_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::heightmap_debug_imgui_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, heightmap_debug_imgui_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(heightmap_debug_imgui_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            , ECS_RW_COMP(heightmap_debug_imgui_ecs_query_comps, "dagdp__heightmap_manager", dagdp::HeightmapManager)
            );

        }while (++comp != compE);
    }
  );
}
