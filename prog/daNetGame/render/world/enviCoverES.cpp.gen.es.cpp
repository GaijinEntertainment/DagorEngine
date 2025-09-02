// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "enviCoverES.cpp.inl"
ECS_DEF_PULL_VAR(enviCover);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc envi_cover_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("envi_cover"), ecs::ComponentTypeInfo<bool>()}
};
static void envi_cover_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    envi_cover_es(evt
        , ECS_RO_COMP(envi_cover_es_comps, "envi_cover", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc envi_cover_es_es_desc
(
  "envi_cover_es",
  "prog/daNetGame/render/world/enviCoverES.cpp.inl",
  ecs::EntitySystemOps(nullptr, envi_cover_es_all_events),
  empty_span(),
  make_span(envi_cover_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc envi_cover_unload_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("envi_cover"), ecs::ComponentTypeInfo<bool>()}
};
static void envi_cover_unload_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  envi_cover_unload_es(evt
        );
}
static ecs::EntitySystemDesc envi_cover_unload_es_es_desc
(
  "envi_cover_unload_es",
  "prog/daNetGame/render/world/enviCoverES.cpp.inl",
  ecs::EntitySystemOps(nullptr, envi_cover_unload_es_all_events),
  empty_span(),
  empty_span(),
  make_span(envi_cover_unload_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_envi_cover_params_es_comps[] =
{
//start of 18 ro components at [0]
  {ECS_HASH("envi_cover_intensity_map_left_top_right_bottom"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("envi_cover_intensity_map"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("envi_cover_albedo"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("envi_cover_normal"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("envi_cover_reflectance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_translucency"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_smoothness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_normal_infl"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_depth_smoothstep_max"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_depth_pow_exponent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_noise_high_frequency"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_noise_low_frequency"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_noise_mask_factor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_depth_mask_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_normal_mask_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_depth_mask_contrast"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_normal_mask_contrast"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("envi_cover_lowest_intensity"), ecs::ComponentTypeInfo<float>()}
};
static void set_envi_cover_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_envi_cover_params_es(evt
        , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_intensity_map_left_top_right_bottom", Point4)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_intensity_map", ecs::string)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_albedo", Point4)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_normal", Point4)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_reflectance", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_translucency", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_smoothness", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_normal_infl", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_depth_smoothstep_max", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_depth_pow_exponent", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_noise_high_frequency", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_noise_low_frequency", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_noise_mask_factor", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_depth_mask_threshold", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_normal_mask_threshold", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_depth_mask_contrast", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_normal_mask_contrast", float)
    , ECS_RO_COMP(set_envi_cover_params_es_comps, "envi_cover_lowest_intensity", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_envi_cover_params_es_es_desc
(
  "set_envi_cover_params_es",
  "prog/daNetGame/render/world/enviCoverES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_envi_cover_params_es_all_events),
  empty_span(),
  make_span(set_envi_cover_params_es_comps+0, 18)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render",nullptr,"*");
