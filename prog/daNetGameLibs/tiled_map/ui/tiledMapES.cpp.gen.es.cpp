// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "tiledMapES.cpp.inl"
ECS_DEF_PULL_VAR(tiledMap);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc hud_tiled_map_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("tiled_map"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void hud_tiled_map_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RenderEventUI>());
  hud_tiled_map_es(static_cast<const RenderEventUI&>(evt)
        );
}
static ecs::EntitySystemDesc hud_tiled_map_es_es_desc
(
  "hud_tiled_map_es",
  "prog/daNetGameLibs/tiled_map/ui/tiledMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hud_tiled_map_es_all_events),
  empty_span(),
  empty_span(),
  make_span(hud_tiled_map_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RenderEventUI>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc hud_tiled_map_fog_of_war_update_data_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("fog_of_war__data"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("fog_of_war__dataGen"), ecs::ComponentTypeInfo<int>()}
};
static void hud_tiled_map_fog_of_war_update_data_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    hud_tiled_map_fog_of_war_update_data_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(hud_tiled_map_fog_of_war_update_data_es_comps, "fog_of_war__data", ecs::IntList)
    , ECS_RO_COMP(hud_tiled_map_fog_of_war_update_data_es_comps, "fog_of_war__dataGen", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc hud_tiled_map_fog_of_war_update_data_es_es_desc
(
  "hud_tiled_map_fog_of_war_update_data_es",
  "prog/daNetGameLibs/tiled_map/ui/tiledMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hud_tiled_map_fog_of_war_update_data_es_all_events),
  empty_span(),
  make_span(hud_tiled_map_fog_of_war_update_data_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"hud_tiled_map_fog_of_war_es");
//static constexpr ecs::ComponentDesc hud_tiled_map_fog_of_war_es_comps[] ={};
static void hud_tiled_map_fog_of_war_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  hud_tiled_map_fog_of_war_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc hud_tiled_map_fog_of_war_es_es_desc
(
  "hud_tiled_map_fog_of_war_es",
  "prog/daNetGameLibs/tiled_map/ui/tiledMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hud_tiled_map_fog_of_war_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc tiled_map_fog_of_war_after_reset_es_comps[] ={};
static void tiled_map_fog_of_war_after_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  tiled_map_fog_of_war_after_reset_es(static_cast<const AfterDeviceReset&>(evt)
        );
}
static ecs::EntitySystemDesc tiled_map_fog_of_war_after_reset_es_es_desc
(
  "tiled_map_fog_of_war_after_reset_es",
  "prog/daNetGameLibs/tiled_map/ui/tiledMapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tiled_map_fog_of_war_after_reset_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc tiled_map_fog_of_war_get_data_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("fog_of_war__data"), ecs::ComponentTypeInfo<ecs::IntList>()}
};
static ecs::CompileTimeQueryDesc tiled_map_fog_of_war_get_data_ecs_query_desc
(
  "tiled_map_fog_of_war_get_data_ecs_query",
  empty_span(),
  make_span(tiled_map_fog_of_war_get_data_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void tiled_map_fog_of_war_get_data_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, tiled_map_fog_of_war_get_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(tiled_map_fog_of_war_get_data_ecs_query_comps, "fog_of_war__data", ecs::IntList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc tiled_map_fog_of_war_set_data_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("fog_of_war__data"), ecs::ComponentTypeInfo<ecs::IntList>()}
};
static ecs::CompileTimeQueryDesc tiled_map_fog_of_war_set_data_ecs_query_desc
(
  "tiled_map_fog_of_war_set_data_ecs_query",
  make_span(tiled_map_fog_of_war_set_data_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void tiled_map_fog_of_war_set_data_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, tiled_map_fog_of_war_set_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(tiled_map_fog_of_war_set_data_ecs_query_comps, "fog_of_war__data", ecs::IntList)
            );

        }while (++comp != compE);
    }
  );
}
