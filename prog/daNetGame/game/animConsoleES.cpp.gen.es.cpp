// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animConsoleES.cpp.inl"
ECS_DEF_PULL_VAR(animConsole);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_eid_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc animchar_eid_ecs_query_desc
(
  "animchar_eid_ecs_query",
  make_span(animchar_eid_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_eid_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, animchar_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }while (++comp != compE);
    }
  );
}
