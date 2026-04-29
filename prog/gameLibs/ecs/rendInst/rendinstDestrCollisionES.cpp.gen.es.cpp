// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "rendinstDestrCollisionES.cpp.inl"
ECS_DEF_PULL_VAR(rendinstDestrCollision);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc process_ri_destr_collision_queue_es_comps[] ={};
static void process_ri_destr_collision_queue_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  process_ri_destr_collision_queue_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc process_ri_destr_collision_queue_es_es_desc
(
  "process_ri_destr_collision_queue_es",
  "prog/gameLibs/ecs/rendInst/./rendinstDestrCollisionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, process_ri_destr_collision_queue_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"gameClient",nullptr,"start_async_phys_sim_es");
