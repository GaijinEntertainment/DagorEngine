#include "screenEffectRendererES.cpp.inl"
ECS_DEF_PULL_VAR(screenEffectRenderer);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc screen_effect_renderer_init_es_event_handler_comps[] ={};
static void screen_effect_renderer_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  screen_effect_renderer_init_es_event_handler(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc screen_effect_renderer_init_es_event_handler_es_desc
(
  "screen_effect_renderer_init_es",
  "prog/daNetGameLibs/screen_effect/render/screenEffectRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_effect_renderer_init_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc screen_effect_renderer_close_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("screen_effect__texVars"), ecs::ComponentTypeInfo<ecs::IntList>()}
};
static void screen_effect_renderer_close_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    screen_effect_renderer_close_es(evt
        , ECS_RO_COMP(screen_effect_renderer_close_es_comps, "screen_effect__texVars", ecs::IntList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc screen_effect_renderer_close_es_es_desc
(
  "screen_effect_renderer_close_es",
  "prog/daNetGameLibs/screen_effect/render/screenEffectRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_effect_renderer_close_es_all_events),
  empty_span(),
  make_span(screen_effect_renderer_close_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc screen_effect_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("screen_effect__is_active"), ecs::ComponentTypeInfo<bool>()},
//start of 4 ro components at [1]
  {ECS_HASH("screen_effect__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("screen_effect__countVar"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("screen_effect__texVars"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("screen_effect_renderer__enable"), ecs::ComponentTypeInfo<bool>()}
};
static void screen_effect_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    screen_effect_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(screen_effect_render_es_comps, "screen_effect__buffer", UniqueBufHolder)
    , ECS_RO_COMP(screen_effect_render_es_comps, "screen_effect__countVar", int)
    , ECS_RO_COMP(screen_effect_render_es_comps, "screen_effect__texVars", ecs::IntList)
    , ECS_RO_COMP(screen_effect_render_es_comps, "screen_effect_renderer__enable", bool)
    , ECS_RW_COMP(screen_effect_render_es_comps, "screen_effect__is_active", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc screen_effect_render_es_es_desc
(
  "screen_effect_render_es",
  "prog/daNetGameLibs/screen_effect/render/screenEffectRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_effect_render_es_all_events),
  make_span(screen_effect_render_es_comps+0, 1)/*rw*/,
  make_span(screen_effect_render_es_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc screen_effect_node_enable_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("screenEffect"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
//start of 1 ro components at [1]
  {ECS_HASH("screen_effect__is_active"), ecs::ComponentTypeInfo<bool>()}
};
static void screen_effect_node_enable_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(screen_effect_node_enable_es_comps, "screen_effect__is_active", bool)) )
      continue;
    screen_effect_node_enable_es(evt
          , ECS_RW_COMP(screen_effect_node_enable_es_comps, "screenEffect", resource_slot::NodeHandleWithSlotsAccess)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc screen_effect_node_enable_es_es_desc
(
  "screen_effect_node_enable_es",
  "prog/daNetGameLibs/screen_effect/render/screenEffectRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_effect_node_enable_es_all_events),
  make_span(screen_effect_node_enable_es_comps+0, 1)/*rw*/,
  make_span(screen_effect_node_enable_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","screen_effect__is_active");
static constexpr ecs::ComponentDesc screen_effect_node_disable_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("screenEffect"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
//start of 1 ro components at [1]
  {ECS_HASH("screen_effect__is_active"), ecs::ComponentTypeInfo<bool>()}
};
static void screen_effect_node_disable_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RO_COMP(screen_effect_node_disable_es_comps, "screen_effect__is_active", bool)) )
      continue;
    screen_effect_node_disable_es(evt
          , ECS_RW_COMP(screen_effect_node_disable_es_comps, "screenEffect", resource_slot::NodeHandleWithSlotsAccess)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc screen_effect_node_disable_es_es_desc
(
  "screen_effect_node_disable_es",
  "prog/daNetGameLibs/screen_effect/render/screenEffectRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_effect_node_disable_es_all_events),
  make_span(screen_effect_node_disable_es_comps+0, 1)/*rw*/,
  make_span(screen_effect_node_disable_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","screen_effect__is_active");
static constexpr ecs::ComponentDesc screen_effect_renderer_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("screen_effect__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("screen_effect__countVar"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("screen_effect__texVars"), ecs::ComponentTypeInfo<ecs::IntList>()},
//start of 1 ro components at [3]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc screen_effect_renderer_ecs_query_desc
(
  "screen_effect_renderer_ecs_query",
  make_span(screen_effect_renderer_ecs_query_comps+0, 3)/*rw*/,
  make_span(screen_effect_renderer_ecs_query_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void screen_effect_renderer_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, screen_effect_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(screen_effect_renderer_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(screen_effect_renderer_ecs_query_comps, "screen_effect__buffer", UniqueBufHolder)
            , ECS_RW_COMP(screen_effect_renderer_ecs_query_comps, "screen_effect__countVar", int)
            , ECS_RW_COMP(screen_effect_renderer_ecs_query_comps, "screen_effect__texVars", ecs::IntList)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_screen_effect_renderer_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("screen_effect__texture"), ecs::ComponentTypeInfo<SharedTex>()},
  {ECS_HASH("screen_effect__diffuse"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("screen_effect__uvScale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_effect__roughness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_effect__opacity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_effect__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_effect__weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_effect__borderOffset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_effect__borderSaturation"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_screen_effect_renderer_ecs_query_desc
(
  "get_screen_effect_renderer_ecs_query",
  empty_span(),
  make_span(get_screen_effect_renderer_ecs_query_comps+0, 9)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_screen_effect_renderer_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_screen_effect_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__texture", SharedTex)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__diffuse", Point4)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__uvScale", float)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__roughness", float)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__opacity", float)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__intensity", float)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__weight", float)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__borderOffset", float)
            , ECS_RO_COMP(get_screen_effect_renderer_ecs_query_comps, "screen_effect__borderSaturation", float)
            );

        }while (++comp != compE);
    }
  );
}
