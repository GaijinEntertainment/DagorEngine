// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "laserSightDebugES.cpp.inl"
ECS_DEF_PULL_VAR(laserSightDebug);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc get_all_lasers_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("laserActive"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_all_lasers_ecs_query_desc
(
  "get_all_lasers_ecs_query",
  make_span(get_all_lasers_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_all_lasers_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_all_lasers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_all_lasers_ecs_query_comps, "laserActive", bool)
            );

        }while (++comp != compE);
    }
  );
}
