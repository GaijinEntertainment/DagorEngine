#include "vhsCameraRendererES.cpp.inl"
ECS_DEF_PULL_VAR(vhsCameraRenderer);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc vhs_camera_preset_add_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("vhs_camera__isActive"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("vhs_camera__isPreset"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void vhs_camera_preset_add_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    vhs_camera_preset_add_es(evt
        , ECS_RO_COMP(vhs_camera_preset_add_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(vhs_camera_preset_add_es_comps, "vhs_camera__isActive", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_preset_add_es_es_desc
(
  "vhs_camera_preset_add_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_preset_add_es_all_events),
  empty_span(),
  make_span(vhs_camera_preset_add_es_comps+0, 2)/*ro*/,
  make_span(vhs_camera_preset_add_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc vhs_camera_preset_remove_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("vhs_camera__isPreset"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void vhs_camera_preset_remove_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    vhs_camera_preset_remove_es(evt
        , ECS_RO_COMP(vhs_camera_preset_remove_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_preset_remove_es_es_desc
(
  "vhs_camera_preset_remove_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_preset_remove_es_all_events),
  empty_span(),
  make_span(vhs_camera_preset_remove_es_comps+0, 1)/*ro*/,
  make_span(vhs_camera_preset_remove_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc vhs_camera_preset_active_track_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("vhs_camera__isActive"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("vhs_camera__isPreset"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void vhs_camera_preset_active_track_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    vhs_camera_preset_active_track_es(evt
        , ECS_RO_COMP(vhs_camera_preset_active_track_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(vhs_camera_preset_active_track_es_comps, "vhs_camera__isActive", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_preset_active_track_es_es_desc
(
  "vhs_camera_preset_active_track_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_preset_active_track_es_all_events),
  empty_span(),
  make_span(vhs_camera_preset_active_track_es_comps+0, 2)/*ro*/,
  make_span(vhs_camera_preset_active_track_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"vhs_camera__isActive");
static constexpr ecs::ComponentDesc vhs_camera_preset_params_track_es_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("vhs_camera__resolutionDownscale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__saturationMultiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__noiseIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeMin"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeMax"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeGamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__scanlineHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("vhs_camera__isActive"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [10]
  {ECS_HASH("vhs_camera__isPreset"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void vhs_camera_preset_params_track_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP_OR(vhs_camera_preset_params_track_es_comps, "camera__active", bool( true)) && ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__isActive", bool)) )
      continue;
    vhs_camera_preset_params_track_es(evt
          , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "eid", ecs::EntityId)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__resolutionDownscale", float)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__saturationMultiplier", float)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__noiseIntensity", float)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__dynamicRangeMin", float)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__dynamicRangeMax", float)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__dynamicRangeGamma", float)
      , ECS_RO_COMP(vhs_camera_preset_params_track_es_comps, "vhs_camera__scanlineHeight", float)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc vhs_camera_preset_params_track_es_es_desc
(
  "vhs_camera_preset_params_track_es",
  "prog/daNetGameLibs/screen_vhs/render/vhsCameraRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, vhs_camera_preset_params_track_es_all_events),
  empty_span(),
  make_span(vhs_camera_preset_params_track_es_comps+0, 10)/*ro*/,
  make_span(vhs_camera_preset_params_track_es_comps+10, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"*");
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
//start of 8 ro components at [1]
  {ECS_HASH("vhs_camera__activePresets"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("vhs_camera__downscale"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__saturation"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__noise_strength"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_min"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_max"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_gamma"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__scanline_height"), ecs::ComponentTypeInfo<ShaderVar>()}
};
static void vhs_camera_node_parameters_track_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    vhs_camera_node_parameters_track_es(evt
        , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__activePresets", ecs::EidList)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__downscale", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__saturation", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__noise_strength", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__dynamic_range_min", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__dynamic_range_max", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__dynamic_range_gamma", ShaderVar)
    , ECS_RO_COMP(vhs_camera_node_parameters_track_es_comps, "vhs_camera__scanline_height", ShaderVar)
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
  make_span(vhs_camera_node_parameters_track_es_comps+1, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"vhs_camera__activePresets");
static constexpr ecs::ComponentDesc get_vhs_camera_presets_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("vhs_camera__activePresets"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static ecs::CompileTimeQueryDesc get_vhs_camera_presets_ecs_query_desc
(
  "get_vhs_camera_presets_ecs_query",
  make_span(get_vhs_camera_presets_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_vhs_camera_presets_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_vhs_camera_presets_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_vhs_camera_presets_ecs_query_comps, "vhs_camera__activePresets", ecs::EidList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_vhs_camera_shader_vars_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("vhs_camera__activePresets"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("vhsCamera"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
//start of 7 ro components at [2]
  {ECS_HASH("vhs_camera__downscale"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__saturation"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__noise_strength"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_min"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_max"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__dynamic_range_gamma"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("vhs_camera__scanline_height"), ecs::ComponentTypeInfo<ShaderVar>()}
};
static ecs::CompileTimeQueryDesc get_vhs_camera_shader_vars_ecs_query_desc
(
  "get_vhs_camera_shader_vars_ecs_query",
  make_span(get_vhs_camera_shader_vars_ecs_query_comps+0, 2)/*rw*/,
  make_span(get_vhs_camera_shader_vars_ecs_query_comps+2, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_vhs_camera_shader_vars_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_vhs_camera_shader_vars_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__activePresets", ecs::EidList)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__downscale", ShaderVar)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__saturation", ShaderVar)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__noise_strength", ShaderVar)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__dynamic_range_min", ShaderVar)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__dynamic_range_max", ShaderVar)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__dynamic_range_gamma", ShaderVar)
            , ECS_RO_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhs_camera__scanline_height", ShaderVar)
            , ECS_RW_COMP(get_vhs_camera_shader_vars_ecs_query_comps, "vhsCamera", resource_slot::NodeHandleWithSlotsAccess)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc vhs_camera_preset_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("vhs_camera__resolutionDownscale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__saturationMultiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__noiseIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeMin"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeMax"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__dynamicRangeGamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vhs_camera__scanlineHeight"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc vhs_camera_preset_ecs_query_desc
(
  "vhs_camera_preset_ecs_query",
  empty_span(),
  make_span(vhs_camera_preset_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void vhs_camera_preset_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, vhs_camera_preset_ecs_query_desc.getHandle(),
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
            );

        }
    }
  );
}
