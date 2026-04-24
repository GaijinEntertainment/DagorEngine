#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t daeditor__selected_get_type();
static ecs::LTComponentList daeditor__selected_component(ECS_HASH("daeditor__selected"), daeditor__selected_get_type(), "prog/gameLibs/ecs/lights/lightES.cpp.inl", "editor_draw_spot_lights_ecs_query", 0);
static constexpr ecs::component_t light__is_paused_get_type();
static ecs::LTComponentList light__is_paused_component(ECS_HASH("light__is_paused"), light__is_paused_get_type(), "prog/gameLibs/ecs/lights/lightES.cpp.inl", "pause_lights_ecs_query", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lightES.cpp.inl"
ECS_DEF_PULL_VAR(light);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc omni_light_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("omni_light"), ecs::ComponentTypeInfo<OmniLightEntity>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("light__force_max_light_radius"), ecs::ComponentTypeInfo<bool>()},
//start of 23 ro components at [3]
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("light__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__direction"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("light__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("light__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("omni_light__shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__dynamic_obj_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__shadow_quality"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__priority"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_shrink"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__radius_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_near_far_planes"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__dynamic_light"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__contact_shadows"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__affect_volumes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__enable_lens_flares"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__approximate_static"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__shadow_two_sided"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__force_affect_volfog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 3 no components at [26]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("light__use_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__use_clip"), ecs::ComponentTypeInfo<bool>()}
};
static void omni_light_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    omni_light_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(omni_light_es_comps, "omni_light", OmniLightEntity)
    , ECS_RW_COMP(omni_light_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(omni_light_es_comps, "lightModTm", TMatrix)
    , ECS_RO_COMP(omni_light_es_comps, "light__offset", Point3)
    , ECS_RO_COMP(omni_light_es_comps, "light__direction", Point3)
    , ECS_RO_COMP(omni_light_es_comps, "light__texture_name", ecs::string)
    , ECS_RW_COMP(omni_light_es_comps, "light__force_max_light_radius", bool)
    , ECS_RO_COMP(omni_light_es_comps, "light__color", E3DCOLOR)
    , ECS_RO_COMP(omni_light_es_comps, "light__brightness", float)
    , ECS_RO_COMP(omni_light_es_comps, "omni_light__shadows", bool)
    , ECS_RO_COMP(omni_light_es_comps, "omni_light__dynamic_obj_shadows", bool)
    , ECS_RO_COMP_OR(omni_light_es_comps, "omni_light__shadow_quality", int(0))
    , ECS_RO_COMP_OR(omni_light_es_comps, "omni_light__priority", int(0))
    , ECS_RO_COMP_OR(omni_light_es_comps, "omni_light__shadow_shrink", int(0))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__radius_scale", float(1))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__max_radius", float(-1))
    , ECS_RO_COMP_OR(omni_light_es_comps, "omni_light__shadow_near_far_planes", Point2(Point2(0, 0)))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__dynamic_light", bool(false))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__contact_shadows", bool(true))
    , ECS_RO_COMP_OR(omni_light_es_comps, "animchar_render__enabled", bool(true))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__affect_volumes", bool(true))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__render_gpu_objects", bool(false))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__enable_lens_flares", bool(false))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__approximate_static", bool(false))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__shadow_two_sided", bool(false))
    , ECS_RO_COMP_OR(omni_light_es_comps, "light__force_affect_volfog", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc omni_light_es_es_desc
(
  "omni_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(omni_light_es_all),
  make_span(omni_light_es_comps+0, 3)/*rw*/,
  make_span(omni_light_es_comps+3, 23)/*ro*/,
  empty_span(),
  make_span(omni_light_es_comps+26, 3)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc pause_lights_es_comps[] ={};
static void pause_lights_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    pause_lights_es(*info.cast<ecs::UpdateStageInfoAct>()
    , components.manager()
    );
}
static ecs::EntitySystemDesc pause_lights_es_es_desc
(
  "pause_lights_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(pause_lights_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"after_camera_sync");
static constexpr ecs::ComponentDesc spot_light_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("spot_light"), ecs::ComponentTypeInfo<SpotLightEntity>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("light__force_max_light_radius"), ecs::ComponentTypeInfo<bool>()},
//start of 27 ro components at [3]
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("light__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("light__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("light__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spot_light__dynamic_light"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__dynamic_obj_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadow_quality"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__priority"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_shrink"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__inner_attenuation"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__cone_angle"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__illuminating_disc_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_frustum_offset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_cone_angle"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_near_far_planes"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__radius_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__affect_volumes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__contact_shadows"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__enable_lens_flares"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__approximate_static"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__shadow_two_sided"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__force_affect_volfog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [30]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void spot_light_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    spot_light_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(spot_light_es_comps, "spot_light", SpotLightEntity)
    , ECS_RW_COMP(spot_light_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(spot_light_es_comps, "lightModTm", TMatrix)
    , ECS_RO_COMP(spot_light_es_comps, "light__offset", Point3)
    , ECS_RO_COMP(spot_light_es_comps, "light__texture_name", ecs::string)
    , ECS_RO_COMP(spot_light_es_comps, "light__color", E3DCOLOR)
    , ECS_RO_COMP(spot_light_es_comps, "light__brightness", float)
    , ECS_RO_COMP(spot_light_es_comps, "spot_light__dynamic_light", bool)
    , ECS_RO_COMP(spot_light_es_comps, "spot_light__shadows", bool)
    , ECS_RO_COMP(spot_light_es_comps, "spot_light__dynamic_obj_shadows", bool)
    , ECS_RW_COMP(spot_light_es_comps, "light__force_max_light_radius", bool)
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__shadow_quality", int(0))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__priority", int(0))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__shadow_shrink", int(0))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__inner_attenuation", float(0.95))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__cone_angle", float(-1.0))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__illuminating_disc_radius", float(0.0))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__shadow_frustum_offset", float(0.f))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__shadow_cone_angle", float(-1.f))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__shadow_near_far_planes", Point2(Point2::ZERO))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__radius_scale", float(1))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__max_radius", float(-1))
    , ECS_RO_COMP_OR(spot_light_es_comps, "animchar_render__enabled", bool(true))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__affect_volumes", bool(true))
    , ECS_RO_COMP_OR(spot_light_es_comps, "spot_light__contact_shadows", bool(false))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__render_gpu_objects", bool(false))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__enable_lens_flares", bool(false))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__approximate_static", bool(false))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__shadow_two_sided", bool(false))
    , ECS_RO_COMP_OR(spot_light_es_comps, "light__force_affect_volfog", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spot_light_es_es_desc
(
  "spot_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(spot_light_es_all),
  make_span(spot_light_es_comps+0, 3)/*rw*/,
  make_span(spot_light_es_comps+3, 27)/*ro*/,
  empty_span(),
  make_span(spot_light_es_comps+30, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc editor_draw_omni_light_es_comps[] ={};
static void editor_draw_omni_light_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    editor_draw_omni_light_es(*info.cast<UpdateStageInfoRenderDebug>()
    , components.manager()
    );
}
static ecs::EntitySystemDesc editor_draw_omni_light_es_es_desc
(
  "editor_draw_omni_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(editor_draw_omni_light_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
//static constexpr ecs::ComponentDesc editor_draw_spot_light_es_comps[] ={};
static void editor_draw_spot_light_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    editor_draw_spot_light_es(*info.cast<UpdateStageInfoRenderDebug>()
    , components.manager()
    );
}
static ecs::EntitySystemDesc editor_draw_spot_light_es_es_desc
(
  "editor_draw_spot_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(editor_draw_spot_light_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
//static constexpr ecs::ComponentDesc editor_draw_spot_light_shadow_es_comps[] ={};
static void editor_draw_spot_light_shadow_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    editor_draw_spot_light_shadow_es(*info.cast<UpdateStageInfoRenderDebug>()
    , components.manager()
    );
}
static ecs::EntitySystemDesc editor_draw_spot_light_shadow_es_es_desc
(
  "editor_draw_spot_light_shadow_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(editor_draw_spot_light_shadow_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc update_omni_light_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("omni_light"), ecs::ComponentTypeInfo<OmniLightEntity>()},
  {ECS_HASH("light__automatic_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__force_max_light_radius"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__box"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 30 ro components at [4]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("light__use_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__direction"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("light__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("light__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("omni_light__shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__dynamic_obj_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__use_clip"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__nightly"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_quality"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__priority"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_shrink"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__radius_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_near_far_planes"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__dynamic_light"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__contact_shadows"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__affect_volumes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__is_paused"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light_switch__on"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__enable_lens_flares"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__approximate_static"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__shadow_two_sided"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__force_affect_volfog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static void update_omni_light_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_omni_light_es(evt
        , ECS_RO_COMP(update_omni_light_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RW_COMP(update_omni_light_es_comps, "omni_light", OmniLightEntity)
    , ECS_RO_COMP(update_omni_light_es_comps, "light__use_box", bool)
    , ECS_RW_COMP(update_omni_light_es_comps, "light__automatic_box", bool)
    , ECS_RW_COMP(update_omni_light_es_comps, "light__force_max_light_radius", bool)
    , ECS_RW_COMP(update_omni_light_es_comps, "light__box", TMatrix)
    , ECS_RO_COMP(update_omni_light_es_comps, "light__offset", Point3)
    , ECS_RO_COMP(update_omni_light_es_comps, "light__direction", Point3)
    , ECS_RO_COMP(update_omni_light_es_comps, "light__texture_name", ecs::string)
    , ECS_RO_COMP(update_omni_light_es_comps, "light__color", E3DCOLOR)
    , ECS_RO_COMP(update_omni_light_es_comps, "light__brightness", float)
    , ECS_RO_COMP(update_omni_light_es_comps, "omni_light__shadows", bool)
    , ECS_RO_COMP(update_omni_light_es_comps, "omni_light__dynamic_obj_shadows", bool)
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__use_clip", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__nightly", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "omni_light__shadow_quality", int(0))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "omni_light__priority", int(0))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "omni_light__shadow_shrink", int(0))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__radius_scale", float(1))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__max_radius", float(-1))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "omni_light__shadow_near_far_planes", Point2(Point2(0, 0)))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__dynamic_light", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__contact_shadows", bool(true))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__affect_volumes", bool(true))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__render_gpu_objects", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__is_paused", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light_switch__on", bool(true))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__enable_lens_flares", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__approximate_static", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__shadow_two_sided", bool(false))
    , ECS_RO_COMP_OR(update_omni_light_es_comps, "light__force_affect_volfog", bool(false))
    , ECS_RO_COMP_PTR(update_omni_light_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(update_omni_light_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP_PTR(update_omni_light_es_comps, "lightModTm", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_omni_light_es_es_desc
(
  "update_omni_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_omni_light_es_all_events),
  make_span(update_omni_light_es_comps+0, 4)/*rw*/,
  make_span(update_omni_light_es_comps+4, 30)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdRecreateAllLights,
                       CmdRecreateOmniLights,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc update_high_priority_omni_light_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("omni_light"), ecs::ComponentTypeInfo<OmniLightEntity>()},
  {ECS_HASH("light__automatic_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__force_max_light_radius"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__box"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 27 ro components at [4]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("light__use_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__use_clip"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__direction"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("light__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("light__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("omni_light__shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__dynamic_obj_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__shadow_quality"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__priority"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_shrink"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__radius_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("omni_light__shadow_near_far_planes"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__dynamic_light"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__contact_shadows"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__affect_volumes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__enable_lens_flares"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__approximate_static"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__shadow_two_sided"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__force_affect_volfog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [31]
  {ECS_HASH("light__high_priority_update"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void update_high_priority_omni_light_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_high_priority_omni_light_es(evt
        , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RW_COMP(update_high_priority_omni_light_es_comps, "omni_light", OmniLightEntity)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__use_box", bool)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__use_clip", bool)
    , ECS_RW_COMP(update_high_priority_omni_light_es_comps, "light__automatic_box", bool)
    , ECS_RW_COMP(update_high_priority_omni_light_es_comps, "light__force_max_light_radius", bool)
    , ECS_RW_COMP(update_high_priority_omni_light_es_comps, "light__box", TMatrix)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__offset", Point3)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__direction", Point3)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__texture_name", ecs::string)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__color", E3DCOLOR)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "light__brightness", float)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "omni_light__shadows", bool)
    , ECS_RO_COMP(update_high_priority_omni_light_es_comps, "omni_light__dynamic_obj_shadows", bool)
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "omni_light__shadow_quality", int(0))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "omni_light__priority", int(0))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "omni_light__shadow_shrink", int(0))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__radius_scale", float(1))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__max_radius", float(-1))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "omni_light__shadow_near_far_planes", Point2(Point2(0, 0)))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__dynamic_light", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__contact_shadows", bool(true))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__affect_volumes", bool(true))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__render_gpu_objects", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__enable_lens_flares", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__approximate_static", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__shadow_two_sided", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_omni_light_es_comps, "light__force_affect_volfog", bool(false))
    , ECS_RO_COMP_PTR(update_high_priority_omni_light_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(update_high_priority_omni_light_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP_PTR(update_high_priority_omni_light_es_comps, "lightModTm", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_high_priority_omni_light_es_es_desc
(
  "update_high_priority_omni_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_high_priority_omni_light_es_all_events),
  make_span(update_high_priority_omni_light_es_comps+0, 4)/*rw*/,
  make_span(update_high_priority_omni_light_es_comps+4, 27)/*ro*/,
  make_span(update_high_priority_omni_light_es_comps+31, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<CmdUpdateHighPriorityLights>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc omni_light_upd_visibility_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("light__visible"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("omni_light"), ecs::ComponentTypeInfo<OmniLightEntity>()}
};
static void omni_light_upd_visibility_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    omni_light_upd_visibility_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(omni_light_upd_visibility_es_comps, "omni_light", OmniLightEntity)
    , ECS_RW_COMP(omni_light_upd_visibility_es_comps, "light__visible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc omni_light_upd_visibility_es_es_desc
(
  "omni_light_upd_visibility_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, omni_light_upd_visibility_es_all_events),
  make_span(omni_light_upd_visibility_es_comps+0, 1)/*rw*/,
  make_span(omni_light_upd_visibility_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc pause_lights_set_sq_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("light__render_range_limit_start_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("light__render_range_limit_stop_sq"), ecs::ComponentTypeInfo<float>()},
//start of 2 ro components at [2]
  {ECS_HASH("light__render_range_limit"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("light__render_range_gap"), ecs::ComponentTypeInfo<float>()}
};
static void pause_lights_set_sq_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    pause_lights_set_sq_es(evt
        , ECS_RO_COMP(pause_lights_set_sq_es_comps, "light__render_range_limit", float)
    , ECS_RO_COMP(pause_lights_set_sq_es_comps, "light__render_range_gap", float)
    , ECS_RW_COMP(pause_lights_set_sq_es_comps, "light__render_range_limit_start_sq", float)
    , ECS_RW_COMP(pause_lights_set_sq_es_comps, "light__render_range_limit_stop_sq", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc pause_lights_set_sq_es_es_desc
(
  "pause_lights_set_sq_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, pause_lights_set_sq_es_all_events),
  make_span(pause_lights_set_sq_es_comps+0, 2)/*rw*/,
  make_span(pause_lights_set_sq_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_spot_light_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spot_light"), ecs::ComponentTypeInfo<SpotLightEntity>()},
  {ECS_HASH("light__force_max_light_radius"), ecs::ComponentTypeInfo<bool>()},
//start of 32 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("light__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("light__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("light__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spot_light__dynamic_light"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__dynamic_obj_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__nightly"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_quality"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__priority"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_shrink"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__cone_angle"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__illuminating_disc_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__inner_attenuation"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_cone_angle"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_frustum_offset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_near_far_planes"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__radius_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__affect_volumes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__contact_shadows"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__is_paused"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light_switch__on"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__enable_lens_flares"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__approximate_static"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__shadow_two_sided"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__force_affect_volfog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void update_spot_light_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_spot_light_es(evt
        , ECS_RO_COMP(update_spot_light_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RW_COMP(update_spot_light_es_comps, "spot_light", SpotLightEntity)
    , ECS_RO_COMP(update_spot_light_es_comps, "light__offset", Point3)
    , ECS_RO_COMP(update_spot_light_es_comps, "light__texture_name", ecs::string)
    , ECS_RO_COMP(update_spot_light_es_comps, "light__color", E3DCOLOR)
    , ECS_RO_COMP(update_spot_light_es_comps, "light__brightness", float)
    , ECS_RO_COMP(update_spot_light_es_comps, "spot_light__dynamic_light", bool)
    , ECS_RO_COMP(update_spot_light_es_comps, "spot_light__shadows", bool)
    , ECS_RO_COMP(update_spot_light_es_comps, "spot_light__dynamic_obj_shadows", bool)
    , ECS_RW_COMP(update_spot_light_es_comps, "light__force_max_light_radius", bool)
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__nightly", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__shadow_quality", int(0))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__priority", int(0))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__shadow_shrink", int(0))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__cone_angle", float(-1.0))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__illuminating_disc_radius", float(0.0))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__inner_attenuation", float(0.95))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__shadow_cone_angle", float(-1.f))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__shadow_frustum_offset", float(0.f))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__shadow_near_far_planes", Point2(Point2::ZERO))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__radius_scale", float(1))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__max_radius", float(-1))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__affect_volumes", bool(true))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "spot_light__contact_shadows", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__render_gpu_objects", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__is_paused", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light_switch__on", bool(true))
    , ECS_RO_COMP_PTR(update_spot_light_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(update_spot_light_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP_PTR(update_spot_light_es_comps, "lightModTm", TMatrix)
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__enable_lens_flares", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__approximate_static", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__shadow_two_sided", bool(false))
    , ECS_RO_COMP_OR(update_spot_light_es_comps, "light__force_affect_volfog", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_spot_light_es_es_desc
(
  "update_spot_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_spot_light_es_all_events),
  make_span(update_spot_light_es_comps+0, 2)/*rw*/,
  make_span(update_spot_light_es_comps+2, 32)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdRecreateAllLights,
                       CmdRecreateSpotLights,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc update_high_priority_spot_light_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spot_light"), ecs::ComponentTypeInfo<SpotLightEntity>()},
  {ECS_HASH("light__force_max_light_radius"), ecs::ComponentTypeInfo<bool>()},
//start of 29 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("light__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("light__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("light__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("spot_light__dynamic_light"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__dynamic_obj_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadow_quality"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__priority"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_shrink"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__cone_angle"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__illuminating_disc_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__inner_attenuation"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_frustum_offset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_cone_angle"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__shadow_near_far_planes"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__radius_scale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__affect_volumes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("spot_light__contact_shadows"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__enable_lens_flares"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__approximate_static"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__shadow_two_sided"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("light__force_affect_volfog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [31]
  {ECS_HASH("light__high_priority_update"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void update_high_priority_spot_light_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_high_priority_spot_light_es(evt
        , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RW_COMP(update_high_priority_spot_light_es_comps, "spot_light", SpotLightEntity)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "light__offset", Point3)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "light__texture_name", ecs::string)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "light__color", E3DCOLOR)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "light__brightness", float)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "spot_light__dynamic_light", bool)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "spot_light__shadows", bool)
    , ECS_RO_COMP(update_high_priority_spot_light_es_comps, "spot_light__dynamic_obj_shadows", bool)
    , ECS_RW_COMP(update_high_priority_spot_light_es_comps, "light__force_max_light_radius", bool)
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__shadow_quality", int(0))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__priority", int(0))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__shadow_shrink", int(0))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__cone_angle", float(-1.0))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__illuminating_disc_radius", float(0.0))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__inner_attenuation", float(0.95))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__shadow_frustum_offset", float(0.f))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__shadow_cone_angle", float(-1.f))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__shadow_near_far_planes", Point2(Point2::ZERO))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__radius_scale", float(1))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__max_radius", float(-1))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__affect_volumes", bool(true))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "spot_light__contact_shadows", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__render_gpu_objects", bool(false))
    , ECS_RO_COMP_PTR(update_high_priority_spot_light_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(update_high_priority_spot_light_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP_PTR(update_high_priority_spot_light_es_comps, "lightModTm", TMatrix)
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__enable_lens_flares", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__approximate_static", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__shadow_two_sided", bool(false))
    , ECS_RO_COMP_OR(update_high_priority_spot_light_es_comps, "light__force_affect_volfog", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_high_priority_spot_light_es_es_desc
(
  "update_high_priority_spot_light_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_high_priority_spot_light_es_all_events),
  make_span(update_high_priority_spot_light_es_comps+0, 2)/*rw*/,
  make_span(update_high_priority_spot_light_es_comps+2, 29)/*ro*/,
  make_span(update_high_priority_spot_light_es_comps+31, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<CmdUpdateHighPriorityLights>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc spot_light_upd_visibility_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("light__visible"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("spot_light"), ecs::ComponentTypeInfo<SpotLightEntity>()}
};
static void spot_light_upd_visibility_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    spot_light_upd_visibility_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(spot_light_upd_visibility_es_comps, "spot_light", SpotLightEntity)
    , ECS_RW_COMP(spot_light_upd_visibility_es_comps, "light__visible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spot_light_upd_visibility_es_es_desc
(
  "spot_light_upd_visibility_es",
  "prog/gameLibs/ecs/lights/lightES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spot_light_upd_visibility_es_all_events),
  make_span(spot_light_upd_visibility_es_comps+0, 1)/*rw*/,
  make_span(spot_light_upd_visibility_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc set_omni_lights_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("light__box"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 3 ro components at [1]
  {ECS_HASH("omni_light"), ecs::ComponentTypeInfo<OmniLightEntity>()},
  {ECS_HASH("light__automatic_box"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("light__use_box"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc set_omni_lights_ecs_query_desc
(
  "set_omni_lights_ecs_query",
  make_span(set_omni_lights_ecs_query_comps+0, 1)/*rw*/,
  make_span(set_omni_lights_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_omni_lights_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, set_omni_lights_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(set_omni_lights_ecs_query_comps, "light__automatic_box", bool) && ECS_RO_COMP(set_omni_lights_ecs_query_comps, "light__use_box", bool)) )
            continue;
          function(
              ECS_RO_COMP(set_omni_lights_ecs_query_comps, "omni_light", OmniLightEntity)
            , ECS_RW_COMP(set_omni_lights_ecs_query_comps, "light__box", TMatrix)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc pause_lights_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("light__render_range_limit_start_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("light__render_range_limit_stop_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("light__is_paused"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("lightModTm"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc pause_lights_ecs_query_desc
(
  "pause_lights_ecs_query",
  empty_span(),
  make_span(pause_lights_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void pause_lights_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, pause_lights_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(pause_lights_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(pause_lights_ecs_query_comps, "light__render_range_limit_start_sq", float)
            , ECS_RO_COMP(pause_lights_ecs_query_comps, "light__render_range_limit_stop_sq", float)
            , ECS_RO_COMP(pause_lights_ecs_query_comps, "light__is_paused", bool)
            , ECS_RO_COMP_PTR(pause_lights_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_PTR(pause_lights_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP_PTR(pause_lights_ecs_query_comps, "lightModTm", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc editor_draw_omni_lights_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("omni_light"), ecs::ComponentTypeInfo<OmniLightEntity>()}
};
static ecs::CompileTimeQueryDesc editor_draw_omni_lights_ecs_query_desc
(
  "editor_draw_omni_lights_ecs_query",
  empty_span(),
  make_span(editor_draw_omni_lights_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void editor_draw_omni_lights_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, editor_draw_omni_lights_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(editor_draw_omni_lights_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(editor_draw_omni_lights_ecs_query_comps, "omni_light", OmniLightEntity)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc editor_draw_spot_lights_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("spot_light"), ecs::ComponentTypeInfo<SpotLightEntity>()}
};
static ecs::CompileTimeQueryDesc editor_draw_spot_lights_ecs_query_desc
(
  "editor_draw_spot_lights_ecs_query",
  empty_span(),
  make_span(editor_draw_spot_lights_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void editor_draw_spot_lights_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, editor_draw_spot_lights_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(editor_draw_spot_lights_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(editor_draw_spot_lights_ecs_query_comps, "spot_light", SpotLightEntity)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc editor_draw_spot_lights_shadow_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("spot_light"), ecs::ComponentTypeInfo<SpotLightEntity>()},
//start of 1 rq components at [1]
  {ECS_HASH("daeditor__selected"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc editor_draw_spot_lights_shadow_ecs_query_desc
(
  "editor_draw_spot_lights_shadow_ecs_query",
  empty_span(),
  make_span(editor_draw_spot_lights_shadow_ecs_query_comps+0, 1)/*ro*/,
  make_span(editor_draw_spot_lights_shadow_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void editor_draw_spot_lights_shadow_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, editor_draw_spot_lights_shadow_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(editor_draw_spot_lights_shadow_ecs_query_comps, "spot_light", SpotLightEntity)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t daeditor__selected_get_type(){return ecs::ComponentTypeInfo<ecs::Tag>::type; }
static constexpr ecs::component_t light__is_paused_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
