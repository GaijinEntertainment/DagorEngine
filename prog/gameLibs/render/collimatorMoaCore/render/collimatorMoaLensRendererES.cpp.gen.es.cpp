// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorMoaLensRendererES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorMoaLensRenderer);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc collimator_moa_update_lens_axis_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("collimator_moa_render__gun_mod_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_render__rigid_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_id"), ecs::ComponentTypeInfo<int>()}
};
static void collimator_moa_update_lens_axis_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    collimator_moa_update_lens_axis_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RO_COMP(collimator_moa_update_lens_axis_es_comps, "collimator_moa_render__gun_mod_eid", ecs::EntityId)
    , ECS_RO_COMP(collimator_moa_update_lens_axis_es_comps, "collimator_moa_render__rigid_id", int)
    , ECS_RO_COMP(collimator_moa_update_lens_axis_es_comps, "collimator_moa_render__relem_id", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc collimator_moa_update_lens_axis_es_es_desc
(
  "collimator_moa_update_lens_axis_es",
  "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaLensRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, collimator_moa_update_lens_axis_es_all_events),
  empty_span(),
  make_span(collimator_moa_update_lens_axis_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es,collimator_moa_update_lens_eids_es");
static constexpr ecs::ComponentDesc update_collimator_moa_lens_axis_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gunmod__collimator_moa_lens_axis"), ecs::ComponentTypeInfo<Point3>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("gunmod__collimator_moa_local_forward"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc update_collimator_moa_lens_axis_ecs_query_desc
(
  "update_collimator_moa_lens_axis_ecs_query",
  make_span(update_collimator_moa_lens_axis_ecs_query_comps+0, 1)/*rw*/,
  make_span(update_collimator_moa_lens_axis_ecs_query_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_collimator_moa_lens_axis_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, update_collimator_moa_lens_axis_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(update_collimator_moa_lens_axis_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(update_collimator_moa_lens_axis_ecs_query_comps, "gunmod__collimator_moa_local_forward", Point3)
            , ECS_RW_COMP(update_collimator_moa_lens_axis_ecs_query_comps, "gunmod__collimator_moa_lens_axis", Point3)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc setup_collimator_moa_image_ecs_query_comps[] =
{
//start of 15 ro components at [0]
  {ECS_HASH("collimator_moa_image__color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("collimator_moa_image__border_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__border_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__use_noise"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("collimator_moa_image__noise_min_intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__light_noise_thinness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__light_noise_intensity_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__static_noise_uv_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__static_noise_add"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__static_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__dynamic_noise_uv_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__dynamic_noise_sub_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__dynamic_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__dynamic_noise_add"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("collimator_moa_image__dynamic_noise_speed"), ecs::ComponentTypeInfo<Point2>()}
};
static ecs::CompileTimeQueryDesc setup_collimator_moa_image_ecs_query_desc
(
  "setup_collimator_moa_image_ecs_query",
  empty_span(),
  make_span(setup_collimator_moa_image_ecs_query_comps+0, 15)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void setup_collimator_moa_image_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, setup_collimator_moa_image_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__color", Point4)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__border_min", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__border_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__use_noise", bool)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__noise_min_intensity", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__light_noise_thinness", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__light_noise_intensity_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__static_noise_uv_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__static_noise_add", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__static_noise_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__dynamic_noise_uv_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__dynamic_noise_sub_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__dynamic_noise_scale", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__dynamic_noise_add", float)
            , ECS_RO_COMP(setup_collimator_moa_image_ecs_query_comps, "collimator_moa_image__dynamic_noise_speed", Point2)
            );

        }
    }
  );
}
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
inline void setup_collimator_moa_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, setup_collimator_moa_render_ecs_query_desc.getHandle(),
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
//start of 6 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("gunmod__collimator_moa_local_forward"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gunmod__collimator_moa_local_right"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gunmod__collimator_moa_lens_axis"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gunmod__collimator_moa_parallax_plane_dist"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_lens_color"), ecs::ComponentTypeInfo<Point4>()}
};
static ecs::CompileTimeQueryDesc setup_collimator_moa_lens_ecs_query_desc
(
  "setup_collimator_moa_lens_ecs_query",
  empty_span(),
  make_span(setup_collimator_moa_lens_ecs_query_comps+0, 6)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void setup_collimator_moa_lens_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, setup_collimator_moa_lens_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_local_forward", Point3)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_local_right", Point3)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_lens_axis", Point3)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_moa_parallax_plane_dist", float)
            , ECS_RO_COMP(setup_collimator_moa_lens_ecs_query_comps, "gunmod__collimator_lens_color", Point4)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc render_collimator_moa_lens_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("collimator_moa_render__active_shapes_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__shapes_buf_reg_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__current_shapes_buf"), ecs::ComponentTypeInfo<UniqueBufWithShaderVar>()},
  {ECS_HASH("collimator_moa_render__active_image_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_render__lens_shader"), ecs::ComponentTypeInfo<ShadersECS>()}
};
static ecs::CompileTimeQueryDesc render_collimator_moa_lens_ecs_query_desc
(
  "render_collimator_moa_lens_ecs_query",
  empty_span(),
  make_span(render_collimator_moa_lens_ecs_query_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_collimator_moa_lens_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, render_collimator_moa_lens_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__active_shapes_count", int)
            , ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__shapes_buf_reg_count", int)
            , ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__current_shapes_buf", UniqueBufWithShaderVar)
            , ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__active_image_eid", ecs::EntityId)
            , ECS_RO_COMP(render_collimator_moa_lens_ecs_query_comps, "collimator_moa_render__lens_shader", ShadersECS)
            );

        }while (++comp != compE);
    }
  );
}
