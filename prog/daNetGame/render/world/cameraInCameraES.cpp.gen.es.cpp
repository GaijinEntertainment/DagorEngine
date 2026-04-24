// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cameraInCameraES.cpp.inl"
ECS_DEF_PULL_VAR(cameraInCamera);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc camcam_preprocess_prev_frame_weapon_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camcam__lens_only_zoom_enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void camcam_preprocess_prev_frame_weapon_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    camcam_preprocess_prev_frame_weapon_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(camcam_preprocess_prev_frame_weapon_es_comps, "camcam__lens_only_zoom_enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camcam_preprocess_prev_frame_weapon_es_es_desc
(
  "camcam_preprocess_prev_frame_weapon_es",
  "prog/daNetGame/render/world/cameraInCameraES.cpp.inl",
  ecs::EntitySystemOps(camcam_preprocess_prev_frame_weapon_es_all),
  make_span(camcam_preprocess_prev_frame_weapon_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"camcam_activate_view_es,camera_set_sync");
//static constexpr ecs::ComponentDesc camcam_activate_view_es_comps[] ={};
static void camcam_activate_view_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    camcam_activate_view_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc camcam_activate_view_es_es_desc
(
  "camcam_activate_view_es",
  "prog/daNetGame/render/world/cameraInCameraES.cpp.inl",
  ecs::EntitySystemOps(camcam_activate_view_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"camera_update_lods_scaling_es","update_shooter_camera_aim_parameters_es");
static constexpr ecs::ComponentDesc check_if_frame_after_deactivation_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("camcam__frame_after_deactivation"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc check_if_frame_after_deactivation_ecs_query_desc
(
  "check_if_frame_after_deactivation_ecs_query",
  empty_span(),
  make_span(check_if_frame_after_deactivation_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void check_if_frame_after_deactivation_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, check_if_frame_after_deactivation_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(check_if_frame_after_deactivation_ecs_query_comps, "camcam__frame_after_deactivation", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_camcam_frame_number_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("camcam__iFrame"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc get_camcam_frame_number_ecs_query_desc
(
  "get_camcam_frame_number_ecs_query",
  empty_span(),
  make_span(get_camcam_frame_number_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_camcam_frame_number_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_camcam_frame_number_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_camcam_frame_number_ecs_query_comps, "camcam__iFrame", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc check_if_scope_disables_camcam_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("gunmod__lensOnlyZoomDisabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc check_if_scope_disables_camcam_ecs_query_desc
(
  "check_if_scope_disables_camcam_ecs_query",
  empty_span(),
  make_span(check_if_scope_disables_camcam_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void check_if_scope_disables_camcam_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, check_if_scope_disables_camcam_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(check_if_scope_disables_camcam_ecs_query_comps, "gunmod__lensOnlyZoomDisabled", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc update_camcam_state_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("camcam__lens_render_active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("camcam__iFrame"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("camcam__frame_after_deactivation"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [3]
  {ECS_HASH("camcam__lens_only_zoom_enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc update_camcam_state_ecs_query_desc
(
  "update_camcam_state_ecs_query",
  make_span(update_camcam_state_ecs_query_comps+0, 3)/*rw*/,
  make_span(update_camcam_state_ecs_query_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_camcam_state_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, update_camcam_state_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_camcam_state_ecs_query_comps, "camcam__lens_render_active", bool)
            , ECS_RW_COMP(update_camcam_state_ecs_query_comps, "camcam__iFrame", int)
            , ECS_RO_COMP(update_camcam_state_ecs_query_comps, "camcam__lens_only_zoom_enabled", bool)
            , ECS_RW_COMP(update_camcam_state_ecs_query_comps, "camcam__frame_after_deactivation", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_camcam_render_state_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("camcam__lens_render_active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_camcam_render_state_ecs_query_desc
(
  "get_camcam_render_state_ecs_query",
  empty_span(),
  make_span(get_camcam_render_state_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_camcam_render_state_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_camcam_render_state_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_camcam_render_state_ecs_query_comps, "camcam__lens_render_active", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_camcam_lens_only_zoom_state_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("camcam__lens_only_zoom_enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_camcam_lens_only_zoom_state_ecs_query_desc
(
  "get_camcam_lens_only_zoom_state_ecs_query",
  empty_span(),
  make_span(get_camcam_lens_only_zoom_state_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_camcam_lens_only_zoom_state_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_camcam_lens_only_zoom_state_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_camcam_lens_only_zoom_state_ecs_query_comps, "camcam__lens_only_zoom_enabled", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc update_camcam_transforms_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camcam__uv_remapping"), ecs::ComponentTypeInfo<Point4>()}
};
static ecs::CompileTimeQueryDesc update_camcam_transforms_ecs_query_desc
(
  "update_camcam_transforms_ecs_query",
  make_span(update_camcam_transforms_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_camcam_transforms_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, update_camcam_transforms_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_camcam_transforms_ecs_query_comps, "camcam__uv_remapping", Point4)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_camcam_uv_remapping_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("camcam__uv_remapping"), ecs::ComponentTypeInfo<Point4>()}
};
static ecs::CompileTimeQueryDesc get_camcam_uv_remapping_ecs_query_desc
(
  "get_camcam_uv_remapping_ecs_query",
  empty_span(),
  make_span(get_camcam_uv_remapping_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_camcam_uv_remapping_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_camcam_uv_remapping_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_camcam_uv_remapping_ecs_query_comps, "camcam__uv_remapping", Point4)
            );

        }while (++comp != compE);
    }
  );
}
