// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "outlineRendererES.cpp.inl"
ECS_DEF_PULL_VAR(outlineRenderer);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_outline_rendrerer_locally_es_comps[] ={};
static void create_outline_rendrerer_locally_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  create_outline_rendrerer_locally_es(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc create_outline_rendrerer_locally_es_es_desc
(
  "create_outline_rendrerer_locally_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_outline_rendrerer_locally_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
);
static constexpr ecs::ComponentDesc outline_renderer_create_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()},
//start of 2 ro components at [1]
  {ECS_HASH("outline_blur_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("outline_brightness"), ecs::ComponentTypeInfo<float>()}
};
static void outline_renderer_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    outline_renderer_create_es_event_handler(evt
        , ECS_RW_COMP(outline_renderer_create_es_event_handler_comps, "outline_renderer", OutlineRenderer)
    , ECS_RO_COMP(outline_renderer_create_es_event_handler_comps, "outline_blur_width", float)
    , ECS_RO_COMP(outline_renderer_create_es_event_handler_comps, "outline_brightness", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc outline_renderer_create_es_event_handler_es_desc
(
  "outline_renderer_create_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, outline_renderer_create_es_event_handler_all_events),
  make_span(outline_renderer_create_es_event_handler_comps+0, 1)/*rw*/,
  make_span(outline_renderer_create_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc disable_outline_on_ri_destroyed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void disable_outline_on_ri_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventRiExtraDestroyed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    disable_outline_on_ri_destroyed_es_event_handler(static_cast<const EventRiExtraDestroyed&>(evt)
        , ECS_RW_COMP(disable_outline_on_ri_destroyed_es_event_handler_comps, "outline__enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc disable_outline_on_ri_destroyed_es_event_handler_es_desc
(
  "disable_outline_on_ri_destroyed_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_outline_on_ri_destroyed_es_event_handler_all_events),
  make_span(disable_outline_on_ri_destroyed_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRiExtraDestroyed>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc outline_prepare_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_ctxs"), ecs::ComponentTypeInfo<OutlineContexts>()}
};
static void outline_prepare_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    outline_prepare_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(outline_prepare_es_comps, "outline_ctxs", OutlineContexts)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc outline_prepare_es_es_desc
(
  "outline_prepare_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, outline_prepare_es_all_events),
  make_span(outline_prepare_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,nullptr,nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc outline_render_resolution_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()}
};
static void outline_render_resolution_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    outline_render_resolution_es_event_handler(static_cast<const SetResolutionEvent&>(evt)
        , ECS_RW_COMP(outline_render_resolution_es_event_handler_comps, "outline_renderer", OutlineRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc outline_render_resolution_es_event_handler_es_desc
(
  "outline_render_resolution_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, outline_render_resolution_es_event_handler_all_events),
  make_span(outline_render_resolution_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_outline_node_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("outline_prepare_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("outline_apply_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void create_outline_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_outline_node_es(evt
        , ECS_RW_COMP(create_outline_node_es_comps, "outline_prepare_node", dafg::NodeHandle)
    , ECS_RW_COMP(create_outline_node_es_comps, "outline_apply_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_outline_node_es_es_desc
(
  "create_outline_node_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_outline_node_es_all_events),
  make_span(create_outline_node_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc render_outline_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()},
  {ECS_HASH("outline_ctxs"), ecs::ComponentTypeInfo<OutlineContexts>()}
};
static ecs::CompileTimeQueryDesc render_outline_ecs_query_desc
(
  "render_outline_ecs_query",
  make_span(render_outline_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_outline_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_outline_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(render_outline_ecs_query_comps, "outline_renderer", OutlineRenderer)
            , ECS_RW_COMP(render_outline_ecs_query_comps, "outline_ctxs", OutlineContexts)
            );

        }while (++comp != compE);
    }
  );
}
