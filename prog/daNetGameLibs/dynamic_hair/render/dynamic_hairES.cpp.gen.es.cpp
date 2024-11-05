#include "dynamic_hairES.cpp.inl"
ECS_DEF_PULL_VAR(dynamic_hair);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc detect_hair_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void detect_hair_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    detect_hair_es(evt
        , ECS_RW_COMP(detect_hair_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(detect_hair_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc detect_hair_es_es_desc
(
  "detect_hair_es",
  "prog/daNetGameLibs/dynamic_hair/render/dynamic_hairES.cpp.inl",
  ecs::EntitySystemOps(nullptr, detect_hair_es_all_events),
  make_span(detect_hair_es_comps+0, 1)/*rw*/,
  make_span(detect_hair_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc remove_hair_on_destroy_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("dynamic_hair_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void remove_hair_on_destroy_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    remove_hair_on_destroy_es(evt
        , ECS_RO_COMP(remove_hair_on_destroy_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc remove_hair_on_destroy_es_es_desc
(
  "remove_hair_on_destroy_es",
  "prog/daNetGameLibs/dynamic_hair/render/dynamic_hairES.cpp.inl",
  ecs::EntitySystemOps(nullptr, remove_hair_on_destroy_es_all_events),
  empty_span(),
  make_span(remove_hair_on_destroy_es_comps+0, 1)/*ro*/,
  make_span(remove_hair_on_destroy_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc init_dynamic_hair_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dynamic_hair__render_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void init_dynamic_hair_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_dynamic_hair_es_event_handler(evt
        , ECS_RW_COMP(init_dynamic_hair_es_event_handler_comps, "dynamic_hair__render_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_dynamic_hair_es_event_handler_es_desc
(
  "init_dynamic_hair_es",
  "prog/daNetGameLibs/dynamic_hair/render/dynamic_hairES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_dynamic_hair_es_event_handler_all_events),
  make_span(init_dynamic_hair_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc add_entity_with_hair_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("dynamic_hair__max_nodes_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dynamic_hair__entities"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("dynamic_hair__nodeMaskOffsets"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("dynamic_hair__nodeMasks"), ecs::ComponentTypeInfo<ecs::UInt8List>()}
};
static ecs::CompileTimeQueryDesc add_entity_with_hair_ecs_query_desc
(
  "add_entity_with_hair_ecs_query",
  make_span(add_entity_with_hair_ecs_query_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void add_entity_with_hair_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, add_entity_with_hair_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(add_entity_with_hair_ecs_query_comps, "dynamic_hair__max_nodes_count", int)
            , ECS_RW_COMP(add_entity_with_hair_ecs_query_comps, "dynamic_hair__entities", ecs::EidList)
            , ECS_RW_COMP(add_entity_with_hair_ecs_query_comps, "dynamic_hair__nodeMaskOffsets", ecs::IntList)
            , ECS_RW_COMP(add_entity_with_hair_ecs_query_comps, "dynamic_hair__nodeMasks", ecs::UInt8List)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc remove_entity_with_hair_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("dynamic_hair__entities"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("dynamic_hair__nodeMaskOffsets"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("dynamic_hair__nodeMasks"), ecs::ComponentTypeInfo<ecs::UInt8List>()}
};
static ecs::CompileTimeQueryDesc remove_entity_with_hair_ecs_query_desc
(
  "remove_entity_with_hair_ecs_query",
  make_span(remove_entity_with_hair_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void remove_entity_with_hair_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, remove_entity_with_hair_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(remove_entity_with_hair_ecs_query_comps, "dynamic_hair__entities", ecs::EidList)
            , ECS_RW_COMP(remove_entity_with_hair_ecs_query_comps, "dynamic_hair__nodeMaskOffsets", ecs::IntList)
            , ECS_RW_COMP(remove_entity_with_hair_ecs_query_comps, "dynamic_hair__nodeMasks", ecs::UInt8List)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc render_hair_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dynamic_hair__max_nodes_count"), ecs::ComponentTypeInfo<int>()},
//start of 3 ro components at [1]
  {ECS_HASH("dynamic_hair__entities"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("dynamic_hair__nodeMaskOffsets"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("dynamic_hair__nodeMasks"), ecs::ComponentTypeInfo<ecs::UInt8List>()}
};
static ecs::CompileTimeQueryDesc render_hair_ecs_query_desc
(
  "render_hair_ecs_query",
  make_span(render_hair_ecs_query_comps+0, 1)/*rw*/,
  make_span(render_hair_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_hair_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, render_hair_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(render_hair_ecs_query_comps, "dynamic_hair__max_nodes_count", int)
            , ECS_RO_COMP(render_hair_ecs_query_comps, "dynamic_hair__entities", ecs::EidList)
            , ECS_RO_COMP(render_hair_ecs_query_comps, "dynamic_hair__nodeMaskOffsets", ecs::IntList)
            , ECS_RO_COMP(render_hair_ecs_query_comps, "dynamic_hair__nodeMasks", ecs::UInt8List)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc gather_hair_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()}
};
static ecs::CompileTimeQueryDesc gather_hair_ecs_query_desc
(
  "gather_hair_ecs_query",
  empty_span(),
  make_span(gather_hair_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_hair_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, gather_hair_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(gather_hair_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(gather_hair_ecs_query_comps, "animchar_visbits", uint8_t)
            );

        }
    }
  );
}
