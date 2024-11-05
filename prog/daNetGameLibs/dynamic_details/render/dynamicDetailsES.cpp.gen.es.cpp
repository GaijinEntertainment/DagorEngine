#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t appearance__rndSeed_get_type();
static ecs::LTComponentList appearance__rndSeed_component(ECS_HASH("appearance__rndSeed"), appearance__rndSeed_get_type(), "prog/daNetGameLibs/dynamic_details/render/dynamicDetailsES.cpp.inl", "dynamic_details_es_event_handler", 0);
#include "dynamicDetailsES.cpp.inl"
ECS_DEF_PULL_VAR(dynamicDetails);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_dynamic_details_es_comps[] ={};
static void create_dynamic_details_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  create_dynamic_details_es(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc create_dynamic_details_es_es_desc
(
  "create_dynamic_details_es",
  "prog/daNetGameLibs/dynamic_details/render/dynamicDetailsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_dynamic_details_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dynamic_details_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 4 ro components at [1]
  {ECS_HASH("dynamic_details__groups"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("dynamic_details__presets"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("dynamic_details_preset"), ecs::ComponentTypeInfo<ecs::Array>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("skeleton_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void dynamic_details_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_details_es_event_handler(evt
        , ECS_RW_COMP(dynamic_details_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(dynamic_details_es_event_handler_comps, "dynamic_details__groups", ecs::Object)
    , ECS_RO_COMP(dynamic_details_es_event_handler_comps, "dynamic_details__presets", ecs::Array)
    , ECS_RO_COMP_PTR(dynamic_details_es_event_handler_comps, "dynamic_details_preset", ecs::Array)
    , ECS_RO_COMP(dynamic_details_es_event_handler_comps, "skeleton_attach__attachedTo", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_details_es_event_handler_es_desc
(
  "dynamic_details_es",
  "prog/daNetGameLibs/dynamic_details/render/dynamicDetailsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_details_es_event_handler_all_events),
  make_span(dynamic_details_es_event_handler_comps+0, 1)/*rw*/,
  make_span(dynamic_details_es_event_handler_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","skeleton_attach__attachedTo");
static constexpr ecs::ComponentDesc dynamic_detials_after_reset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dynamic_details_mgr"), ecs::ComponentTypeInfo<DynamicDetailsTextureManager>()}
};
static void dynamic_detials_after_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_detials_after_reset_es(static_cast<const AfterDeviceReset&>(evt)
        , ECS_RW_COMP(dynamic_detials_after_reset_es_comps, "dynamic_details_mgr", DynamicDetailsTextureManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_detials_after_reset_es_es_desc
(
  "dynamic_detials_after_reset_es",
  "prog/daNetGameLibs/dynamic_details/render/dynamicDetailsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_detials_after_reset_es_all_events),
  make_span(dynamic_detials_after_reset_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
);
static constexpr ecs::ComponentDesc create_dynamic_details_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dynamic_details_mgr"), ecs::ComponentTypeInfo<DynamicDetailsTextureManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("dynamicDetails"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc create_dynamic_details_ecs_query_desc
(
  "create_dynamic_details_ecs_query",
  make_span(create_dynamic_details_ecs_query_comps+0, 1)/*rw*/,
  make_span(create_dynamic_details_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void create_dynamic_details_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, create_dynamic_details_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(create_dynamic_details_ecs_query_comps, "dynamic_details_mgr", DynamicDetailsTextureManager)
            , ECS_RO_COMP(create_dynamic_details_ecs_query_comps, "dynamicDetails", ecs::StringList)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_dynamic_details_indices_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dynamic_details_mgr"), ecs::ComponentTypeInfo<DynamicDetailsTextureManager>()}
};
static ecs::CompileTimeQueryDesc get_dynamic_details_indices_ecs_query_desc
(
  "get_dynamic_details_indices_ecs_query",
  empty_span(),
  make_span(get_dynamic_details_indices_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_dynamic_details_indices_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_dynamic_details_indices_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_dynamic_details_indices_ecs_query_comps, "dynamic_details_mgr", DynamicDetailsTextureManager)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc info_dynamic_details_selected_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 5 rq components at [2]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("dynamic_details__presets"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("dynamic_details__groups"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("skeleton_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("daeditor__selected"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc info_dynamic_details_selected_ecs_query_desc
(
  "info_dynamic_details_selected_ecs_query",
  make_span(info_dynamic_details_selected_ecs_query_comps+0, 1)/*rw*/,
  make_span(info_dynamic_details_selected_ecs_query_comps+1, 1)/*ro*/,
  make_span(info_dynamic_details_selected_ecs_query_comps+2, 5)/*rq*/,
  empty_span());
template<typename Callable>
inline void info_dynamic_details_selected_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, info_dynamic_details_selected_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(info_dynamic_details_selected_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(info_dynamic_details_selected_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t appearance__rndSeed_get_type(){return ecs::ComponentTypeInfo<int>::type; }
