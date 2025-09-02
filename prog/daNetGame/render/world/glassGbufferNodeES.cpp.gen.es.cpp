// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "glassGbufferNodeES.cpp.inl"
ECS_DEF_PULL_VAR(glassGbufferNode);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc glass_gbuffer_renderer_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("glass_gbuffer_renderer_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void glass_gbuffer_renderer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    glass_gbuffer_renderer_es(evt
        , ECS_RW_COMP(glass_gbuffer_renderer_es_comps, "glass_gbuffer_renderer_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc glass_gbuffer_renderer_es_es_desc
(
  "glass_gbuffer_renderer_es",
  "prog/daNetGame/render/world/glassGbufferNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, glass_gbuffer_renderer_es_all_events),
  make_span(glass_gbuffer_renderer_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRTTRChanged,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc glass_rttr_recreate_es_comps[] =
{
//start of 2 rq components at [0]
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__enableRTTR"), ecs::ComponentTypeInfo<bool>()}
};
static void glass_rttr_recreate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  glass_rttr_recreate_es(evt
        );
}
static ecs::EntitySystemDesc glass_rttr_recreate_es_es_desc
(
  "glass_rttr_recreate_es",
  "prog/daNetGame/render/world/glassGbufferNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, glass_rttr_recreate_es_all_events),
  empty_span(),
  empty_span(),
  make_span(glass_rttr_recreate_es_comps+0, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"render_settings__bare_minimum,render_settings__enableRTTR",nullptr,"bvh_render_settings_changed_es");
static constexpr ecs::ComponentDesc animchar_render_trans_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [3]
  {ECS_HASH("requires_trans_render"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 2 no components at [4]
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("late_transparent_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_render_trans_ecs_query_desc
(
  "animchar_render_trans_ecs_query",
  make_span(animchar_render_trans_ecs_query_comps+0, 1)/*rw*/,
  make_span(animchar_render_trans_ecs_query_comps+1, 2)/*ro*/,
  make_span(animchar_render_trans_ecs_query_comps+3, 1)/*rq*/,
  make_span(animchar_render_trans_ecs_query_comps+4, 2)/*no*/);
template<typename Callable>
inline void animchar_render_trans_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_render_trans_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_render_trans_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_render_trans_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(animchar_render_trans_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc send_on_rttr_changed_event_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("glass_gbuffer_renderer_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc send_on_rttr_changed_event_ecs_query_desc
(
  "send_on_rttr_changed_event_ecs_query",
  empty_span(),
  make_span(send_on_rttr_changed_event_ecs_query_comps+0, 1)/*ro*/,
  make_span(send_on_rttr_changed_event_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void send_on_rttr_changed_event_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, send_on_rttr_changed_event_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(send_on_rttr_changed_event_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
