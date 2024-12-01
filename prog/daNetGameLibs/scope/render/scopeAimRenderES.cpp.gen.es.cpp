#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t bindedCamera_get_type();
static ecs::LTComponentList bindedCamera_component(ECS_HASH("bindedCamera"), bindedCamera_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
static constexpr ecs::component_t gunmod__distortionParams_get_type();
static ecs::LTComponentList gunmod__distortionParams_component(ECS_HASH("gunmod__distortionParams"), gunmod__distortionParams_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
static constexpr ecs::component_t gunmod__lensBrightness_get_type();
static ecs::LTComponentList gunmod__lensBrightness_component(ECS_HASH("gunmod__lensBrightness"), gunmod__lensBrightness_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
static constexpr ecs::component_t gunmod__lensLocalX_get_type();
static ecs::LTComponentList gunmod__lensLocalX_component(ECS_HASH("gunmod__lensLocalX"), gunmod__lensLocalX_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
static constexpr ecs::component_t gunmod__lensLocalY_get_type();
static ecs::LTComponentList gunmod__lensLocalY_component(ECS_HASH("gunmod__lensLocalY"), gunmod__lensLocalY_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
static constexpr ecs::component_t gunmod__lensLocalZ_get_type();
static ecs::LTComponentList gunmod__lensLocalZ_component(ECS_HASH("gunmod__lensLocalZ"), gunmod__lensLocalZ_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
static constexpr ecs::component_t gunmod__scopeRadLen_get_type();
static ecs::LTComponentList gunmod__scopeRadLen_component(ECS_HASH("gunmod__scopeRadLen"), gunmod__scopeRadLen_get_type(), "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl", "", 0);
#include "scopeAimRenderES.cpp.inl"
ECS_DEF_PULL_VAR(scopeAimRender);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc prepare_aim_occlusion_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("aim_data__gunEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("aim_data__isAiming"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void prepare_aim_occlusion_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(prepare_aim_occlusion_es_comps, "camera__active", bool)) )
      continue;
    prepare_aim_occlusion_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RO_COMP(prepare_aim_occlusion_es_comps, "aim_data__gunEid", ecs::EntityId)
      , ECS_RO_COMP(prepare_aim_occlusion_es_comps, "aim_data__isAiming", bool)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc prepare_aim_occlusion_es_es_desc
(
  "prepare_aim_occlusion_es",
  "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, prepare_aim_occlusion_es_all_events),
  empty_span(),
  make_span(prepare_aim_occlusion_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"start_occlusion_and_sw_raster_es");
//static constexpr ecs::ComponentDesc scope_quality_render_features_changed_es_comps[] ={};
static void scope_quality_render_features_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  scope_quality_render_features_changed_es(evt
        );
}
static ecs::EntitySystemDesc scope_quality_render_features_changed_es_es_desc
(
  "scope_quality_render_features_changed_es",
  "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, scope_quality_render_features_changed_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel,
                       ChangeRenderFeatures>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_scope_reticle_quad_rendering_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("scope_reticle_shader"), ecs::ComponentTypeInfo<ShadersECS>()}
};
static void init_scope_reticle_quad_rendering_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  init_scope_reticle_quad_rendering_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc init_scope_reticle_quad_rendering_es_event_handler_es_desc
(
  "init_scope_reticle_quad_rendering_es",
  "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_scope_reticle_quad_rendering_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(init_scope_reticle_quad_rendering_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc release_scope_reticle_quad_rendering_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("scope_reticle_shader"), ecs::ComponentTypeInfo<ShadersECS>()}
};
static void release_scope_reticle_quad_rendering_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  release_scope_reticle_quad_rendering_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc release_scope_reticle_quad_rendering_es_event_handler_es_desc
(
  "release_scope_reticle_quad_rendering_es",
  "prog/daNetGameLibs/scope/render/scopeAimRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, release_scope_reticle_quad_rendering_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(release_scope_reticle_quad_rendering_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc nightvision_lens_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("gunmod__nightvisionTex"), ecs::ComponentTypeInfo<SharedTex>()}
};
static ecs::CompileTimeQueryDesc nightvision_lens_ecs_query_desc
(
  "nightvision_lens_ecs_query",
  empty_span(),
  make_span(nightvision_lens_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void nightvision_lens_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, nightvision_lens_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(nightvision_lens_ecs_query_comps, "gunmod__nightvisionTex", SharedTex)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc set_scope_lens_phys_params_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()}
};
static ecs::CompileTimeQueryDesc set_scope_lens_phys_params_ecs_query_desc
(
  "set_scope_lens_phys_params_ecs_query",
  make_span(set_scope_lens_phys_params_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_scope_lens_phys_params_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, set_scope_lens_phys_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(set_scope_lens_phys_params_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc in_vehicle_cockpit_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("cockpit__isHeroInCockpit"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("vehicleWithWatched"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc in_vehicle_cockpit_ecs_query_desc
(
  "in_vehicle_cockpit_ecs_query",
  empty_span(),
  make_span(in_vehicle_cockpit_ecs_query_comps+0, 1)/*ro*/,
  make_span(in_vehicle_cockpit_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void in_vehicle_cockpit_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, in_vehicle_cockpit_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(in_vehicle_cockpit_ecs_query_comps, "cockpit__isHeroInCockpit", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc get_hero_cockpit_entities_with_prepass_flag_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hero_cockpit_entities"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("render_hero_cockpit_into_early_prepass"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_hero_cockpit_entities_with_prepass_flag_ecs_query_desc
(
  "get_hero_cockpit_entities_with_prepass_flag_ecs_query",
  make_span(get_hero_cockpit_entities_with_prepass_flag_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_hero_cockpit_entities_with_prepass_flag_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_hero_cockpit_entities_with_prepass_flag_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_hero_cockpit_entities_with_prepass_flag_ecs_query_comps, "hero_cockpit_entities", ecs::EidList)
            , ECS_RW_COMP(get_hero_cockpit_entities_with_prepass_flag_ecs_query_comps, "render_hero_cockpit_into_early_prepass", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc reprojection_cockpit_data_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("weapon_rearsight_node__nodeTm"), ecs::ComponentTypeInfo<mat44f>()},
  {ECS_HASH("weapon_rearsight_node__nodeIdx"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc reprojection_cockpit_data_ecs_query_desc
(
  "reprojection_cockpit_data_ecs_query",
  empty_span(),
  make_span(reprojection_cockpit_data_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void reprojection_cockpit_data_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, reprojection_cockpit_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(reprojection_cockpit_data_ecs_query_comps, "weapon_rearsight_node__nodeTm", mat44f)
            , ECS_RO_COMP(reprojection_cockpit_data_ecs_query_comps, "weapon_rearsight_node__nodeIdx", int)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc put_into_occlusion_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("close_geometry_bounding_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("close_geometry_prev_to_curr_frame_transform"), ecs::ComponentTypeInfo<mat44f>()},
  {ECS_HASH("occlusion_available"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc put_into_occlusion_ecs_query_desc
(
  "put_into_occlusion_ecs_query",
  make_span(put_into_occlusion_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void put_into_occlusion_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, put_into_occlusion_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(put_into_occlusion_ecs_query_comps, "close_geometry_bounding_radius", float)
            , ECS_RW_COMP(put_into_occlusion_ecs_query_comps, "close_geometry_prev_to_curr_frame_transform", mat44f)
            , ECS_RW_COMP(put_into_occlusion_ecs_query_comps, "occlusion_available", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc process_animchar_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 2 ro components at [2]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc process_animchar_ecs_query_desc
(
  "process_animchar_ecs_query",
  make_span(process_animchar_ecs_query_comps+0, 2)/*rw*/,
  make_span(process_animchar_ecs_query_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void process_animchar_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, process_animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(ECS_RO_COMP_OR(process_animchar_ecs_query_comps, "animchar_render__enabled", bool( true))) )
            return;
          function(
              ECS_RW_COMP(process_animchar_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RW_COMP(process_animchar_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(process_animchar_ecs_query_comps, "additional_data", ecs::Point4List)
            );

        }
      }
  );
}
static constexpr ecs::ComponentDesc render_scope_reticle_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 8 ro components at [1]
  {ECS_HASH("gunmod__scopeRadLen"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gunmod__reticleTex"), ecs::ComponentTypeInfo<SharedTex>()},
  {ECS_HASH("scope_reticle_world_tm_0"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("scope_reticle_world_tm_1"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("scope_reticle_world_tm_2"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("scope_reticle_radius"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("scope_reticle_shader"), ecs::ComponentTypeInfo<ShadersECS>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc render_scope_reticle_ecs_query_desc
(
  "render_scope_reticle_ecs_query",
  make_span(render_scope_reticle_ecs_query_comps+0, 1)/*rw*/,
  make_span(render_scope_reticle_ecs_query_comps+1, 8)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_scope_reticle_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, render_scope_reticle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(ECS_RO_COMP_OR(render_scope_reticle_ecs_query_comps, "animchar_render__enabled", bool( true))) )
            return;
          function(
              ECS_RW_COMP(render_scope_reticle_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "gunmod__scopeRadLen", Point2)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "gunmod__reticleTex", SharedTex)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "scope_reticle_world_tm_0", ShaderVar)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "scope_reticle_world_tm_1", ShaderVar)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "scope_reticle_world_tm_2", ShaderVar)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "scope_reticle_radius", ShaderVar)
            , ECS_RO_COMP(render_scope_reticle_ecs_query_comps, "scope_reticle_shader", ShadersECS)
            );

        }
      }
  );
}
static constexpr ecs::ComponentDesc get_dof_entity_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dof"), ecs::ComponentTypeInfo<DepthOfFieldPS>()}
};
static ecs::CompileTimeQueryDesc get_dof_entity_ecs_query_desc
(
  "get_dof_entity_ecs_query",
  make_span(get_dof_entity_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_dof_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_dof_entity_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_dof_entity_ecs_query_comps, "dof", DepthOfFieldPS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_aim_dof_scope_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("gunmod__focusPlaneShift"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__dofNearAmountPercent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__dofFarAmountPercent"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_aim_dof_scope_ecs_query_desc
(
  "get_aim_dof_scope_ecs_query",
  empty_span(),
  make_span(get_aim_dof_scope_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_aim_dof_scope_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_aim_dof_scope_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_aim_dof_scope_ecs_query_comps, "gunmod__focusPlaneShift", float)
            , ECS_RO_COMP(get_aim_dof_scope_ecs_query_comps, "gunmod__dofNearAmountPercent", float)
            , ECS_RO_COMP(get_aim_dof_scope_ecs_query_comps, "gunmod__dofFarAmountPercent", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_rearsight_dist_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("weapon_rearsight_node__nodeTm"), ecs::ComponentTypeInfo<mat44f>()}
};
static ecs::CompileTimeQueryDesc get_rearsight_dist_ecs_query_desc
(
  "get_rearsight_dist_ecs_query",
  empty_span(),
  make_span(get_rearsight_dist_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_rearsight_dist_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_rearsight_dist_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_rearsight_dist_ecs_query_comps, "weapon_rearsight_node__nodeTm", mat44f)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_aim_dof_weap_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("weap__focusPlaneShift"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("weap__dofNearAmountPercent"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_aim_dof_weap_ecs_query_desc
(
  "get_aim_dof_weap_ecs_query",
  empty_span(),
  make_span(get_aim_dof_weap_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_aim_dof_weap_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_aim_dof_weap_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_aim_dof_weap_ecs_query_comps, "weap__focusPlaneShift", float)
            , ECS_RO_COMP(get_aim_dof_weap_ecs_query_comps, "weap__dofNearAmountPercent", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc enable_scope_lod_change_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("shooter_cam__isScopeLodChangeEnabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc enable_scope_lod_change_ecs_query_desc
(
  "enable_scope_lod_change_ecs_query",
  make_span(enable_scope_lod_change_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void enable_scope_lod_change_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, enable_scope_lod_change_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(enable_scope_lod_change_ecs_query_comps, "shooter_cam__isScopeLodChangeEnabled", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc enable_scope_ri_lod_change_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("shooter_cam__isScopeRiLodChangeEnabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc enable_scope_ri_lod_change_ecs_query_desc
(
  "enable_scope_ri_lod_change_ecs_query",
  make_span(enable_scope_ri_lod_change_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void enable_scope_ri_lod_change_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, enable_scope_ri_lod_change_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(enable_scope_ri_lod_change_ecs_query_comps, "shooter_cam__isScopeRiLodChangeEnabled", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc prepare_scope_aim_rendering_data_ecs_query_comps[] =
{
//start of 14 ro components at [0]
  {ECS_HASH("aim_data__lensNodeId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aim_data__crosshairNodeId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aim_data__entityWithScopeLensEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("aim_data__gunEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("aim_data__scopeLensCockpitEntities"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("aim_data__isAiming"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__isAimingThroughScope"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__nightVision"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__nearDofEnabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__simplifiedAimDof"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__scopeWeaponFov"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aim_data__scopeWeaponFovType"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aim_data__scopeWeaponLensZoomFactor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc prepare_scope_aim_rendering_data_ecs_query_desc
(
  "prepare_scope_aim_rendering_data_ecs_query",
  empty_span(),
  make_span(prepare_scope_aim_rendering_data_ecs_query_comps+0, 14)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void prepare_scope_aim_rendering_data_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, prepare_scope_aim_rendering_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__lensNodeId", int)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__crosshairNodeId", int)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__entityWithScopeLensEid", ecs::EntityId)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__gunEid", ecs::EntityId)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__scopeLensCockpitEntities", ecs::EidList)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__isAiming", bool)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__isAimingThroughScope", bool)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__nightVision", bool)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__nearDofEnabled", bool)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__simplifiedAimDof", bool)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__scopeWeaponFov", float)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__scopeWeaponFovType", int)
            , ECS_RO_COMP(prepare_scope_aim_rendering_data_ecs_query_comps, "aim_data__scopeWeaponLensZoomFactor", float)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::component_t bindedCamera_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t gunmod__distortionParams_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t gunmod__lensBrightness_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t gunmod__lensLocalX_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t gunmod__lensLocalY_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t gunmod__lensLocalZ_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t gunmod__scopeRadLen_get_type(){return ecs::ComponentTypeInfo<Point2>::type; }
