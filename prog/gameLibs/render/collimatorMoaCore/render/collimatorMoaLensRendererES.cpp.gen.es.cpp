// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorMoaLensRendererES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorMoaLensRenderer);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc setup_collimator_moa_render_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("collimator_moa_render__gun_mod_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_render__rigid_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__calibration_range_cm"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc setup_collimator_moa_render_ecs_query_desc
(
  "setup_collimator_moa_render_ecs_query",
  empty_span(),
  make_span(setup_collimator_moa_render_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void setup_collimator_moa_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, setup_collimator_moa_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(setup_collimator_moa_render_ecs_query_comps, "collimator_moa_render__gun_mod_eid", ecs::EntityId)
            , ECS_RO_COMP(setup_collimator_moa_render_ecs_query_comps, "collimator_moa_render__rigid_id", int)
            , ECS_RO_COMP(setup_collimator_moa_render_ecs_query_comps, "collimator_moa_render__relem_id", int)
            , ECS_RO_COMP(setup_collimator_moa_render_ecs_query_comps, "collimator_moa_render__calibration_range_cm", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc setup_collimator_moa_lens_ecs_query_comps[] =
{
//start of 19 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("gunmod__collimator_moa_parallax_plane_dist"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("gunmod__collimator_moa_border_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_border_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_use_noise"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gunmod__collimator_moa_noise_min_intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_light_noise_thinness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_light_noise_intensity_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_static_noise_uv_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_static_noise_add"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_static_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_dynamic_noise_uv_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_dynamic_noise_sub_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_dynamic_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_dynamic_noise_add"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_dynamic_noise_intensity_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_dynamic_noise_speed"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gunmod__collimator_moa_reticle_offset_y"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc setup_collimator_moa_lens_ecs_query_desc
(
  "setup_collimator_moa_lens_ecs_query",
  empty_span(),
  make_span(setup_collimator_moa_lens_ecs_query_comps+0, 19)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void setup_collimator_moa_lens_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, setup_collimator_moa_lens_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_parallax_plane_dist", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_color", Point4)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_border_min", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_border_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_use_noise", bool)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_noise_min_intensity", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_light_noise_thinness", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_light_noise_intensity_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_static_noise_uv_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_static_noise_add", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_static_noise_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_dynamic_noise_uv_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_dynamic_noise_sub_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_dynamic_noise_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_dynamic_noise_add", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_dynamic_noise_intensity_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_dynamic_noise_speed", Point2)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_reticle_offset_y", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc render_collimator_moa_lens_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("collimator_moa_render__active_shapes_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__shapes_buf_reg_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__current_shapes_buf"), ecs::ComponentTypeInfo<UniqueBufHolder>()}
};
static ecs::CompileTimeQueryDesc render_collimator_moa_lens_ecs_query_desc
(
  "render_collimator_moa_lens_ecs_query",
  empty_span(),
  make_span(render_collimator_moa_lens_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_collimator_moa_lens_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_collimator_moa_lens_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__active_shapes_count", int)
            , ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__shapes_buf_reg_count", int)
            , ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__current_shapes_buf", UniqueBufHolder)
            );

        }while (++comp != compE);
    }
  );
}
