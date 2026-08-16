// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "segHumanPhysES.cpp.inl"
ECS_DEF_PULL_VAR(segHumanPhys);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc process_all_segmented_human_physics_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("human_segmented_physics"), ecs::ComponentTypeInfo<ecs::SharedComponent<SharedSegmentedHumanPhysics>>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc process_all_segmented_human_physics_ecs_query_desc
(
  "process_all_segmented_human_physics_ecs_query",
  make_span(process_all_segmented_human_physics_ecs_query_comps+0, 1)/*rw*/,
  make_span(process_all_segmented_human_physics_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void process_all_segmented_human_physics_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, process_all_segmented_human_physics_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(process_all_segmented_human_physics_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(process_all_segmented_human_physics_ecs_query_comps, "human_segmented_physics", ecs::SharedComponent<SharedSegmentedHumanPhysics>)
            );

        }while (++comp != compE);
    }
  );
}
