// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "adaptation_debugES.cpp.inl"
ECS_DEF_PULL_VAR(adaptation_debug);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc get_adaptation_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()}
};
static ecs::CompileTimeQueryDesc get_adaptation_manager_ecs_query_desc
(
  "get_adaptation_manager_ecs_query",
  make_span(get_adaptation_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_adaptation_manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_adaptation_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_adaptation_manager_ecs_query_comps, "adaptation__manager", AdaptationManager)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc adaptation_override_settings_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation_override_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc adaptation_override_settings_ecs_query_desc
(
  "adaptation_override_settings_ecs_query",
  make_span(adaptation_override_settings_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void adaptation_override_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, adaptation_override_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(adaptation_override_settings_ecs_query_comps, "adaptation_override_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc adaptation_level_settings_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation_level_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc adaptation_level_settings_ecs_query_desc
(
  "adaptation_level_settings_ecs_query",
  make_span(adaptation_level_settings_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void adaptation_level_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, adaptation_level_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(adaptation_level_settings_ecs_query_comps, "adaptation_level_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_adaptation_debug_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__debug"), ecs::ComponentTypeInfo<AdaptationDebug>()}
};
static ecs::CompileTimeQueryDesc get_adaptation_debug_ecs_query_desc
(
  "get_adaptation_debug_ecs_query",
  make_span(get_adaptation_debug_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_adaptation_debug_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_adaptation_debug_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_adaptation_debug_ecs_query_comps, "adaptation__debug", AdaptationDebug)
            );

        }while (++comp != compE);
    }
  );
}
