// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cameraInCameraES.cpp.inl"
ECS_DEF_PULL_VAR(cameraInCamera);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc camcam_validate_scope_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("camcam__has_pending_validation"), ecs::ComponentTypeInfo<bool>()}
};
static void camcam_validate_scope_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    camcam_validate_scope_es_event_handler(evt
        , ECS_RO_COMP(camcam_validate_scope_es_event_handler_comps, "camcam__has_pending_validation", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camcam_validate_scope_es_event_handler_es_desc
(
  "camcam_validate_scope_es",
  "prog/daNetGame/render/world/cameraInCameraES.cpp.inl",
  ecs::EntitySystemOps(nullptr, camcam_validate_scope_es_event_handler_all_events),
  empty_span(),
  make_span(camcam_validate_scope_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"dev,render","camcam__has_pending_validation");
static constexpr ecs::ComponentDesc update_camcam_state_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("camcam__lens_render_active"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("camcam__lens_only_zoom_enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("camcam__has_pending_validation"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc update_camcam_state_ecs_query_desc
(
  "update_camcam_state_ecs_query",
  make_span(update_camcam_state_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_camcam_state_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_camcam_state_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_camcam_state_ecs_query_comps, "camcam__lens_render_active", bool)
            , ECS_RW_COMP(update_camcam_state_ecs_query_comps, "camcam__lens_only_zoom_enabled", bool)
            , ECS_RW_COMP_PTR(update_camcam_state_ecs_query_comps, "camcam__has_pending_validation", bool)
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
inline void check_if_scope_disables_camcam_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, check_if_scope_disables_camcam_ecs_query_desc.getHandle(),
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
inline void get_camcam_render_state_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_camcam_render_state_ecs_query_desc.getHandle(),
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
inline void get_camcam_lens_only_zoom_state_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_camcam_lens_only_zoom_state_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc get_scope_animchar_and_colres_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_scope_animchar_and_colres_ecs_query_desc
(
  "get_scope_animchar_and_colres_ecs_query",
  empty_span(),
  make_span(get_scope_animchar_and_colres_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_scope_animchar_and_colres_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_scope_animchar_and_colres_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_scope_animchar_and_colres_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(get_scope_animchar_and_colres_ecs_query_comps, "collres", CollisionResource)
            );

        }
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
inline void update_camcam_transforms_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_camcam_transforms_ecs_query_desc.getHandle(),
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
inline void get_camcam_uv_remapping_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_camcam_uv_remapping_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc vaidate_scope_ecs_query_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("gunmod__lensNode"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("item__name"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__res"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collres__res"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc vaidate_scope_ecs_query_desc
(
  "vaidate_scope_ecs_query",
  empty_span(),
  make_span(vaidate_scope_ecs_query_comps+0, 6)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void vaidate_scope_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, vaidate_scope_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(vaidate_scope_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(vaidate_scope_ecs_query_comps, "gunmod__lensNode", ecs::string)
            , ECS_RO_COMP_PTR(vaidate_scope_ecs_query_comps, "item__name", ecs::string)
            , ECS_RO_COMP_PTR(vaidate_scope_ecs_query_comps, "animchar__res", ecs::string)
            , ECS_RO_COMP_PTR(vaidate_scope_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP_PTR(vaidate_scope_ecs_query_comps, "collres__res", ecs::string)
            );

        }
    }
  );
}
