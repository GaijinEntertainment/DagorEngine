#include "worldRendererQueriesES.cpp.inl"
ECS_DEF_PULL_VAR(worldRendererQueries);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc invalidate_clipmap_under_camera_es_comps[] ={};
static void invalidate_clipmap_under_camera_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    invalidate_clipmap_under_camera_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc invalidate_clipmap_under_camera_es_es_desc
(
  "invalidate_clipmap_under_camera_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(invalidate_clipmap_under_camera_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc add_volfog_optional_graph_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("volfog"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void add_volfog_optional_graph_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    add_volfog_optional_graph_es_event_handler(evt
        , ECS_RO_COMP(add_volfog_optional_graph_es_event_handler_comps, "volfog", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc add_volfog_optional_graph_es_event_handler_es_desc
(
  "add_volfog_optional_graph_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, add_volfog_optional_graph_es_event_handler_all_events),
  empty_span(),
  make_span(add_volfog_optional_graph_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc move_indoor_light_probes_to_render_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static void move_indoor_light_probes_to_render_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventGameObjectsOptimize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    move_indoor_light_probes_to_render_es_event_handler(static_cast<const EventGameObjectsOptimize&>(evt)
        , ECS_RW_COMP(move_indoor_light_probes_to_render_es_event_handler_comps, "game_objects", GameObjects)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc move_indoor_light_probes_to_render_es_event_handler_es_desc
(
  "move_indoor_light_probes_to_render_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, move_indoor_light_probes_to_render_es_event_handler_all_events),
  make_span(move_indoor_light_probes_to_render_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsOptimize>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc start_occlusion_and_sw_raster_es_comps[] ={};
static void start_occlusion_and_sw_raster_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  start_occlusion_and_sw_raster_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc start_occlusion_and_sw_raster_es_es_desc
(
  "start_occlusion_and_sw_raster_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, start_occlusion_and_sw_raster_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"animchar_before_render_es");
//static constexpr ecs::ComponentDesc invalidate_clipmap_es_comps[] ={};
static void invalidate_clipmap_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<InvalidateClipmap>());
  invalidate_clipmap_es(static_cast<const InvalidateClipmap&>(evt)
        );
}
static ecs::EntitySystemDesc invalidate_clipmap_es_es_desc
(
  "invalidate_clipmap_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_clipmap_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<InvalidateClipmap>::build(),
  0
);
//static constexpr ecs::ComponentDesc invalidate_clipmap_box_es_comps[] ={};
static void invalidate_clipmap_box_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<InvalidateClipmapBox>());
  invalidate_clipmap_box_es(static_cast<const InvalidateClipmapBox&>(evt)
        );
}
static ecs::EntitySystemDesc invalidate_clipmap_box_es_es_desc
(
  "invalidate_clipmap_box_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_clipmap_box_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<InvalidateClipmapBox>::build(),
  0
);
static constexpr ecs::ComponentDesc reset_occlusion_data_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("close_geometry_bounding_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("close_geometry_prev_to_curr_frame_transform"), ecs::ComponentTypeInfo<mat44f>()},
  {ECS_HASH("occlusion_available"), ecs::ComponentTypeInfo<bool>()}
};
static void reset_occlusion_data_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reset_occlusion_data_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(reset_occlusion_data_es_comps, "close_geometry_bounding_radius", float)
    , ECS_RW_COMP(reset_occlusion_data_es_comps, "close_geometry_prev_to_curr_frame_transform", mat44f)
    , ECS_RW_COMP(reset_occlusion_data_es_comps, "occlusion_available", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reset_occlusion_data_es_es_desc
(
  "reset_occlusion_data_es",
  "prog/daNetGame/render/world/worldRendererQueriesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_occlusion_data_es_all_events),
  make_span(reset_occlusion_data_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc get_fpv_shadow_dist_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("fov"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("isTpsView"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shadow_cascade0_distance"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_fpv_shadow_dist_ecs_query_desc
(
  "get_fpv_shadow_dist_ecs_query",
  empty_span(),
  make_span(get_fpv_shadow_dist_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_fpv_shadow_dist_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_fpv_shadow_dist_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_fpv_shadow_dist_ecs_query_comps, "fov", float)
            , ECS_RO_COMP_OR(get_fpv_shadow_dist_ecs_query_comps, "isTpsView", bool(false))
            , ECS_RO_COMP_OR(get_fpv_shadow_dist_ecs_query_comps, "shadow_cascade0_distance", float(-1.0f))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_camera_fov_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("fov"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_camera_fov_ecs_query_desc
(
  "get_camera_fov_ecs_query",
  empty_span(),
  make_span(get_camera_fov_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_camera_fov_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_camera_fov_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_camera_fov_ecs_query_comps, "fov", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc volfog_optional_graphs_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("volfog"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc volfog_optional_graphs_ecs_query_desc
(
  "volfog_optional_graphs_ecs_query",
  empty_span(),
  make_span(volfog_optional_graphs_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void volfog_optional_graphs_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, volfog_optional_graphs_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volfog_optional_graphs_ecs_query_comps, "volfog", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_closest_occluders_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
//start of 1 rq components at [4]
  {ECS_HASH("is_occluder"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static ecs::CompileTimeQueryDesc gather_closest_occluders_ecs_query_desc
(
  "gather_closest_occluders_ecs_query",
  empty_span(),
  make_span(gather_closest_occluders_ecs_query_comps+0, 4)/*ro*/,
  make_span(gather_closest_occluders_ecs_query_comps+4, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_closest_occluders_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_closest_occluders_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_closest_occluders_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(gather_closest_occluders_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(gather_closest_occluders_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(gather_closest_occluders_ecs_query_comps, "animchar_bsph", vec4f)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc find_custom_sky_renderer_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("customSkyRenderer"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc find_custom_sky_renderer_ecs_query_desc
(
  "find_custom_sky_renderer_ecs_query",
  empty_span(),
  make_span(find_custom_sky_renderer_ecs_query_comps+0, 1)/*ro*/,
  make_span(find_custom_sky_renderer_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult find_custom_sky_renderer_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, find_custom_sky_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(find_custom_sky_renderer_ecs_query_comps, "eid", ecs::EntityId)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc get_envi_probe_render_flags_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("envi_probe_use_geometry"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("envi_probe_sun_enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_envi_probe_render_flags_ecs_query_desc
(
  "get_envi_probe_render_flags_ecs_query",
  empty_span(),
  make_span(get_envi_probe_render_flags_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_envi_probe_render_flags_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_envi_probe_render_flags_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_envi_probe_render_flags_ecs_query_comps, "envi_probe_use_geometry", bool)
            , ECS_RO_COMP(get_envi_probe_render_flags_ecs_query_comps, "envi_probe_sun_enabled", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc find_custom_envi_probe_renderer_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("customEnviProbeTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc find_custom_envi_probe_renderer_ecs_query_desc
(
  "find_custom_envi_probe_renderer_ecs_query",
  empty_span(),
  make_span(find_custom_envi_probe_renderer_ecs_query_comps+0, 1)/*ro*/,
  make_span(find_custom_envi_probe_renderer_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult find_custom_envi_probe_renderer_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, find_custom_envi_probe_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(find_custom_envi_probe_renderer_ecs_query_comps, "eid", ecs::EntityId)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc gather_occlusion_data_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("close_geometry_prev_to_curr_frame_transform"), ecs::ComponentTypeInfo<mat44f>()},
  {ECS_HASH("close_geometry_bounding_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("occlusion_available"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc gather_occlusion_data_ecs_query_desc
(
  "gather_occlusion_data_ecs_query",
  empty_span(),
  make_span(gather_occlusion_data_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_occlusion_data_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_occlusion_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_occlusion_data_ecs_query_comps, "close_geometry_prev_to_curr_frame_transform", mat44f)
            , ECS_RO_COMP(gather_occlusion_data_ecs_query_comps, "close_geometry_bounding_radius", float)
            , ECS_RO_COMP(gather_occlusion_data_ecs_query_comps, "occlusion_available", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_underground_zones_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [1]
  {ECS_HASH("underground_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_underground_zones_ecs_query_desc
(
  "gather_underground_zones_ecs_query",
  empty_span(),
  make_span(gather_underground_zones_ecs_query_comps+0, 1)/*ro*/,
  make_span(gather_underground_zones_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_underground_zones_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_underground_zones_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_underground_zones_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc needs_water_heightmap_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("needs_water_heightmap"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc needs_water_heightmap_ecs_query_desc
(
  "needs_water_heightmap_ecs_query",
  empty_span(),
  make_span(needs_water_heightmap_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void needs_water_heightmap_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, needs_water_heightmap_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(needs_water_heightmap_ecs_query_comps, "needs_water_heightmap", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc use_foam_tex_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("use_foam_tex"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc use_foam_tex_ecs_query_desc
(
  "use_foam_tex_ecs_query",
  empty_span(),
  make_span(use_foam_tex_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void use_foam_tex_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, use_foam_tex_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(use_foam_tex_ecs_query_comps, "use_foam_tex", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc use_wfx_textures_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("should_use_wfx_textures"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc use_wfx_textures_ecs_query_desc
(
  "use_wfx_textures_ecs_query",
  empty_span(),
  make_span(use_wfx_textures_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void use_wfx_textures_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, use_wfx_textures_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(use_wfx_textures_ecs_query_comps, "should_use_wfx_textures", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_is_camera_inside_custom_gi_zones_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("is_camera_inside_box"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("custom_gi_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_is_camera_inside_custom_gi_zones_ecs_query_desc
(
  "gather_is_camera_inside_custom_gi_zones_ecs_query",
  empty_span(),
  make_span(gather_is_camera_inside_custom_gi_zones_ecs_query_comps+0, 1)/*ro*/,
  make_span(gather_is_camera_inside_custom_gi_zones_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_is_camera_inside_custom_gi_zones_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_is_camera_inside_custom_gi_zones_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_is_camera_inside_custom_gi_zones_ecs_query_comps, "is_camera_inside_box", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_custom_gi_zones_bbox_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("is_camera_inside_box"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("custom_gi_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_custom_gi_zones_bbox_ecs_query_desc
(
  "gather_custom_gi_zones_bbox_ecs_query",
  empty_span(),
  make_span(gather_custom_gi_zones_bbox_ecs_query_comps+0, 2)/*ro*/,
  make_span(gather_custom_gi_zones_bbox_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_custom_gi_zones_bbox_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_custom_gi_zones_bbox_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_custom_gi_zones_bbox_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(gather_custom_gi_zones_bbox_ecs_query_comps, "is_camera_inside_box", bool)
            );

        }while (++comp != compE);
    }
  );
}
