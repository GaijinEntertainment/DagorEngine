#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t level_spline__name_get_type();
static ecs::LTComponentList level_spline__name_component(ECS_HASH("level_spline__name"), level_spline__name_get_type(), "prog/gameLibs/ecs/game/zones/./levelRegionsES.cpp.inl", "", 0);
static constexpr ecs::component_t level_spline__points_get_type();
static ecs::LTComponentList level_spline__points_component(ECS_HASH("level_spline__points"), level_spline__points_get_type(), "prog/gameLibs/ecs/game/zones/./levelRegionsES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "levelRegionsES.cpp.inl"
ECS_DEF_PULL_VAR(levelRegions);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc level_regions_debug_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("level_regions"), ecs::ComponentTypeInfo<LevelRegions>()}
};
static void level_regions_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    level_regions_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(level_regions_debug_es_comps, "level_regions", LevelRegions)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc level_regions_debug_es_es_desc
(
  "level_regions_debug_es",
  "prog/gameLibs/ecs/game/zones/./levelRegionsES.cpp.inl",
  ecs::EntitySystemOps(level_regions_debug_es_all),
  empty_span(),
  make_span(level_regions_debug_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc level_splines_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("level_spline__name"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static ecs::CompileTimeQueryDesc level_splines_ecs_query_desc
(
  "level_splines_ecs_query",
  empty_span(),
  make_span(level_splines_ecs_query_comps+0, 1)/*ro*/,
  make_span(level_splines_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void level_splines_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, level_splines_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(level_splines_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t level_spline__name_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
static constexpr ecs::component_t level_spline__points_get_type(){return ecs::ComponentTypeInfo<ecs::List<Point3>>::type; }
