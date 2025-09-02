// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterEffectsES.cpp.inl"
ECS_DEF_PULL_VAR(waterEffects);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc attempt_to_enable_water_effects_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void attempt_to_enable_water_effects_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  attempt_to_enable_water_effects_es(evt
        );
}
static ecs::EntitySystemDesc attempt_to_enable_water_effects_es_es_desc
(
  "attempt_to_enable_water_effects_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attempt_to_enable_water_effects_es_all_events),
  empty_span(),
  empty_span(),
  make_span(attempt_to_enable_water_effects_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_up_foam_tex_request_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("use_foam_tex"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()}
};
static void set_up_foam_tex_request_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_up_foam_tex_request_es(evt
        , ECS_RO_COMP(set_up_foam_tex_request_es_comps, "water_effects", WaterEffects)
    , ECS_RW_COMP(set_up_foam_tex_request_es_comps, "use_foam_tex", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_up_foam_tex_request_es_es_desc
(
  "set_up_foam_tex_request_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_up_foam_tex_request_es_all_events),
  make_span(set_up_foam_tex_request_es_comps+0, 1)/*rw*/,
  make_span(set_up_foam_tex_request_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_water_effects_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void disable_water_effects_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  disable_water_effects_es(evt
        );
}
static ecs::EntitySystemDesc disable_water_effects_es_es_desc
(
  "disable_water_effects_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_water_effects_es_all_events),
  empty_span(),
  empty_span(),
  make_span(disable_water_effects_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc reset_water_effects_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()}
};
static void reset_water_effects_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reset_water_effects_es(evt
        , ECS_RW_COMP(reset_water_effects_es_comps, "water_effects", WaterEffects)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reset_water_effects_es_es_desc
(
  "reset_water_effects_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_water_effects_es_all_events),
  make_span(reset_water_effects_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_water_effects_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()},
  {ECS_HASH("should_use_wfx_textures"), ecs::ComponentTypeInfo<bool>()}
};
static void update_water_effects_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_water_effects_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(update_water_effects_es_comps, "water_effects", WaterEffects)
    , ECS_RW_COMP(update_water_effects_es_comps, "should_use_wfx_textures", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_water_effects_es_es_desc
(
  "update_water_effects_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_water_effects_es_all_events),
  make_span(update_water_effects_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_up_foam_params_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()}
};
static void set_up_foam_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<SetResolutionEvent>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      set_up_foam_params_es(static_cast<const SetResolutionEvent&>(evt)
            , ECS_RW_COMP(set_up_foam_params_es_comps, "water_effects", WaterEffects)
      );
    while (++comp != compE);
  } else {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      set_up_foam_params_es(evt
            , ECS_RW_COMP(set_up_foam_params_es_comps, "water_effects", WaterEffects)
      );
    while (++comp != compE);
  }
}
static ecs::EntitySystemDesc set_up_foam_params_es_es_desc
(
  "set_up_foam_params_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_up_foam_params_es_all_events),
  make_span(set_up_foam_params_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc change_effects_resolution_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()}
};
static void change_effects_resolution_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetFxQuality>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    change_effects_resolution_es(static_cast<const SetFxQuality&>(evt)
        , ECS_RW_COMP(change_effects_resolution_es_comps, "water_effects", WaterEffects)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc change_effects_resolution_es_es_desc
(
  "change_effects_resolution_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, change_effects_resolution_es_all_events),
  make_span(change_effects_resolution_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetFxQuality>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_up_water_effect_nodes_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()},
  {ECS_HASH("water_effects__init_res_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("water_effects__node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void set_up_water_effect_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_up_water_effect_nodes_es(evt
        , ECS_RW_COMP(set_up_water_effect_nodes_es_comps, "water_effects", WaterEffects)
    , ECS_RW_COMP(set_up_water_effect_nodes_es_comps, "water_effects__init_res_node", dafg::NodeHandle)
    , ECS_RW_COMP(set_up_water_effect_nodes_es_comps, "water_effects__node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_up_water_effect_nodes_es_es_desc
(
  "set_up_water_effect_nodes_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_up_water_effect_nodes_es_all_events),
  make_span(set_up_water_effect_nodes_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc water_foamfx_es_comps[] =
{
//start of 15 ro components at [0]
  {ECS_HASH("foamfx__tile_uv_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__distortion_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__normal_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__pattern_gamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__mask_gamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__gradient_gamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__underfoam_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__overfoam_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__underfoam_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__overfoam_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__underfoam_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("foamfx__overfoam_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("foamfx__reflectivity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__tile_tex"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("foamfx__gradient_tex"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void water_foamfx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_foamfx_es(evt
        , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__tile_uv_scale", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__distortion_scale", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__normal_scale", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__pattern_gamma", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__mask_gamma", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__gradient_gamma", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__underfoam_threshold", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__overfoam_threshold", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__underfoam_weight", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__overfoam_weight", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__underfoam_color", Point3)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__overfoam_color", Point3)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__reflectivity", float)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__tile_tex", ecs::string)
    , ECS_RO_COMP(water_foamfx_es_comps, "foamfx__gradient_tex", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_foamfx_es_es_desc
(
  "water_foamfx_es",
  "prog/daNetGameLibs/water_effects/render/waterEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_foamfx_es_all_events),
  empty_span(),
  make_span(water_foamfx_es_comps+0, 15)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"foamfx__distortion_scale,foamfx__gradient_gamma,foamfx__gradient_tex,foamfx__mask_gamma,foamfx__normal_scale,foamfx__overfoam_color,foamfx__overfoam_threshold,foamfx__overfoam_weight,foamfx__pattern_gamma,foamfx__reflectivity,foamfx__tile_tex,foamfx__tile_uv_scale,foamfx__underfoam_color,foamfx__underfoam_threshold,foamfx__underfoam_weight");
static constexpr ecs::ComponentDesc get_water_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static ecs::CompileTimeQueryDesc get_water_ecs_query_desc
(
  "get_water_ecs_query",
  make_span(get_water_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_water_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_water_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_water_ecs_query_comps, "water", FFTWater)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_foam_params_ecs_query_comps[] =
{
//start of 15 ro components at [0]
  {ECS_HASH("foamfx__tile_uv_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__distortion_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__normal_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__pattern_gamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__mask_gamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__gradient_gamma"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__underfoam_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__overfoam_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__underfoam_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__overfoam_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__underfoam_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("foamfx__overfoam_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("foamfx__reflectivity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foamfx__tile_tex"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("foamfx__gradient_tex"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc get_foam_params_ecs_query_desc
(
  "get_foam_params_ecs_query",
  empty_span(),
  make_span(get_foam_params_ecs_query_comps+0, 15)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_foam_params_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_foam_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__tile_uv_scale", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__distortion_scale", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__normal_scale", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__pattern_gamma", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__mask_gamma", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__gradient_gamma", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__underfoam_threshold", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__overfoam_threshold", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__underfoam_weight", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__overfoam_weight", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__underfoam_color", Point3)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__overfoam_color", Point3)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__reflectivity", float)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__tile_tex", ecs::string)
            , ECS_RO_COMP(get_foam_params_ecs_query_comps, "foamfx__gradient_tex", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_water_effects_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_effects"), ecs::ComponentTypeInfo<WaterEffects>()}
};
static ecs::CompileTimeQueryDesc get_water_effects_ecs_query_desc
(
  "get_water_effects_ecs_query",
  make_span(get_water_effects_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_water_effects_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_water_effects_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_water_effects_ecs_query_comps, "water_effects", WaterEffects)
            );

        }
    }
  );
}
