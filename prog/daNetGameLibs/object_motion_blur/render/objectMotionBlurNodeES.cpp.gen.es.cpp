#include "objectMotionBlurNodeES.cpp.inl"
ECS_DEF_PULL_VAR(objectMotionBlurNode);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc calc_resolution_for_motion_blur_es_comps[] ={};
static void calc_resolution_for_motion_blur_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  calc_resolution_for_motion_blur_es(static_cast<const SetResolutionEvent&>(evt)
        );
}
static ecs::EntitySystemDesc calc_resolution_for_motion_blur_es_es_desc
(
  "calc_resolution_for_motion_blur_es",
  "prog/daNetGameLibs/object_motion_blur/render/objectMotionBlurNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, calc_resolution_for_motion_blur_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_object_motion_blur_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__motionBlur"), ecs::ComponentTypeInfo<bool>()}
};
static void init_object_motion_blur_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_object_motion_blur_es(evt
        , ECS_RO_COMP(init_object_motion_blur_es_comps, "render_settings__motionBlur", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_object_motion_blur_es_es_desc
(
  "init_object_motion_blur_es",
  "prog/daNetGameLibs/object_motion_blur/render/objectMotionBlurNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_object_motion_blur_es_all_events),
  empty_span(),
  make_span(init_object_motion_blur_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__motionBlur");
static constexpr ecs::ComponentDesc close_object_motion_blur_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("object_motion_blur__render_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void close_object_motion_blur_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  close_object_motion_blur_es(evt
        );
}
static ecs::EntitySystemDesc close_object_motion_blur_es_es_desc
(
  "close_object_motion_blur_es",
  "prog/daNetGameLibs/object_motion_blur/render/objectMotionBlurNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, close_object_motion_blur_es_all_events),
  empty_span(),
  empty_span(),
  make_span(close_object_motion_blur_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc object_motion_blur_settings_appear_es_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("object_motion_blur_settings__max_blur_px"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("object_motion_blur_settings__max_samples"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("object_motion_blur_settings__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur_settings__vignette_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur_settings__ramp_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur_settings__ramp_cutoff"), ecs::ComponentTypeInfo<float>()}
};
static void object_motion_blur_settings_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    object_motion_blur_settings_appear_es(evt
        , ECS_RO_COMP(object_motion_blur_settings_appear_es_comps, "object_motion_blur_settings__max_blur_px", int)
    , ECS_RO_COMP(object_motion_blur_settings_appear_es_comps, "object_motion_blur_settings__max_samples", int)
    , ECS_RO_COMP(object_motion_blur_settings_appear_es_comps, "object_motion_blur_settings__strength", float)
    , ECS_RO_COMP(object_motion_blur_settings_appear_es_comps, "object_motion_blur_settings__vignette_strength", float)
    , ECS_RO_COMP(object_motion_blur_settings_appear_es_comps, "object_motion_blur_settings__ramp_strength", float)
    , ECS_RO_COMP(object_motion_blur_settings_appear_es_comps, "object_motion_blur_settings__ramp_cutoff", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc object_motion_blur_settings_appear_es_es_desc
(
  "object_motion_blur_settings_appear_es",
  "prog/daNetGameLibs/object_motion_blur/render/objectMotionBlurNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, object_motion_blur_settings_appear_es_all_events),
  empty_span(),
  make_span(object_motion_blur_settings_appear_es_comps+0, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc object_motion_blur_node_init_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("object_motion_blur__render_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
//start of 6 ro components at [1]
  {ECS_HASH("object_motion_blur__max_blur_px"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("object_motion_blur__max_samples"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("object_motion_blur__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur__vignette_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur__ramp_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur__ramp_cutoff"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc object_motion_blur_node_init_ecs_query_desc
(
  "object_motion_blur_node_init_ecs_query",
  make_span(object_motion_blur_node_init_ecs_query_comps+0, 1)/*rw*/,
  make_span(object_motion_blur_node_init_ecs_query_comps+1, 6)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void object_motion_blur_node_init_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, object_motion_blur_node_init_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__render_node", dabfg::NodeHandle)
            , ECS_RO_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__max_blur_px", int)
            , ECS_RO_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__max_samples", int)
            , ECS_RO_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__strength", float)
            , ECS_RO_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__vignette_strength", float)
            , ECS_RO_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__ramp_strength", float)
            , ECS_RO_COMP(object_motion_blur_node_init_ecs_query_comps, "object_motion_blur__ramp_cutoff", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc object_motion_blur_init_settings_ecs_query_comps[] =
{
//start of 6 rw components at [0]
  {ECS_HASH("object_motion_blur__max_blur_px"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("object_motion_blur__max_samples"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("object_motion_blur__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur__vignette_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur__ramp_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("object_motion_blur__ramp_cutoff"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc object_motion_blur_init_settings_ecs_query_desc
(
  "object_motion_blur_init_settings_ecs_query",
  make_span(object_motion_blur_init_settings_ecs_query_comps+0, 6)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void object_motion_blur_init_settings_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, object_motion_blur_init_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(object_motion_blur_init_settings_ecs_query_comps, "object_motion_blur__max_blur_px", int)
            , ECS_RW_COMP(object_motion_blur_init_settings_ecs_query_comps, "object_motion_blur__max_samples", int)
            , ECS_RW_COMP(object_motion_blur_init_settings_ecs_query_comps, "object_motion_blur__strength", float)
            , ECS_RW_COMP(object_motion_blur_init_settings_ecs_query_comps, "object_motion_blur__vignette_strength", float)
            , ECS_RW_COMP(object_motion_blur_init_settings_ecs_query_comps, "object_motion_blur__ramp_strength", float)
            , ECS_RW_COMP(object_motion_blur_init_settings_ecs_query_comps, "object_motion_blur__ramp_cutoff", float)
            );

        }
    }
  );
}
