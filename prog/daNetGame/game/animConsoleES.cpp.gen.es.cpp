// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animConsoleES.cpp.inl"
ECS_DEF_PULL_VAR(animConsole);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc set_irq_pos_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc set_irq_pos_ecs_query_desc
(
  "set_irq_pos_ecs_query",
  make_span(set_irq_pos_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_irq_pos_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_irq_pos_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_irq_pos_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }while (++comp != compE);
    }
  );
}
