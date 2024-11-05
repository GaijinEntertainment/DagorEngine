#include "splineGenGeometryES.cpp.inl"
ECS_DEF_PULL_VAR(splineGenGeometry);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc spline_gen_geometry_select_lod_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("spline_gen_geometry_renderer"), ecs::ComponentTypeInfo<SplineGenGeometry>()},
  {ECS_HASH("spline_gen_geometry__lod"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("spline_gen_geometry__is_rendered"), ecs::ComponentTypeInfo<bool>()},
//start of 3 ro components at [3]
  {ECS_HASH("spline_gen_geometry__lods_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("spline_gen_geometry__radius"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("spline_gen_geometry__points"), ecs::ComponentTypeInfo<ecs::List<Point3>>()}
};
static void spline_gen_geometry_select_lod_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    spline_gen_geometry_select_lod_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(spline_gen_geometry_select_lod_es_comps, "spline_gen_geometry_renderer", SplineGenGeometry)
    , ECS_RO_COMP(spline_gen_geometry_select_lod_es_comps, "spline_gen_geometry__lods_eid", ecs::EntityId)
    , ECS_RW_COMP(spline_gen_geometry_select_lod_es_comps, "spline_gen_geometry__lod", int)
    , ECS_RW_COMP(spline_gen_geometry_select_lod_es_comps, "spline_gen_geometry__is_rendered", bool)
    , ECS_RO_COMP(spline_gen_geometry_select_lod_es_comps, "spline_gen_geometry__radius", Point3)
    , ECS_RO_COMP(spline_gen_geometry_select_lod_es_comps, "spline_gen_geometry__points", ecs::List<Point3>)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_select_lod_es_es_desc
(
  "spline_gen_geometry_select_lod_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_select_lod_es_all_events),
  make_span(spline_gen_geometry_select_lod_es_comps+0, 3)/*rw*/,
  make_span(spline_gen_geometry_select_lod_es_comps+3, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"spline_gen_geometry_manager_update_buffers_es");
static constexpr ecs::ComponentDesc spline_gen_geometry_manager_update_buffers_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_repository"), ecs::ComponentTypeInfo<SplineGenGeometryRepository>()}
};
static void spline_gen_geometry_manager_update_buffers_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    spline_gen_geometry_manager_update_buffers_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(spline_gen_geometry_manager_update_buffers_es_comps, "spline_gen_repository", SplineGenGeometryRepository)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_manager_update_buffers_es_es_desc
(
  "spline_gen_geometry_manager_update_buffers_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_manager_update_buffers_es_all_events),
  make_span(spline_gen_geometry_manager_update_buffers_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"spline_gen_geometry_update_instancing_data_es");
static constexpr ecs::ComponentDesc spline_gen_geometry_request_active_state_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spline_gen_geometry_renderer"), ecs::ComponentTypeInfo<SplineGenGeometry>()},
  {ECS_HASH("spline_gen_geometry__renderer_active"), ecs::ComponentTypeInfo<bool>()},
//start of 2 ro components at [2]
  {ECS_HASH("spline_gen_geometry__request_active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spline_gen_geometry__is_rendered"), ecs::ComponentTypeInfo<bool>()}
};
static void spline_gen_geometry_request_active_state_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(spline_gen_geometry_request_active_state_es_comps, "spline_gen_geometry__is_rendered", bool)) )
      continue;
    spline_gen_geometry_request_active_state_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(spline_gen_geometry_request_active_state_es_comps, "spline_gen_geometry_renderer", SplineGenGeometry)
      , ECS_RO_COMP(spline_gen_geometry_request_active_state_es_comps, "spline_gen_geometry__request_active", bool)
      , ECS_RW_COMP(spline_gen_geometry_request_active_state_es_comps, "spline_gen_geometry__renderer_active", bool)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_request_active_state_es_es_desc
(
  "spline_gen_geometry_request_active_state_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_request_active_state_es_all_events),
  make_span(spline_gen_geometry_request_active_state_es_comps+0, 2)/*rw*/,
  make_span(spline_gen_geometry_request_active_state_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"spline_gen_geometry_select_lod_es");
static constexpr ecs::ComponentDesc spline_gen_geometry_update_instancing_data_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spline_gen_geometry_renderer"), ecs::ComponentTypeInfo<SplineGenGeometry>()},
  {ECS_HASH("spline_gen_geometry__radius"), ecs::ComponentTypeInfo<Point3>()},
//start of 8 ro components at [2]
  {ECS_HASH("spline_gen_geometry__displacement_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spline_gen_geometry__tiles_around"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("spline_gen_geometry__tile_size_meters"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spline_gen_geometry__points"), ecs::ComponentTypeInfo<ecs::List<Point3>>()},
  {ECS_HASH("spline_gen_geometry__obj_size_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spline_gen_geometry__meter_between_objs"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spline_gen_geometry__is_rendered"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spline_gen_geometry__renderer_active"), ecs::ComponentTypeInfo<bool>()}
};
static void spline_gen_geometry_update_instancing_data_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__is_rendered", bool) && ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__renderer_active", bool)) )
      continue;
    spline_gen_geometry_update_instancing_data_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry_renderer", SplineGenGeometry)
      , ECS_RW_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__radius", Point3)
      , ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__displacement_strength", float)
      , ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__tiles_around", int)
      , ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__tile_size_meters", float)
      , ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__points", ecs::List<Point3>)
      , ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__obj_size_mul", float)
      , ECS_RO_COMP(spline_gen_geometry_update_instancing_data_es_comps, "spline_gen_geometry__meter_between_objs", float)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_update_instancing_data_es_es_desc
(
  "spline_gen_geometry_update_instancing_data_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_update_instancing_data_es_all_events),
  make_span(spline_gen_geometry_update_instancing_data_es_comps+0, 2)/*rw*/,
  make_span(spline_gen_geometry_update_instancing_data_es_comps+2, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"spline_gen_geometry_request_active_state_es");
static constexpr ecs::ComponentDesc spline_gen_geometry_update_inactive_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_geometry_renderer"), ecs::ComponentTypeInfo<SplineGenGeometry>()},
//start of 2 ro components at [1]
  {ECS_HASH("spline_gen_geometry__renderer_active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spline_gen_geometry__is_rendered"), ecs::ComponentTypeInfo<bool>()}
};
static void spline_gen_geometry_update_inactive_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RO_COMP(spline_gen_geometry_update_inactive_es_comps, "spline_gen_geometry__renderer_active", bool) && ECS_RO_COMP(spline_gen_geometry_update_inactive_es_comps, "spline_gen_geometry__is_rendered", bool)) )
      continue;
    spline_gen_geometry_update_inactive_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(spline_gen_geometry_update_inactive_es_comps, "spline_gen_geometry_renderer", SplineGenGeometry)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_update_inactive_es_es_desc
(
  "spline_gen_geometry_update_inactive_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_update_inactive_es_all_events),
  make_span(spline_gen_geometry_update_inactive_es_comps+0, 1)/*rw*/,
  make_span(spline_gen_geometry_update_inactive_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"spline_gen_geometry_update_instancing_data_es");
static constexpr ecs::ComponentDesc spline_gen_geometry_manager_generate_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_repository"), ecs::ComponentTypeInfo<SplineGenGeometryRepository>()}
};
static void spline_gen_geometry_manager_generate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    spline_gen_geometry_manager_generate_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(spline_gen_geometry_manager_generate_es_comps, "spline_gen_repository", SplineGenGeometryRepository)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_manager_generate_es_es_desc
(
  "spline_gen_geometry_manager_generate_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_manager_generate_es_all_events),
  make_span(spline_gen_geometry_manager_generate_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"spline_gen_geometry_update_inactive_es");
static constexpr ecs::ComponentDesc spline_gen_geometry_render_opaque_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_repository"), ecs::ComponentTypeInfo<SplineGenGeometryRepository>()}
};
static void spline_gen_geometry_render_opaque_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    spline_gen_geometry_render_opaque_es(static_cast<const UpdateStageInfoRender&>(evt)
        , ECS_RW_COMP(spline_gen_geometry_render_opaque_es_comps, "spline_gen_repository", SplineGenGeometryRepository)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_geometry_render_opaque_es_es_desc
(
  "spline_gen_geometry_render_opaque_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_geometry_render_opaque_es_all_events),
  make_span(spline_gen_geometry_render_opaque_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc register_to_lods_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("spline_gen_lods__lods_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc register_to_lods_ecs_query_desc
(
  "register_to_lods_ecs_query",
  empty_span(),
  make_span(register_to_lods_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void register_to_lods_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, register_to_lods_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(register_to_lods_ecs_query_comps, "spline_gen_lods__lods_name", ecs::string)
            , ECS_RO_COMP(register_to_lods_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_lod_data_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("spline_gen_lods__distances"), ecs::ComponentTypeInfo<ecs::FloatList>()},
  {ECS_HASH("spline_gen_lods__template_names"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc get_lod_data_ecs_query_desc
(
  "get_lod_data_ecs_query",
  empty_span(),
  make_span(get_lod_data_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_lod_data_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_lod_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_lod_data_ecs_query_comps, "spline_gen_lods__distances", ecs::FloatList)
            , ECS_RO_COMP(get_lod_data_ecs_query_comps, "spline_gen_lods__template_names", ecs::StringList)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc reset_spline_gen_geometry_renderer_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spline_gen_geometry_renderer"), ecs::ComponentTypeInfo<SplineGenGeometry>()},
  {ECS_HASH("spline_gen_geometry__lod"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc reset_spline_gen_geometry_renderer_ecs_query_desc
(
  "reset_spline_gen_geometry_renderer_ecs_query",
  make_span(reset_spline_gen_geometry_renderer_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void reset_spline_gen_geometry_renderer_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, reset_spline_gen_geometry_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(reset_spline_gen_geometry_renderer_ecs_query_comps, "spline_gen_geometry_renderer", SplineGenGeometry)
            , ECS_RW_COMP(reset_spline_gen_geometry_renderer_ecs_query_comps, "spline_gen_geometry__lod", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc reset_spline_gen_repository_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_repository"), ecs::ComponentTypeInfo<SplineGenGeometryRepository>()}
};
static ecs::CompileTimeQueryDesc reset_spline_gen_repository_ecs_query_desc
(
  "reset_spline_gen_repository_ecs_query",
  make_span(reset_spline_gen_repository_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void reset_spline_gen_repository_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, reset_spline_gen_repository_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(reset_spline_gen_repository_ecs_query_comps, "spline_gen_repository", SplineGenGeometryRepository)
            );

        }while (++comp != compE);
    }
  );
}
