// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "flashBlindES.cpp.inl"
ECS_DEF_PULL_VAR(flashBlind);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc flash_blind_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("flash_blind__intensity"), ecs::ComponentTypeInfo<float>()}
};
static void flash_blind_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    flash_blind_es(evt
        , ECS_RO_COMP(flash_blind_es_comps, "flash_blind__intensity", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc flash_blind_es_es_desc
(
  "flash_blind_es",
  "prog/daNetGameLibs/flash_blind/render/flashBlindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, flash_blind_es_all_events),
  empty_span(),
  make_span(flash_blind_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc flash_blind_stop_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("flash_blind__intensity"), ecs::ComponentTypeInfo<float>()}
};
static void flash_blind_stop_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  flash_blind_stop_es(evt
        );
}
static ecs::EntitySystemDesc flash_blind_stop_es_es_desc
(
  "flash_blind_stop_es",
  "prog/daNetGameLibs/flash_blind/render/flashBlindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, flash_blind_stop_es_all_events),
  empty_span(),
  empty_span(),
  make_span(flash_blind_stop_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc flash_blind_finish_init_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("flash_blind__just_captured"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc flash_blind_finish_init_ecs_query_desc
(
  "flash_blind_finish_init_ecs_query",
  make_span(flash_blind_finish_init_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void flash_blind_finish_init_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, flash_blind_finish_init_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(flash_blind_finish_init_ecs_query_comps, "flash_blind__just_captured", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc start_flash_blind_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("flashBlind"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
  {ECS_HASH("flash_blind__remaining_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("flash_blind__just_captured"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [3]
  {ECS_HASH("flash_blind__decay_factor"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc start_flash_blind_ecs_query_desc
(
  "start_flash_blind_ecs_query",
  make_span(start_flash_blind_ecs_query_comps+0, 3)/*rw*/,
  make_span(start_flash_blind_ecs_query_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void start_flash_blind_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, start_flash_blind_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(start_flash_blind_ecs_query_comps, "flash_blind__decay_factor", float)
            , ECS_RW_COMP(start_flash_blind_ecs_query_comps, "flashBlind", resource_slot::NodeHandleWithSlotsAccess)
            , ECS_RW_COMP(start_flash_blind_ecs_query_comps, "flash_blind__remaining_time", float)
            , ECS_RW_COMP(start_flash_blind_ecs_query_comps, "flash_blind__just_captured", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc stop_flash_blind_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("flashBlind"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
  {ECS_HASH("flash_blind__remaining_time"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc stop_flash_blind_ecs_query_desc
(
  "stop_flash_blind_ecs_query",
  make_span(stop_flash_blind_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void stop_flash_blind_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, stop_flash_blind_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(stop_flash_blind_ecs_query_comps, "flashBlind", resource_slot::NodeHandleWithSlotsAccess)
            , ECS_RW_COMP(stop_flash_blind_ecs_query_comps, "flash_blind__remaining_time", float)
            );

        }while (++comp != compE);
    }
  );
}
