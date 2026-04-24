// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dynamicMirrorES.cpp.inl"
ECS_DEF_PULL_VAR(dynamicMirror);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc prepare_mirror_es_comps[] =
{
//start of 10 rw components at [0]
  {ECS_HASH("dynamic_mirror_renderer"), ecs::ComponentTypeInfo<DynamicMirrorRenderer>()},
  {ECS_HASH("dynamic_mirror_prepare_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_prepass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_end_prepass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_render_dynamic_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_render_static_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_render_ground_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_resolve_gbuf_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_resolve_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_mirror_envi_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void prepare_mirror_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    prepare_mirror_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_renderer", DynamicMirrorRenderer)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_prepare_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_prepass_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_end_prepass_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_render_dynamic_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_render_static_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_render_ground_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_resolve_gbuf_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_resolve_node", dafg::NodeHandle)
    , ECS_RW_COMP(prepare_mirror_es_comps, "dynamic_mirror_envi_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc prepare_mirror_es_es_desc
(
  "prepare_mirror_es",
  "prog/daNetGameLibs/dynamic_mirror/render/dynamicMirrorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, prepare_mirror_es_all_events),
  make_span(prepare_mirror_es_comps+0, 10)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc dynamic_mirror_appear_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__renderPriority"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [4]
  {ECS_HASH("dynamic_mirror_holder__mirrorNormal"), ecs::ComponentTypeInfo<Point3>()}
};
static void dynamic_mirror_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_mirror_appear_es_event_handler(evt
        , components.manager()
    , ECS_RO_COMP(dynamic_mirror_appear_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(dynamic_mirror_appear_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP_PTR(dynamic_mirror_appear_es_event_handler_comps, "additional_data", ecs::Point4List)
    , ECS_RO_COMP_OR(dynamic_mirror_appear_es_event_handler_comps, "animchar__renderPriority", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_mirror_appear_es_event_handler_es_desc
(
  "dynamic_mirror_appear_es",
  "prog/daNetGameLibs/dynamic_mirror/render/dynamicMirrorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_mirror_appear_es_event_handler_all_events),
  make_span(dynamic_mirror_appear_es_event_handler_comps+0, 1)/*rw*/,
  make_span(dynamic_mirror_appear_es_event_handler_comps+1, 3)/*ro*/,
  make_span(dynamic_mirror_appear_es_event_handler_comps+4, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dynamic_mirror_disappear_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("dynamic_mirror_holder__mirrorNormal"), ecs::ComponentTypeInfo<Point3>()}
};
static void dynamic_mirror_disappear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_mirror_disappear_es_event_handler(evt
        , components.manager()
    , ECS_RO_COMP(dynamic_mirror_disappear_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_mirror_disappear_es_event_handler_es_desc
(
  "dynamic_mirror_disappear_es",
  "prog/daNetGameLibs/dynamic_mirror/render/dynamicMirrorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_mirror_disappear_es_event_handler_all_events),
  empty_span(),
  make_span(dynamic_mirror_disappear_es_event_handler_comps+0, 1)/*ro*/,
  make_span(dynamic_mirror_disappear_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc animchar_mirror_before_render_es_comps[] ={};
static void animchar_mirror_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  animchar_mirror_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc animchar_mirror_before_render_es_es_desc
(
  "animchar_mirror_before_render_es",
  "prog/daNetGameLibs/dynamic_mirror/render/dynamicMirrorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_mirror_before_render_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc get_dynamic_mirror_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 3 ro components at [1]
  {ECS_HASH("dynamic_mirror_holder__mirrorNormal"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()}
};
static ecs::CompileTimeQueryDesc get_dynamic_mirror_ecs_query_desc
(
  "get_dynamic_mirror_ecs_query",
  make_span(get_dynamic_mirror_ecs_query_comps+0, 1)/*rw*/,
  make_span(get_dynamic_mirror_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_dynamic_mirror_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_dynamic_mirror_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_dynamic_mirror_ecs_query_comps, "dynamic_mirror_holder__mirrorNormal", Point3)
            , ECS_RO_COMP(get_dynamic_mirror_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(get_dynamic_mirror_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RW_COMP(get_dynamic_mirror_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_dynamic_mirror_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dynamic_mirror_renderer"), ecs::ComponentTypeInfo<DynamicMirrorRenderer>()}
};
static ecs::CompileTimeQueryDesc get_dynamic_mirror_render_ecs_query_desc
(
  "get_dynamic_mirror_render_ecs_query",
  make_span(get_dynamic_mirror_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_dynamic_mirror_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_dynamic_mirror_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_dynamic_mirror_render_ecs_query_comps, "dynamic_mirror_renderer", DynamicMirrorRenderer)
            );

        }while (++comp != compE);
    }
  );
}
