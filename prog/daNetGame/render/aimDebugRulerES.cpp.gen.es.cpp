#include "aimDebugRulerES.cpp.inl"
ECS_DEF_PULL_VAR(aimDebugRuler);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc debug_physmap_decals_es_comps[] ={};
static void debug_physmap_decals_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    debug_physmap_decals_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc debug_physmap_decals_es_es_desc
(
  "debug_physmap_decals_es",
  "prog/daNetGame/render/aimDebugRulerES.cpp.inl",
  ecs::EntitySystemOps(debug_physmap_decals_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc debug_physmap_appear_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_map_tex"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
//start of 1 ro components at [1]
  {ECS_HASH("drawPhysMap"), ecs::ComponentTypeInfo<bool>()}
};
static void debug_physmap_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    debug_physmap_appear_es_event_handler(evt
        , ECS_RW_COMP(debug_physmap_appear_es_event_handler_comps, "phys_map_tex", UniqueTexHolder)
    , ECS_RO_COMP(debug_physmap_appear_es_event_handler_comps, "drawPhysMap", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_physmap_appear_es_event_handler_es_desc
(
  "debug_physmap_appear_es",
  "prog/daNetGame/render/aimDebugRulerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, debug_physmap_appear_es_event_handler_all_events),
  make_span(debug_physmap_appear_es_event_handler_comps+0, 1)/*rw*/,
  make_span(debug_physmap_appear_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev,render","drawPhysMap");
