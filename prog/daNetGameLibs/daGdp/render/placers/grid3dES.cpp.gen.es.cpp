// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "grid3dES.cpp.inl"
ECS_DEF_PULL_VAR(grid3d);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc grid3d_view_process_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__grid3d_manager"), ecs::ComponentTypeInfo<dagdp::Grid3dManager>()}
};
static void grid3d_view_process_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewProcess>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::grid3d_view_process_es(static_cast<const dagdp::EventViewProcess&>(evt)
        , components.manager()
    , ECS_RW_COMP(grid3d_view_process_es_comps, "dagdp__grid3d_manager", dagdp::Grid3dManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid3d_view_process_es_es_desc
(
  "grid3d_view_process_es",
  "prog/daNetGameLibs/daGdp/render/placers/grid3dES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid3d_view_process_es_all_events),
  make_span(grid3d_view_process_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewProcess>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc grid3d_view_finalize_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__grid3d_manager"), ecs::ComponentTypeInfo<dagdp::Grid3dManager>()}
};
static void grid3d_view_finalize_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewFinalize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::grid3d_view_finalize_es(static_cast<const dagdp::EventViewFinalize&>(evt)
        , ECS_RW_COMP(grid3d_view_finalize_es_comps, "dagdp__grid3d_manager", dagdp::Grid3dManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid3d_view_finalize_es_es_desc
(
  "grid3d_view_finalize_es",
  "prog/daNetGameLibs/daGdp/render/placers/grid3dES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid3d_view_finalize_es_all_events),
  make_span(grid3d_view_finalize_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewFinalize>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc dagdp_placer_grid3d_changed_es_comps[] =
{
//start of 12 rq components at [0]
  {ECS_HASH("dagdp__volume_axis_abs"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__volume_axis_local"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__distance_based_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("dagdp__distance_based_center"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("dagdp__volume_axis"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp_placer_grid3d"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("dagdp__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__distance_based_range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__jitter"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__csm_cascade_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dagdp__seed"), ecs::ComponentTypeInfo<int>()}
};
static void dagdp_placer_grid3d_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  dagdp::dagdp_placer_grid3d_changed_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc dagdp_placer_grid3d_changed_es_es_desc
(
  "dagdp_placer_grid3d_changed_es",
  "prog/daNetGameLibs/daGdp/render/placers/grid3dES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_placer_grid3d_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dagdp_placer_grid3d_changed_es_comps+0, 12)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__csm_cascade_count,dagdp__density,dagdp__distance_based_center,dagdp__distance_based_range,dagdp__distance_based_scale,dagdp__jitter,dagdp__name,dagdp__seed,dagdp__volume_axis,dagdp__volume_axis_abs,dagdp__volume_axis_local");
static constexpr ecs::ComponentDesc dagdp_grid3d_volume_link_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_placer_grid3d"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dagdp_grid3d_volume_link_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_grid3d_volume_link_es(evt
        , components.manager()
    , ECS_RO_COMP(dagdp_grid3d_volume_link_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(dagdp_grid3d_volume_link_es_comps, "dagdp__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_grid3d_volume_link_es_es_desc
(
  "dagdp_grid3d_volume_link_es",
  "prog/daNetGameLibs/daGdp/render/placers/grid3dES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_grid3d_volume_link_es_all_events),
  empty_span(),
  make_span(dagdp_grid3d_volume_link_es_comps+0, 2)/*ro*/,
  make_span(dagdp_grid3d_volume_link_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","dagdp__name");
static constexpr ecs::ComponentDesc dagdp_grid3d_volume_unlink_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [1]
  {ECS_HASH("dagdp_placer_grid3d"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dagdp_grid3d_volume_unlink_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_grid3d_volume_unlink_es(evt
        , components.manager()
    , ECS_RO_COMP(dagdp_grid3d_volume_unlink_es_comps, "dagdp__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_grid3d_volume_unlink_es_es_desc
(
  "dagdp_grid3d_volume_unlink_es",
  "prog/daNetGameLibs/daGdp/render/placers/grid3dES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_grid3d_volume_unlink_es_all_events),
  empty_span(),
  make_span(dagdp_grid3d_volume_unlink_es_comps+0, 1)/*ro*/,
  make_span(dagdp_grid3d_volume_unlink_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_grid3d_volume_changed_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp_internal__volume_placer_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 ro components at [1]
  {ECS_HASH("dagdp__volume_placer_name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_volume"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dagdp_grid3d_volume_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_grid3d_volume_changed_es(evt
        , components.manager()
    , ECS_RO_COMP(dagdp_grid3d_volume_changed_es_comps, "dagdp__volume_placer_name", ecs::string)
    , ECS_RW_COMP(dagdp_grid3d_volume_changed_es_comps, "dagdp_internal__volume_placer_eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_grid3d_volume_changed_es_es_desc
(
  "dagdp_grid3d_volume_changed_es",
  "prog/daNetGameLibs/daGdp/render/placers/grid3dES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_grid3d_volume_changed_es_all_events),
  make_span(dagdp_grid3d_volume_changed_es_comps+0, 1)/*rw*/,
  make_span(dagdp_grid3d_volume_changed_es_comps+1, 1)/*ro*/,
  make_span(dagdp_grid3d_volume_changed_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","dagdp__volume_placer_name");
static constexpr ecs::ComponentDesc grid3d_placers_ecs_query_comps[] =
{
//start of 12 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp__volume_axis"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("dagdp__volume_axis_local"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__volume_axis_abs"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp__distance_based_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("dagdp__distance_based_range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__distance_based_center"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("dagdp__jitter"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__seed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dagdp__csm_cascade_count"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [12]
  {ECS_HASH("dagdp_placer_grid3d"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc grid3d_placers_ecs_query_desc
(
  "dagdp::grid3d_placers_ecs_query",
  empty_span(),
  make_span(grid3d_placers_ecs_query_comps+0, 12)/*ro*/,
  make_span(grid3d_placers_ecs_query_comps+12, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::grid3d_placers_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, grid3d_placers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(grid3d_placers_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__density", float)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__name", ecs::string)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__volume_axis", Point3)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__volume_axis_local", bool)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__volume_axis_abs", bool)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__distance_based_scale", Point2)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__distance_based_range", float)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__distance_based_center", Point3)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__jitter", float)
            , ECS_RO_COMP(grid3d_placers_ecs_query_comps, "dagdp__seed", int)
            , ECS_RO_COMP_OR(grid3d_placers_ecs_query_comps, "dagdp__csm_cascade_count", int(-1))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc volume_boxes_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("dagdp_internal__volume_placer_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_volume_box"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc volume_boxes_ecs_query_desc
(
  "dagdp::volume_boxes_ecs_query",
  empty_span(),
  make_span(volume_boxes_ecs_query_comps+0, 2)/*ro*/,
  make_span(volume_boxes_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::volume_boxes_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volume_boxes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volume_boxes_ecs_query_comps, "dagdp_internal__volume_placer_eid", ecs::EntityId)
            , ECS_RO_COMP(volume_boxes_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc volume_cylinders_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("dagdp_internal__volume_placer_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_volume_cylinder"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc volume_cylinders_ecs_query_desc
(
  "dagdp::volume_cylinders_ecs_query",
  empty_span(),
  make_span(volume_cylinders_ecs_query_comps+0, 2)/*ro*/,
  make_span(volume_cylinders_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::volume_cylinders_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volume_cylinders_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volume_cylinders_ecs_query_comps, "dagdp_internal__volume_placer_eid", ecs::EntityId)
            , ECS_RO_COMP(volume_cylinders_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc volume_spheres_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("dagdp_internal__volume_placer_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("sphere_zone__radius"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [3]
  {ECS_HASH("dagdp_volume_sphere"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc volume_spheres_ecs_query_desc
(
  "dagdp::volume_spheres_ecs_query",
  empty_span(),
  make_span(volume_spheres_ecs_query_comps+0, 3)/*ro*/,
  make_span(volume_spheres_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::volume_spheres_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volume_spheres_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volume_spheres_ecs_query_comps, "dagdp_internal__volume_placer_eid", ecs::EntityId)
            , ECS_RO_COMP(volume_spheres_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(volume_spheres_ecs_query_comps, "sphere_zone__radius", float)
            );

        }while (++comp != compE);
    }
  );
}
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
inline void dagdp::manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, manager_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc volumes_link_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp_internal__volume_placer_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 ro components at [1]
  {ECS_HASH("dagdp__volume_placer_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc volumes_link_ecs_query_desc
(
  "dagdp::volumes_link_ecs_query",
  make_span(volumes_link_ecs_query_comps+0, 1)/*rw*/,
  make_span(volumes_link_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::volumes_link_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volumes_link_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volumes_link_ecs_query_comps, "dagdp__volume_placer_name", ecs::string)
            , ECS_RW_COMP(volumes_link_ecs_query_comps, "dagdp_internal__volume_placer_eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc volume_placers_link_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc volume_placers_link_ecs_query_desc
(
  "dagdp::volume_placers_link_ecs_query",
  empty_span(),
  make_span(volume_placers_link_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::volume_placers_link_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volume_placers_link_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volume_placers_link_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(volume_placers_link_ecs_query_comps, "dagdp__name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
