#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t blood_puddles__dir_get_type();
static ecs::LTComponentList blood_puddles__dir_component(ECS_HASH("blood_puddles__dir"), blood_puddles__dir_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_puddles__pos_get_type();
static ecs::LTComponentList blood_puddles__pos_component(ECS_HASH("blood_puddles__pos"), blood_puddles__pos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__gravity_get_type();
static ecs::LTComponentList blood_splash_emitter__gravity_component(ECS_HASH("blood_splash_emitter__gravity"), blood_splash_emitter__gravity_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__itm_get_type();
static ecs::LTComponentList blood_splash_emitter__itm_component(ECS_HASH("blood_splash_emitter__itm"), blood_splash_emitter__itm_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__matrix_id_get_type();
static ecs::LTComponentList blood_splash_emitter__matrix_id_component(ECS_HASH("blood_splash_emitter__matrix_id"), blood_splash_emitter__matrix_id_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__normal_get_type();
static ecs::LTComponentList blood_splash_emitter__normal_component(ECS_HASH("blood_splash_emitter__normal"), blood_splash_emitter__normal_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__pos_get_type();
static ecs::LTComponentList blood_splash_emitter__pos_component(ECS_HASH("blood_splash_emitter__pos"), blood_splash_emitter__pos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__size_get_type();
static ecs::LTComponentList blood_splash_emitter__size_component(ECS_HASH("blood_splash_emitter__size"), blood_splash_emitter__size_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__targetPos_get_type();
static ecs::LTComponentList blood_splash_emitter__targetPos_component(ECS_HASH("blood_splash_emitter__targetPos"), blood_splash_emitter__targetPos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__velocity_get_type();
static ecs::LTComponentList blood_splash_emitter__velocity_component(ECS_HASH("blood_splash_emitter__velocity"), blood_splash_emitter__velocity_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
#include "bloodPuddlesES.cpp.inl"
ECS_DEF_PULL_VAR(bloodPuddles);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc reset_blood_es_comps[] ={};
static void reset_blood_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  reset_blood_es(static_cast<const AfterDeviceReset&>(evt)
        );
}
static ecs::EntitySystemDesc reset_blood_es_es_desc
(
  "reset_blood_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_blood_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
);
static constexpr ecs::ComponentDesc update_blood_shader_params_es_comps[] =
{
//start of 16 ro components at [0]
  {ECS_HASH("blood_begin_color"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_begin_color_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("blood_end_color"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_end_color_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("blood_puddle_high_intensity_color"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_puddle_high_intensity_color_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("blood_puddle_low_intensity_color"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_puddle_low_intensity_color_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("blood_puddle_start_size"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_puddle_start_size_value"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("blood_puddle_reflectance"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_puddle_reflectance_value"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("blood_puddle_smoothness"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_puddle_smoothness_value"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("blood_puddle_smoothness_edge"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("blood_puddle_smoothness_edge_value"), ecs::ComponentTypeInfo<float>()}
};
static void update_blood_shader_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_blood_shader_params_es(evt
        , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_begin_color", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_begin_color_value", Point4)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_end_color", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_end_color_value", Point4)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_high_intensity_color", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_high_intensity_color_value", Point4)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_low_intensity_color", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_low_intensity_color_value", Point4)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_start_size", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_start_size_value", float)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_reflectance", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_reflectance_value", float)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_smoothness", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_smoothness_value", float)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_smoothness_edge", ShaderVar)
    , ECS_RO_COMP(update_blood_shader_params_es_comps, "blood_puddle_smoothness_edge_value", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_blood_shader_params_es_es_desc
(
  "update_blood_shader_params_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_blood_shader_params_es_all_events),
  empty_span(),
  make_span(update_blood_shader_params_es_comps+0, 16)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc is_bood_enabled_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("isBloodEnabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc is_bood_enabled_ecs_query_desc
(
  "is_bood_enabled_ecs_query",
  empty_span(),
  make_span(is_bood_enabled_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_bood_enabled_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_bood_enabled_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_bood_enabled_ecs_query_comps, "isBloodEnabled", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_blood_color_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("isBloodEnabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("disabledBloodColor"), ecs::ComponentTypeInfo<E3DCOLOR>()}
};
static ecs::CompileTimeQueryDesc get_blood_color_ecs_query_desc
(
  "get_blood_color_ecs_query",
  empty_span(),
  make_span(get_blood_color_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_blood_color_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_blood_color_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_blood_color_ecs_query_comps, "isBloodEnabled", bool)
            , ECS_RO_COMP(get_blood_color_ecs_query_comps, "disabledBloodColor", E3DCOLOR)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t blood_puddles__dir_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_puddles__pos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__gravity_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__itm_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
static constexpr ecs::component_t blood_splash_emitter__matrix_id_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t blood_splash_emitter__normal_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__pos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__size_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t blood_splash_emitter__targetPos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__velocity_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
