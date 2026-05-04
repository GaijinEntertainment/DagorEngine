// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "decalsMatrices.cpp"
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc decals_invalidate_entities_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("decals__useDecalMatrices"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc decals_invalidate_entities_ecs_query_desc
(
  "decals_invalidate_entities_ecs_query",
  make_span(decals_invalidate_entities_ecs_query_comps+0, 1)/*rw*/,
  make_span(decals_invalidate_entities_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void decals_invalidate_entities_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, decals_invalidate_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(decals_invalidate_entities_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(decals_invalidate_entities_ecs_query_comps, "decals__useDecalMatrices", bool)
            );

        }while (++comp != compE);
    }
  );
}
