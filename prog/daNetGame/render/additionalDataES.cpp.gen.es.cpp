// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "additionalDataES.cpp.inl"
ECS_DEF_PULL_VAR(additionalData);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc additional_data_for_plane_cutting_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 3 ro components at [1]
  {ECS_HASH("plane_cutting__setId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("plane_cutting__cutting"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("plane_cutting__orMode"), ecs::ComponentTypeInfo<bool>()}
};
static void additional_data_for_plane_cutting_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    additional_data_for_plane_cutting_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(additional_data_for_plane_cutting_es_comps, "plane_cutting__setId", int)
    , ECS_RO_COMP(additional_data_for_plane_cutting_es_comps, "plane_cutting__cutting", bool)
    , ECS_RO_COMP(additional_data_for_plane_cutting_es_comps, "plane_cutting__orMode", bool)
    , ECS_RW_COMP(additional_data_for_plane_cutting_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc additional_data_for_plane_cutting_es_es_desc
(
  "additional_data_for_plane_cutting_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, additional_data_for_plane_cutting_es_all_events),
  make_span(additional_data_for_plane_cutting_es_comps+0, 1)/*rw*/,
  make_span(additional_data_for_plane_cutting_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc additional_data_for_tracks_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 2 ro components at [1]
  {ECS_HASH("vehicle_tracks_visual_pos"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("vehicle_tracks_visual_pos_delta"), ecs::ComponentTypeInfo<Point2>()}
};
static void additional_data_for_tracks_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    additional_data_for_tracks_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(additional_data_for_tracks_es_comps, "vehicle_tracks_visual_pos", Point2)
    , ECS_RO_COMP(additional_data_for_tracks_es_comps, "vehicle_tracks_visual_pos_delta", Point2)
    , ECS_RW_COMP(additional_data_for_tracks_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc additional_data_for_tracks_es_es_desc
(
  "additional_data_for_tracks_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, additional_data_for_tracks_es_all_events),
  make_span(additional_data_for_tracks_es_comps+0, 1)/*rw*/,
  make_span(additional_data_for_tracks_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc additional_data_for_vehicle_camo_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 3 ro components at [1]
  {ECS_HASH("vehicle_camo_condition"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vehicle_camo_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vehicle_camo_rotation"), ecs::ComponentTypeInfo<float>()}
};
static void additional_data_for_vehicle_camo_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    additional_data_for_vehicle_camo_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(additional_data_for_vehicle_camo_es_comps, "vehicle_camo_condition", float)
    , ECS_RO_COMP(additional_data_for_vehicle_camo_es_comps, "vehicle_camo_scale", float)
    , ECS_RO_COMP(additional_data_for_vehicle_camo_es_comps, "vehicle_camo_rotation", float)
    , ECS_RW_COMP(additional_data_for_vehicle_camo_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc additional_data_for_vehicle_camo_es_es_desc
(
  "additional_data_for_vehicle_camo_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, additional_data_for_vehicle_camo_es_all_events),
  make_span(additional_data_for_vehicle_camo_es_comps+0, 1)/*rw*/,
  make_span(additional_data_for_vehicle_camo_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc on_vehicle_camo_changed_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 3 rq components at [1]
  {ECS_HASH("vehicle_camo_condition"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vehicle_camo_rotation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("vehicle_camo_scale"), ecs::ComponentTypeInfo<float>()}
};
static void on_vehicle_camo_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_vehicle_camo_changed_es(evt
        , ECS_RO_COMP(on_vehicle_camo_changed_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_vehicle_camo_changed_es_es_desc
(
  "on_vehicle_camo_changed_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_vehicle_camo_changed_es_all_events),
  empty_span(),
  make_span(on_vehicle_camo_changed_es_comps+0, 1)/*ro*/,
  make_span(on_vehicle_camo_changed_es_comps+1, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","vehicle_camo_condition,vehicle_camo_rotation,vehicle_camo_scale");
static constexpr ecs::ComponentDesc additional_data_copy_model_tm_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 1 ro components at [1]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 1 rq components at [2]
  {ECS_HASH("needModelMatrix"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void additional_data_copy_model_tm_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    additional_data_copy_model_tm_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(additional_data_copy_model_tm_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RW_COMP(additional_data_copy_model_tm_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc additional_data_copy_model_tm_es_es_desc
(
  "additional_data_copy_model_tm_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, additional_data_copy_model_tm_es_all_events),
  make_span(additional_data_copy_model_tm_es_comps+0, 1)/*rw*/,
  make_span(additional_data_copy_model_tm_es_comps+1, 1)/*ro*/,
  make_span(additional_data_copy_model_tm_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc validate_dynamic_material_params_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dynamic_material_channels_arr"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void validate_dynamic_material_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_dynamic_material_params_es_event_handler(evt
        , ECS_RO_COMP(validate_dynamic_material_params_es_event_handler_comps, "dynamic_material_channels_arr", ecs::Array)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_dynamic_material_params_es_event_handler_es_desc
(
  "validate_dynamic_material_params_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_dynamic_material_params_es_event_handler_all_events),
  empty_span(),
  make_span(validate_dynamic_material_params_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc additional_data_for_dynamic_material_params_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 1 ro components at [1]
  {ECS_HASH("dynamic_material_channels_arr"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void additional_data_for_dynamic_material_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    additional_data_for_dynamic_material_params_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(additional_data_for_dynamic_material_params_es_comps, "dynamic_material_channels_arr", ecs::Array)
    , ECS_RW_COMP(additional_data_for_dynamic_material_params_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc additional_data_for_dynamic_material_params_es_es_desc
(
  "additional_data_for_dynamic_material_params_es",
  "prog/daNetGame/render/additionalDataES.cpp.inl",
  ecs::EntitySystemOps(nullptr, additional_data_for_dynamic_material_params_es_all_events),
  make_span(additional_data_for_dynamic_material_params_es_comps+0, 1)/*rw*/,
  make_span(additional_data_for_dynamic_material_params_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
