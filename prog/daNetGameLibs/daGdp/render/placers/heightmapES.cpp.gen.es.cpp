#include "heightmapES.cpp.inl"
ECS_DEF_PULL_VAR(heightmap);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc heightmap_density_mask_disappeared_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("dagdp_density_mask"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void heightmap_density_mask_disappeared_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::heightmap_density_mask_disappeared_es(evt
        );
}
static ecs::EntitySystemDesc heightmap_density_mask_disappeared_es_es_desc
(
  "heightmap_density_mask_disappeared_es",
  "prog/daNetGameLibs/daGdp/render/placers/heightmapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heightmap_density_mask_disappeared_es_all_events),
  empty_span(),
  empty_span(),
  make_span(heightmap_density_mask_disappeared_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc heightmap_initialize_density_mask_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__heightmap_manager"), ecs::ComponentTypeInfo<dagdp::HeightmapManager>()}
};
static void heightmap_initialize_density_mask_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventInitialize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::heightmap_initialize_density_mask_es(static_cast<const dagdp::EventInitialize&>(evt)
        , ECS_RW_COMP(heightmap_initialize_density_mask_es_comps, "dagdp__heightmap_manager", dagdp::HeightmapManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc heightmap_initialize_density_mask_es_es_desc
(
  "heightmap_initialize_density_mask_es",
  "prog/daNetGameLibs/daGdp/render/placers/heightmapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heightmap_initialize_density_mask_es_all_events),
  make_span(heightmap_initialize_density_mask_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventInitialize>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc heightmap_view_process_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__heightmap_manager"), ecs::ComponentTypeInfo<dagdp::HeightmapManager>()}
};
static void heightmap_view_process_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewProcess>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::heightmap_view_process_es(static_cast<const dagdp::EventViewProcess&>(evt)
        , ECS_RW_COMP(heightmap_view_process_es_comps, "dagdp__heightmap_manager", dagdp::HeightmapManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc heightmap_view_process_es_es_desc
(
  "heightmap_view_process_es",
  "prog/daNetGameLibs/daGdp/render/placers/heightmapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heightmap_view_process_es_all_events),
  make_span(heightmap_view_process_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewProcess>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc heightmap_view_finalize_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__heightmap_manager"), ecs::ComponentTypeInfo<dagdp::HeightmapManager>()}
};
static void heightmap_view_finalize_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewFinalize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::heightmap_view_finalize_es(static_cast<const dagdp::EventViewFinalize&>(evt)
        , ECS_RW_COMP(heightmap_view_finalize_es_comps, "dagdp__heightmap_manager", dagdp::HeightmapManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc heightmap_view_finalize_es_es_desc
(
  "heightmap_view_finalize_es",
  "prog/daNetGameLibs/daGdp/render/placers/heightmapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heightmap_view_finalize_es_all_events),
  make_span(heightmap_view_finalize_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewFinalize>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc dagdp_placer_heightmap_changed_es_comps[] =
{
//start of 9 rq components at [0]
  {ECS_HASH("dagdp__heightmap_allow_unoptimal_grids"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__heightmap_lower_level"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__biomes"), ecs::ComponentTypeInfo<ecs::List<int>>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp_placer_heightmap"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("dagdp__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__heightmap_cell_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__jitter"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__seed"), ecs::ComponentTypeInfo<int>()}
};
static void dagdp_placer_heightmap_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::dagdp_placer_heightmap_changed_es(evt
        );
}
static ecs::EntitySystemDesc dagdp_placer_heightmap_changed_es_es_desc
(
  "dagdp_placer_heightmap_changed_es",
  "prog/daNetGameLibs/daGdp/render/placers/heightmapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_placer_heightmap_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dagdp_placer_heightmap_changed_es_comps+0, 9)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__biomes,dagdp__density,dagdp__heightmap_allow_unoptimal_grids,dagdp__heightmap_cell_size,dagdp__heightmap_lower_level,dagdp__jitter,dagdp__name,dagdp__seed");
static constexpr ecs::ComponentDesc manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc manager_ecs_query_desc
(
  "dagdp::manager_ecs_query",
  make_span(manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(manager_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc heightmap_density_masks_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("dagdp__density_mask_res"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp__density_mask_left_top_right_bottom"), ecs::ComponentTypeInfo<Point4>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_density_mask"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc heightmap_density_masks_ecs_query_desc
(
  "dagdp::heightmap_density_masks_ecs_query",
  empty_span(),
  make_span(heightmap_density_masks_ecs_query_comps+0, 2)/*ro*/,
  make_span(heightmap_density_masks_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::heightmap_density_masks_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, heightmap_density_masks_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(heightmap_density_masks_ecs_query_comps, "dagdp__density_mask_res", ecs::string)
            , ECS_RO_COMP(heightmap_density_masks_ecs_query_comps, "dagdp__density_mask_left_top_right_bottom", Point4)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc heightmap_placers_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__biomes"), ecs::ComponentTypeInfo<ecs::List<int>>()},
  {ECS_HASH("dagdp__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__seed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dagdp__jitter"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__heightmap_lower_level"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__heightmap_allow_unoptimal_grids"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__heightmap_cell_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [9]
  {ECS_HASH("dagdp_placer_heightmap"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc heightmap_placers_ecs_query_desc
(
  "dagdp::heightmap_placers_ecs_query",
  empty_span(),
  make_span(heightmap_placers_ecs_query_comps+0, 9)/*ro*/,
  make_span(heightmap_placers_ecs_query_comps+9, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::heightmap_placers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, heightmap_placers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(heightmap_placers_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__biomes", ecs::List<int>)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__density", float)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__seed", int)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__jitter", float)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__heightmap_lower_level", bool)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__heightmap_allow_unoptimal_grids", bool)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__heightmap_cell_size", float)
            , ECS_RO_COMP(heightmap_placers_ecs_query_comps, "dagdp__name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
