#include "screenFrostRendererES.cpp.inl"
ECS_DEF_PULL_VAR(screenFrostRenderer);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc screen_frost_renderer_init_es_event_handler_comps[] ={};
static void screen_frost_renderer_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  screen_frost_renderer_init_es_event_handler(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc screen_frost_renderer_init_es_event_handler_es_desc
(
  "screen_frost_renderer_init_es",
  "prog/daNetGameLibs/screen_frost/render/screenFrostRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_frost_renderer_init_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc screen_frost_renderer_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("frost_tex"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("corruption_tex"), ecs::ComponentTypeInfo<SharedTexHolder>()}
};
static ecs::CompileTimeQueryDesc screen_frost_renderer_ecs_query_desc
(
  "screen_frost_renderer_ecs_query",
  empty_span(),
  make_span(screen_frost_renderer_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void screen_frost_renderer_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, screen_frost_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(screen_frost_renderer_ecs_query_comps, "frost_tex", SharedTexHolder)
            , ECS_RO_COMP(screen_frost_renderer_ecs_query_comps, "corruption_tex", SharedTexHolder)
            );

        }
    }
  );
}
