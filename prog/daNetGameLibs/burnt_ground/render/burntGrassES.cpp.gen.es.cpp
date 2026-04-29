// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "burntGrassES.cpp.inl"
ECS_DEF_PULL_VAR(burntGrass);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc burnt_grass_renderer_on_appear_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("burnt_grass_renderer"), ecs::ComponentTypeInfo<BurntGrassRenderer>()},
  {ECS_HASH("burnt_grass_renderer__prepare_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void burnt_grass_renderer_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_grass_renderer_on_appear_es(evt
        , ECS_RW_COMP(burnt_grass_renderer_on_appear_es_comps, "burnt_grass_renderer", BurntGrassRenderer)
    , ECS_RW_COMP(burnt_grass_renderer_on_appear_es_comps, "burnt_grass_renderer__prepare_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_grass_renderer_on_appear_es_es_desc
(
  "burnt_grass_renderer_on_appear_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGrassES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_grass_renderer_on_appear_es_all_events),
  make_span(burnt_grass_renderer_on_appear_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc burnt_grass_renderer_set_up_biomes_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("burnt_grass_renderer"), ecs::ComponentTypeInfo<BurntGrassRenderer>()},
//start of 1 ro components at [1]
  {ECS_HASH("burnt_grass_renderer__biomeNames"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void burnt_grass_renderer_set_up_biomes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_grass_renderer_set_up_biomes_es(evt
        , ECS_RW_COMP(burnt_grass_renderer_set_up_biomes_es_comps, "burnt_grass_renderer", BurntGrassRenderer)
    , ECS_RO_COMP(burnt_grass_renderer_set_up_biomes_es_comps, "burnt_grass_renderer__biomeNames", ecs::StringList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_grass_renderer_set_up_biomes_es_es_desc
(
  "burnt_grass_renderer_set_up_biomes_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGrassES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_grass_renderer_set_up_biomes_es_all_events),
  make_span(burnt_grass_renderer_set_up_biomes_es_comps+0, 1)/*rw*/,
  make_span(burnt_grass_renderer_set_up_biomes_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc burnt_grass__on_fire_appear_event_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("burnt_grass_renderer"), ecs::ComponentTypeInfo<BurntGrassRenderer>()}
};
static void burnt_grass__on_fire_appear_event_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<FireOnGroundEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_grass__on_fire_appear_event_es(static_cast<const FireOnGroundEvent&>(evt)
        , ECS_RW_COMP(burnt_grass__on_fire_appear_event_es_comps, "burnt_grass_renderer", BurntGrassRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_grass__on_fire_appear_event_es_es_desc
(
  "burnt_grass__on_fire_appear_event_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGrassES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_grass__on_fire_appear_event_es_all_events),
  make_span(burnt_grass__on_fire_appear_event_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<FireOnGroundEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_burnt_grass_renderer_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("burnt_grass_renderer"), ecs::ComponentTypeInfo<BurntGrassRenderer>()}
};
static ecs::CompileTimeQueryDesc get_burnt_grass_renderer_ecs_query_desc
(
  "get_burnt_grass_renderer_ecs_query",
  make_span(get_burnt_grass_renderer_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_burnt_grass_renderer_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_burnt_grass_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_burnt_grass_renderer_ecs_query_comps, "burnt_grass_renderer", BurntGrassRenderer)
            );

        }while (++comp != compE);
    }
  );
}
