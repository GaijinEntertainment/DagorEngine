// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bvhES.cpp.inl"
ECS_DEF_PULL_VAR(bvh);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc bvh_out_of_scope_riex_dist_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("bvh__initialized"), ecs::ComponentTypeInfo<bool>()}
};
static void bvh_out_of_scope_riex_dist_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    bvh_out_of_scope_riex_dist_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(bvh_out_of_scope_riex_dist_es_comps, "bvh__initialized", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_out_of_scope_riex_dist_es_es_desc
(
  "bvh_out_of_scope_riex_dist_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(bvh_out_of_scope_riex_dist_es_all),
  empty_span(),
  make_span(bvh_out_of_scope_riex_dist_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc setup_bvh_scene_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("bvh__heightProvider"), ecs::ComponentTypeInfo<BvhHeightProvider>()},
  {ECS_HASH("bvh__initialized"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("bvh__frame_counter"), ecs::ComponentTypeInfo<int>()}
};
static void setup_bvh_scene_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__initialized", bool)) )
      continue;
    setup_bvh_scene_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__heightProvider", BvhHeightProvider)
      , ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__initialized", bool)
      , ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__frame_counter", int)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc setup_bvh_scene_es_es_desc
(
  "setup_bvh_scene_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, setup_bvh_scene_es_all_events),
  make_span(setup_bvh_scene_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc close_bvh_scene_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("bvh__heightProvider"), ecs::ComponentTypeInfo<BvhHeightProvider>()}
};
static void close_bvh_scene_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  close_bvh_scene_es(evt
        );
}
static ecs::EntitySystemDesc close_bvh_scene_es_es_desc
(
  "close_bvh_scene_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, close_bvh_scene_es_all_events),
  empty_span(),
  empty_span(),
  make_span(close_bvh_scene_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bvh_destroy_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()},
  {ECS_HASH("bvh__rendinst_oof_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static void bvh_destroy_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bvh_destroy_es(evt
        , ECS_RW_COMP(bvh_destroy_es_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
    , ECS_RW_COMP(bvh_destroy_es_comps, "bvh__rendinst_oof_visibility", RiGenVisibilityECS)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_destroy_es_es_desc
(
  "bvh_destroy_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_destroy_es_all_events),
  make_span(bvh_destroy_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bvh_start_before_render_jobs_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("bvh__frame_counter"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("bvh__prev_pos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("bvh__accumulated_speed"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [3]
  {ECS_HASH("bvh__update_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void bvh_start_before_render_jobs_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bvh_start_before_render_jobs_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(bvh_start_before_render_jobs_es_comps, "bvh__frame_counter", int)
    , ECS_RW_COMP(bvh_start_before_render_jobs_es_comps, "bvh__prev_pos", Point3)
    , ECS_RW_COMP(bvh_start_before_render_jobs_es_comps, "bvh__accumulated_speed", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_start_before_render_jobs_es_es_desc
(
  "bvh_start_before_render_jobs_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_start_before_render_jobs_es_all_events),
  make_span(bvh_start_before_render_jobs_es_comps+0, 3)/*rw*/,
  empty_span(),
  make_span(bvh_start_before_render_jobs_es_comps+3, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"animchar_before_render_es","start_occlusion_and_sw_raster_es");
//static constexpr ecs::ComponentDesc set_animate_out_of_frustum_trees_es_comps[] ={};
static void set_animate_out_of_frustum_trees_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RendinstLodRangeIncreasedEvent>());
  set_animate_out_of_frustum_trees_es(static_cast<const RendinstLodRangeIncreasedEvent&>(evt)
        );
}
static ecs::EntitySystemDesc set_animate_out_of_frustum_trees_es_es_desc
(
  "set_animate_out_of_frustum_trees_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_animate_out_of_frustum_trees_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RendinstLodRangeIncreasedEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc rt_set_resolution_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("rt_persistent_textures"), ecs::ComponentTypeInfo<RTPersistentTexturesECS>()}
};
static void rt_set_resolution_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  rt_set_resolution_es(static_cast<const SetResolutionEvent&>(evt)
        );
}
static ecs::EntitySystemDesc rt_set_resolution_es_es_desc
(
  "rt_set_resolution_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rt_set_resolution_es_all_events),
  empty_span(),
  empty_span(),
  make_span(rt_set_resolution_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bvh_render_settings_changed_es_comps[] =
{
//start of 11 ro components at [0]
  {ECS_HASH("render_settings__enableBVH"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__enableRTSM"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("render_settings__enableRTR"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__enableRTTR"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__enableRTAO"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__enablePTGI"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__RTRWater"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__enableRTGI"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__antiAliasingMode"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("render_settings__rayReconstruction"), ecs::ComponentTypeInfo<bool>()}
};
static void bvh_render_settings_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bvh_render_settings_changed_es(evt
        , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enableBVH", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enableRTSM", ecs::string)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enableRTR", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enableRTTR", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enableRTAO", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enablePTGI", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__RTRWater", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__enableRTGI", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__bare_minimum", bool)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__antiAliasingMode", int)
    , ECS_RO_COMP(bvh_render_settings_changed_es_comps, "render_settings__rayReconstruction", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_render_settings_changed_es_es_desc
(
  "bvh_render_settings_changed_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_render_settings_changed_es_all_events),
  empty_span(),
  make_span(bvh_render_settings_changed_es_comps+0, 11)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__RTRWater,render_settings__antiAliasingMode,render_settings__bare_minimum,render_settings__enableBVH,render_settings__enablePTGI,render_settings__enableRTAO,render_settings__enableRTGI,render_settings__enableRTR,render_settings__enableRTSM,render_settings__enableRTTR,render_settings__rayReconstruction");
static constexpr ecs::ComponentDesc bvh_update_animchar_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("bvh__update_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void bvh_update_animchar_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  bvh_update_animchar_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc bvh_update_animchar_es_es_desc
(
  "bvh_update_animchar_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_update_animchar_es_all_events),
  empty_span(),
  empty_span(),
  make_span(bvh_update_animchar_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc recreate_draw_dynmodel_debug_node_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bvh_debug_dynmodels_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("bvh_debug_dynmodels_console_command"), ecs::ComponentTypeInfo<int>()}
};
static void recreate_draw_dynmodel_debug_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    recreate_draw_dynmodel_debug_node_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(recreate_draw_dynmodel_debug_node_es_comps, "bvh_debug_dynmodels_node", dafg::NodeHandle)
    , ECS_RW_COMP(recreate_draw_dynmodel_debug_node_es_comps, "bvh_debug_dynmodels_console_command", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc recreate_draw_dynmodel_debug_node_es_es_desc
(
  "recreate_draw_dynmodel_debug_node_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, recreate_draw_dynmodel_debug_node_es_all_events),
  make_span(recreate_draw_dynmodel_debug_node_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc get_resolved_rt_settings_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("resolved_rt_settings"), ecs::ComponentTypeInfo<ResolvedRTSettings>()}
};
static ecs::CompileTimeQueryDesc get_resolved_rt_settings_ecs_query_desc
(
  "get_resolved_rt_settings_ecs_query",
  empty_span(),
  make_span(get_resolved_rt_settings_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_resolved_rt_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_resolved_rt_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_resolved_rt_settings_ecs_query_comps, "resolved_rt_settings", ResolvedRTSettings)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc reset_bvh_entity_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bvh__initialized"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc reset_bvh_entity_ecs_query_desc
(
  "reset_bvh_entity_ecs_query",
  make_span(reset_bvh_entity_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void reset_bvh_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, reset_bvh_entity_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(reset_bvh_entity_ecs_query_comps, "bvh__initialized", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_bvh_rigen_visibility_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static ecs::CompileTimeQueryDesc get_bvh_rigen_visibility_ecs_query_desc
(
  "get_bvh_rigen_visibility_ecs_query",
  empty_span(),
  make_span(get_bvh_rigen_visibility_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_bvh_rigen_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_bvh_rigen_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_bvh_rigen_visibility_ecs_query_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_bvh_rigen_oof_visibility_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("bvh__rendinst_oof_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static ecs::CompileTimeQueryDesc get_bvh_rigen_oof_visibility_ecs_query_desc
(
  "get_bvh_rigen_oof_visibility_ecs_query",
  empty_span(),
  make_span(get_bvh_rigen_oof_visibility_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_bvh_rigen_oof_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_bvh_rigen_oof_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_bvh_rigen_oof_visibility_ecs_query_comps, "bvh__rendinst_oof_visibility", RiGenVisibilityECS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc recreate_bvh_nodes_ecs_query_comps[] =
{
//start of 12 rw components at [0]
  {ECS_HASH("bvh__update_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("denoiser_prepare_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("rtsm_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("rtsm_dynamic_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("rtr_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("rttr_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("rtao_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("ptgi_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("water_rt_early_before_envi_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("water_rt_early_after_envi_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("water_rt_late_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("rt_persistent_textures"), ecs::ComponentTypeInfo<RTPersistentTexturesECS>()}
};
static ecs::CompileTimeQueryDesc recreate_bvh_nodes_ecs_query_desc
(
  "recreate_bvh_nodes_ecs_query",
  make_span(recreate_bvh_nodes_ecs_query_comps+0, 12)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void recreate_bvh_nodes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, recreate_bvh_nodes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "bvh__update_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "denoiser_prepare_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "rtsm_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "rtsm_dynamic_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "rtr_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "rttr_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "rtao_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "ptgi_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "water_rt_early_before_envi_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "water_rt_early_after_envi_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "water_rt_late_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_bvh_nodes_ecs_query_comps, "rt_persistent_textures", RTPersistentTexturesECS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_resolved_rt_settings_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("resolved_rt_settings"), ecs::ComponentTypeInfo<ResolvedRTSettings>()},
//start of 1 ro components at [1]
  {ECS_HASH("needs_water_heightmap"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc set_resolved_rt_settings_ecs_query_desc
(
  "set_resolved_rt_settings_ecs_query",
  make_span(set_resolved_rt_settings_ecs_query_comps+0, 1)/*rw*/,
  make_span(set_resolved_rt_settings_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_resolved_rt_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_resolved_rt_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_resolved_rt_settings_ecs_query_comps, "resolved_rt_settings", ResolvedRTSettings)
            , ECS_RO_COMP(set_resolved_rt_settings_ecs_query_comps, "needs_water_heightmap", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc bvh_destroy_ri_visibility_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("bvh__initialized"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()},
  {ECS_HASH("bvh__rendinst_oof_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static ecs::CompileTimeQueryDesc bvh_destroy_ri_visibility_ecs_query_desc
(
  "bvh_destroy_ri_visibility_ecs_query",
  make_span(bvh_destroy_ri_visibility_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void bvh_destroy_ri_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, bvh_destroy_ri_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(bvh_destroy_ri_visibility_ecs_query_comps, "bvh__initialized", bool)
            , ECS_RW_COMP(bvh_destroy_ri_visibility_ecs_query_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
            , ECS_RW_COMP(bvh_destroy_ri_visibility_ecs_query_comps, "bvh__rendinst_oof_visibility", RiGenVisibilityECS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc bvh_create_ri_visibility_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()},
  {ECS_HASH("bvh__rendinst_oof_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static ecs::CompileTimeQueryDesc bvh_create_ri_visibility_ecs_query_desc
(
  "bvh_create_ri_visibility_ecs_query",
  make_span(bvh_create_ri_visibility_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void bvh_create_ri_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, bvh_create_ri_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(bvh_create_ri_visibility_ecs_query_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
            , ECS_RW_COMP(bvh_create_ri_visibility_ecs_query_comps, "bvh__rendinst_oof_visibility", RiGenVisibilityECS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc bvh_query_camo_params_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("vehicle_camo_condition"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vehicle_camo_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vehicle_camo_rotation"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc bvh_query_camo_params_ecs_query_desc
(
  "bvh_query_camo_params_ecs_query",
  empty_span(),
  make_span(bvh_query_camo_params_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void bvh_query_camo_params_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, bvh_query_camo_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(bvh_query_camo_params_ecs_query_comps, "vehicle_camo_condition", float)
            , ECS_RO_COMP(bvh_query_camo_params_ecs_query_comps, "vehicle_camo_scale", float)
            , ECS_RO_COMP(bvh_query_camo_params_ecs_query_comps, "vehicle_camo_rotation", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc bvh_iterate_over_animchars_ecs_query_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
//start of 2 no components at [6]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc bvh_iterate_over_animchars_ecs_query_desc
(
  "bvh_iterate_over_animchars_ecs_query",
  empty_span(),
  make_span(bvh_iterate_over_animchars_ecs_query_comps+0, 6)/*ro*/,
  empty_span(),
  make_span(bvh_iterate_over_animchars_ecs_query_comps+6, 2)/*no*/);
template<typename Callable>
inline void bvh_iterate_over_animchars_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, bvh_iterate_over_animchars_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(bvh_iterate_over_animchars_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(bvh_iterate_over_animchars_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP(bvh_iterate_over_animchars_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(bvh_iterate_over_animchars_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(bvh_iterate_over_animchars_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            , ECS_RO_COMP(bvh_iterate_over_animchars_ecs_query_comps, "animchar_bsph", vec4f)
            );

        }while (++comp != compE);
    }
  );
}
