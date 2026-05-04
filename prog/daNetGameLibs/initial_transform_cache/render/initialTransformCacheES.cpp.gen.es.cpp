// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "initialTransformCacheES.cpp.inl"
ECS_DEF_PULL_VAR(initialTransformCache);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc initial_transform_cache_create_manager_es_comps[] ={};
static void initial_transform_cache_create_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  initial_transform_cache_create_manager_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc initial_transform_cache_create_manager_es_es_desc
(
  "initial_transform_cache_create_manager_es",
  "prog/daNetGameLibs/initial_transform_cache/render/initialTransformCacheES.cpp.inl",
  ecs::EntitySystemOps(nullptr, initial_transform_cache_create_manager_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc initial_transform_cache_add_fix_globtm_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void initial_transform_cache_add_fix_globtm_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnRendinstMovement>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    initial_transform_cache_add_fix_globtm_es(static_cast<const EventOnRendinstMovement&>(evt)
        , components.manager()
    , ECS_RO_COMP(initial_transform_cache_add_fix_globtm_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc initial_transform_cache_add_fix_globtm_es_es_desc
(
  "initial_transform_cache_add_fix_globtm_es",
  "prog/daNetGameLibs/initial_transform_cache/render/initialTransformCacheES.cpp.inl",
  ecs::EntitySystemOps(nullptr, initial_transform_cache_add_fix_globtm_es_all_events),
  empty_span(),
  make_span(initial_transform_cache_add_fix_globtm_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnRendinstMovement>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc initial_transform_cache_get_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("initial_transform_cache_manager"), ecs::ComponentTypeInfo<InitialTransformCacheManager>()}
};
static ecs::CompileTimeQueryDesc initial_transform_cache_get_manager_ecs_query_desc
(
  "initial_transform_cache_get_manager_ecs_query",
  make_span(initial_transform_cache_get_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void initial_transform_cache_get_manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, initial_transform_cache_get_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(initial_transform_cache_get_manager_ecs_query_comps, "initial_transform_cache_manager", InitialTransformCacheManager)
            );

        }while (++comp != compE);
    }
  );
}
