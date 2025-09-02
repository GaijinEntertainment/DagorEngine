// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "physObjActorDbgRenderES.cpp.inl"
ECS_DEF_PULL_VAR(physObjActorDbgRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc debug_draw_phys_phys_obj_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()}
};
static void debug_draw_phys_phys_obj_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    debug_draw_phys_phys_obj_es(*info.cast<UpdateStageInfoRenderDebug>()
    , ECS_RW_COMP(debug_draw_phys_phys_obj_es_comps, "phys_obj_net_phys", PhysObjActor)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_draw_phys_phys_obj_es_es_desc
(
  "debug_draw_phys_phys_obj_es",
  "prog/daNetGameLibs/package_physobj/render/physObjActorDbgRenderES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_phys_phys_obj_es_all),
  make_span(debug_draw_phys_phys_obj_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
