// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "vhsCameraRendererES.cpp.inl"
ECS_DEF_PULL_VAR(vhsCameraRenderer);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("vhs_camera__resolutionDownscale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("vhs_camera__isActive"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [4]
  {ECS_HASH("vhs_camera__isPreset"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void vhs_camera_fg_node_reinit_on_res_downscale_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP_OR(vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps, "camera__active", bool( true)) && ECS_RO_COMP(vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps, "vhs_camera__isActive", bool)) )
      continue;
    vhs_camera_fg_node_reinit_on_res_downscale_change_es(evt
          , components.manager()
      , ECS_RO_COMP(vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps, "eid", ecs::EntityId)
      , ECS_RO_COMP(vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps, "vhs_camera__resolutionDownscale", float)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_fg_node_reinit_on_res_downscale_change_es_es_desc
(
  "vhs_camera_fg_node_reinit_on_res_downscale_change_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_fg_node_reinit_on_res_downscale_change_es_all_events),
  empty_span(),
  make_span(vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps+0, 4)/*ro*/,
  make_span(vhs_camera_fg_node_reinit_on_res_downscale_change_es_comps+4, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"vhs_camera__resolutionDownscale");
static constexpr ecs::ComponentDesc vhs_camera_init_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("vhsCamera"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()}
};
static void vhs_camera_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    vhs_camera_init_es(evt
        , ECS_RW_COMP(vhs_camera_init_es_comps, "vhsCamera", resource_slot::NodeHandleWithSlotsAccess)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_init_es_es_desc
(
  "vhs_camera_init_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_init_es_all_events),
  make_span(vhs_camera_init_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc vhs_camera_node_parameters_track_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("vhsCamera"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
//start of 12 ro components at [1]
  {ECS_HASH("vhs_camera__activePresets"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("vhs_camera__downscale"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__saturation"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__noise_strength"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__noise_saturation"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_min"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_max"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_gamma"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__scanline_height"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__noise_uv_scale"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__chromatic_aberration_intensity"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__chromatic_aberration_start"), ecs::ComponentTypeInfo<ShaderVar>()}
};
static void vhs_camera_node_parameters_track_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    vhs_camera_node_parameters_track_es(evt
        , components.manager()
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__activePresets", ecs::EidList)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__downscale", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__saturation", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__noise_strength", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__noise_saturation", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__dynamic_range_min", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__dynamic_range_max", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__dynamic_range_gamma", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__scanline_height", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__noise_uv_scale", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__chromatic_aberration_intensity", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__chromatic_aberration_start", ShaderVar)
    , ECS_RW_COMP(vhs_camera_node_parameters_track_es_comps, "vhsCamera", resource_slot::NodeHandleWithSlotsAccess)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_node_parameters_track_es_es_desc
(
  "vhs_camera_node_parameters_track_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_node_parameters_track_es_all_events),
  make_span(vhs_camera_node_parameters_track_es_comps+0, 1)/*rw*/,
  make_span(vhs_camera_node_parameters_track_es_comps+1, 12)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"vhs_camera__activePresets");
static constexpr ecs::ComponentDesc reinit_vhs_camera_fg_node_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("vhs_camera__activePresets"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("vhsCamera"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
//start of 1 ro components at [2]
  {ECS_HASH("vhs_camera__downscale"), ecs::ComponentTypeInfo<ShaderVar>()}
};
static ecs::CompileTimeQueryDesc reinit_vhs_camera_fg_node_ecs_query_desc
(
  "reinit_vhs_camera_fg_node_ecs_query",
  make_span(reinit_vhs_camera_fg_node_ecs_query_comps+0, 2)/*rw*/,
  make_span(reinit_vhs_camera_fg_node_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void reinit_vhs_camera_fg_node_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, reinit_vhs_camera_fg_node_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(reinit_vhs_camera_fg_node_ecs_query_comps, "vhs_camera__activePresets", ecs::EidList)
            , ECS_RO_COMP(reinit_vhs_camera_fg_node_ecs_query_comps, "vhs_camera__downscale", ShaderVar)
            , ECS_RW_COMP(reinit_vhs_camera_fg_node_ecs_query_comps, "vhsCamera", resource_slot::NodeHandleWithSlotsAccess)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc vhs_camera_preset_ecs_query_comps[] =
{
//start of 11 ro components at [0]
  {ECS_HASH("vhs_camera__resolutionDownscale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__saturationMultiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__noiseIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeMin"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeMax"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeGamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__scanlineHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__noiseUvScale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__chromaticAberrationIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__chromaticAberrationStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__noiseSaturation"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc vhs_camera_preset_ecs_query_desc
(
  "vhs_camera_preset_ecs_query",
  empty_span(),
  make_span(vhs_camera_preset_ecs_query_comps+0, 11)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void vhs_camera_preset_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, vhs_camera_preset_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__resolutionDownscale", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__saturationMultiplier", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__noiseIntensity", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__dynamicRangeMin", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__dynamicRangeMax", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__dynamicRangeGamma", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__scanlineHeight", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__noiseUvScale", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__chromaticAberrationIntensity", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__chromaticAberrationStart", float)
            , ECS_RO_COMP(vhs_camera_preset_ecs_query_comps, "vhs_camera__noiseSaturation", float)
            );

        }
    }
  );
}
