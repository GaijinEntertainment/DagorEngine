#include "grassEraseES.cpp.inl"
ECS_DEF_PULL_VAR(grassErase);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc grass_eraser_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("grass_erasers__spots"), ecs::ComponentTypeInfo<ecs::Point4List>()}
};
static void grass_eraser_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grass_eraser_es(evt
        , ECS_RO_COMP(grass_eraser_es_comps, "grass_erasers__spots", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grass_eraser_es_es_desc
(
  "grass_eraser_es",
  "prog/daNetGame/render/grass/grassEraseES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_eraser_es_all_events),
  empty_span(),
  make_span(grass_eraser_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc grass_eraser_destroy_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("grass_erasers__spots"), ecs::ComponentTypeInfo<ecs::Point4List>()}
};
static void grass_eraser_destroy_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  grass_eraser_destroy_es(evt
        );
}
static ecs::EntitySystemDesc grass_eraser_destroy_es_es_desc
(
  "grass_eraser_destroy_es",
  "prog/daNetGame/render/grass/grassEraseES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_eraser_destroy_es_all_events),
  empty_span(),
  empty_span(),
  make_span(grass_eraser_destroy_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc get_grass_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static ecs::CompileTimeQueryDesc get_grass_render_ecs_query_desc
(
  "get_grass_render_ecs_query",
  make_span(get_grass_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_grass_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_grass_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_grass_render_ecs_query_comps, "grass_render", GrassRenderer)
            );

        }while (++comp != compE);
    }
  );
}
