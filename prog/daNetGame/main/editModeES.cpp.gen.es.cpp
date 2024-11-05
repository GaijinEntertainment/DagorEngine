#include "editModeES.cpp.inl"
ECS_DEF_PULL_VAR(editMode);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc make_animchar_not_updatable_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<ecs::auto_type>()},
//start of 2 no components at [2]
  {ECS_HASH("isAlive"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc make_animchar_not_updatable_ecs_query_desc
(
  "make_animchar_not_updatable_ecs_query",
  empty_span(),
  make_span(make_animchar_not_updatable_ecs_query_comps+0, 1)/*ro*/,
  make_span(make_animchar_not_updatable_ecs_query_comps+1, 1)/*rq*/,
  make_span(make_animchar_not_updatable_ecs_query_comps+2, 2)/*no*/);
template<typename Callable>
inline void make_animchar_not_updatable_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, make_animchar_not_updatable_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(make_animchar_not_updatable_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
