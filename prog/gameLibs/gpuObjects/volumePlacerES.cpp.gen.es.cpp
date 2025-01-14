#include "volumePlacerES.cpp.inl"
ECS_DEF_PULL_VAR(volumePlacer);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gpu_object_placer_draw_debug_geometry_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gpu_object_placer__surface_riex_handles"), ecs::ComponentTypeInfo<gpu_objects::riex_handles>()},
//start of 6 ro components at [1]
  {ECS_HASH("gpu_object_placer__min_gathered_triangle_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__triangle_edge_length_ratio_cutoff"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__object_up_vector_threshold"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("gpu_object_placer__object_density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__show_geometry"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [7]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void gpu_object_placer_draw_debug_geometry_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "gpu_object_placer__show_geometry", bool)) )
      continue;
    gpu_object_placer_draw_debug_geometry_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "gpu_object_placer__min_gathered_triangle_size", float)
    , ECS_RO_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "gpu_object_placer__triangle_edge_length_ratio_cutoff", float)
    , ECS_RO_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "gpu_object_placer__object_up_vector_threshold", Point4)
    , ECS_RO_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "gpu_object_placer__object_density", float)
    , ECS_RW_COMP(gpu_object_placer_draw_debug_geometry_es_comps, "gpu_object_placer__surface_riex_handles", gpu_objects::riex_handles)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc gpu_object_placer_draw_debug_geometry_es_es_desc
(
  "gpu_object_placer_draw_debug_geometry_es",
  "prog/gameLibs/gpuObjects/volumePlacerES.cpp.inl",
  ecs::EntitySystemOps(gpu_object_placer_draw_debug_geometry_es_all),
  make_span(gpu_object_placer_draw_debug_geometry_es_comps+0, 1)/*rw*/,
  make_span(gpu_object_placer_draw_debug_geometry_es_comps+1, 6)/*ro*/,
  make_span(gpu_object_placer_draw_debug_geometry_es_comps+7, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc gpu_object_distance_emitter_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 4 rq components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("gpu_objects_distance_emitter__range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_objects_distance_emitter__rotation_factor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_objects_distance_emitter__scale_factor"), ecs::ComponentTypeInfo<float>()}
};
static void gpu_object_distance_emitter_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gpu_objects::gpu_object_distance_emitter_es_event_handler(evt
        , ECS_RO_COMP(gpu_object_distance_emitter_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(gpu_object_distance_emitter_es_event_handler_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gpu_object_distance_emitter_es_event_handler_es_desc
(
  "gpu_object_distance_emitter_es",
  "prog/gameLibs/gpuObjects/volumePlacerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gpu_object_distance_emitter_es_event_handler_all_events),
  empty_span(),
  make_span(gpu_object_distance_emitter_es_event_handler_comps+0, 2)/*ro*/,
  make_span(gpu_object_distance_emitter_es_event_handler_comps+2, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,"gpu_objects_distance_emitter__range,gpu_objects_distance_emitter__rotation_factor,gpu_objects_distance_emitter__scale_factor,transform");
static constexpr ecs::ComponentDesc gpu_object_placer_changed_es_event_handler_comps[] =
{
//start of 11 rw components at [0]
  {ECS_HASH("gpu_object_placer__ri_asset_idx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__filled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_rendinst_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_terrain_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__object_up_vector_threshold"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_is_dirty"), ecs::ComponentTypeInfo<bool>()},
//start of 2 ro components at [11]
  {ECS_HASH("ri_gpu_object__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 23 rq components at [13]
  {ECS_HASH("gpu_object_placer__boxBorderX"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__boxBorderY"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__boxBorderZ"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__distance_based_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__object_scale_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__distance_to_scale_from"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gpu_object_placer__distance_to_scale_pow"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gpu_object_placer__distance_to_scale_to"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("gpu_object_placer__decal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distance_affect_decals"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distance_out_of_range"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distorsion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__opaque"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__place_on_geometry"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__use_distance_emitter"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distance_to_rotation_from"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__distance_to_rotation_pow"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__distance_to_rotation_to"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__min_gathered_triangle_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__min_scale_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__object_density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__object_max_count"), ecs::ComponentTypeInfo<int>()}
};
static void gpu_object_placer_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gpu_objects::gpu_object_placer_changed_es_event_handler(evt
        , ECS_RO_COMP(gpu_object_placer_changed_es_event_handler_comps, "ri_gpu_object__name", ecs::string)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__ri_asset_idx", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__filled", bool)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__buffer_offset", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__buffer_size", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__on_rendinst_geometry_count", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__on_terrain_geometry_count", int)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__object_up_vector_threshold", Point4)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__distance_emitter_eid", ecs::EntityId)
    , ECS_RW_COMP(gpu_object_placer_changed_es_event_handler_comps, "gpu_object_placer__distance_emitter_is_dirty", bool)
    , ECS_RO_COMP(gpu_object_placer_changed_es_event_handler_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gpu_object_placer_changed_es_event_handler_es_desc
(
  "gpu_object_placer_changed_es",
  "prog/gameLibs/gpuObjects/volumePlacerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gpu_object_placer_changed_es_event_handler_all_events),
  make_span(gpu_object_placer_changed_es_event_handler_comps+0, 11)/*rw*/,
  make_span(gpu_object_placer_changed_es_event_handler_comps+11, 2)/*ro*/,
  make_span(gpu_object_placer_changed_es_event_handler_comps+13, 23)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"dev,render","gpu_object_placer__boxBorderX,gpu_object_placer__boxBorderY,gpu_object_placer__boxBorderZ,gpu_object_placer__decal,gpu_object_placer__distance_affect_decals,gpu_object_placer__distance_based_scale,gpu_object_placer__distance_out_of_range,gpu_object_placer__distance_to_rotation_from,gpu_object_placer__distance_to_rotation_pow,gpu_object_placer__distance_to_rotation_to,gpu_object_placer__distance_to_scale_from,gpu_object_placer__distance_to_scale_pow,gpu_object_placer__distance_to_scale_to,gpu_object_placer__distorsion,gpu_object_placer__min_gathered_triangle_size,gpu_object_placer__min_scale_radius,gpu_object_placer__object_density,gpu_object_placer__object_max_count,gpu_object_placer__object_scale_range,gpu_object_placer__object_up_vector_threshold,gpu_object_placer__opaque,gpu_object_placer__place_on_geometry,gpu_object_placer__use_distance_emitter,ri_gpu_object__name,transform");
static constexpr ecs::ComponentDesc gpu_object_placer_create_es_event_handler_comps[] =
{
//start of 9 rw components at [0]
  {ECS_HASH("gpu_object_placer__ri_asset_idx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__filled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_rendinst_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_terrain_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__object_up_vector_threshold"), ecs::ComponentTypeInfo<Point4>()},
//start of 5 ro components at [9]
  {ECS_HASH("ri_gpu_object__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("gpu_object_placer__opaque"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__decal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distorsion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void gpu_object_placer_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gpu_objects::gpu_object_placer_create_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(gpu_object_placer_create_es_event_handler_comps, "ri_gpu_object__name", ecs::string)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__ri_asset_idx", int)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__filled", bool)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__buffer_size", int)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__on_rendinst_geometry_count", int)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__on_terrain_geometry_count", int)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__buffer_offset", int)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
    , ECS_RO_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__opaque", bool)
    , ECS_RO_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__decal", bool)
    , ECS_RO_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__distorsion", bool)
    , ECS_RO_COMP(gpu_object_placer_create_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP(gpu_object_placer_create_es_event_handler_comps, "gpu_object_placer__object_up_vector_threshold", Point4)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gpu_object_placer_create_es_event_handler_es_desc
(
  "gpu_object_placer_create_es",
  "prog/gameLibs/gpuObjects/volumePlacerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gpu_object_placer_create_es_event_handler_all_events),
  make_span(gpu_object_placer_create_es_event_handler_comps+0, 9)/*rw*/,
  make_span(gpu_object_placer_create_es_event_handler_comps+9, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gpu_object_placer_destroy_es_event_handler_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()}
};
static void gpu_object_placer_destroy_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gpu_objects::gpu_object_placer_destroy_es_event_handler(evt
        , ECS_RO_COMP(gpu_object_placer_destroy_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(gpu_object_placer_destroy_es_event_handler_comps, "gpu_object_placer__buffer_size", int)
    , ECS_RO_COMP(gpu_object_placer_destroy_es_event_handler_comps, "gpu_object_placer__buffer_offset", int)
    , ECS_RO_COMP(gpu_object_placer_destroy_es_event_handler_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
    , ECS_RO_COMP(gpu_object_placer_destroy_es_event_handler_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gpu_object_placer_destroy_es_event_handler_es_desc
(
  "gpu_object_placer_destroy_es",
  "prog/gameLibs/gpuObjects/volumePlacerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gpu_object_placer_destroy_es_event_handler_all_events),
  empty_span(),
  make_span(gpu_object_placer_destroy_es_event_handler_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gpu_object_placer_remove_objects_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gpu_object_placer__surface_riex_handles"), ecs::ComponentTypeInfo<gpu_objects::riex_handles>()}
};
static void gpu_object_placer_remove_objects_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnRendinstDamage>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gpu_object_placer_remove_objects_es_event_handler(static_cast<const EventOnRendinstDamage&>(evt)
        , ECS_RW_COMP(gpu_object_placer_remove_objects_es_event_handler_comps, "gpu_object_placer__surface_riex_handles", gpu_objects::riex_handles)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gpu_object_placer_remove_objects_es_event_handler_es_desc
(
  "gpu_object_placer_remove_objects_es",
  "prog/gameLibs/gpuObjects/volumePlacerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gpu_object_placer_remove_objects_es_event_handler_all_events),
  make_span(gpu_object_placer_remove_objects_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnRendinstDamage>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gpu_object_placer_fill_ecs_query_comps[] =
{
//start of 10 rw components at [0]
  {ECS_HASH("gpu_object_placer__current_distance_squared"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__filled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_rendinst_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_terrain_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_is_dirty"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__surface_riex_handles"), ecs::ComponentTypeInfo<gpu_objects::riex_handles>()},
//start of 26 ro components at [10]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("gpu_object_placer__ri_asset_idx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__visible_distance_squared"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__place_on_geometry"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__min_gathered_triangle_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__triangle_edge_length_ratio_cutoff"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__object_density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__object_max_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__object_scale_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__distance_based_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("gpu_object_placer__min_scale_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__object_up_vector_threshold"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("gpu_object_placer__distance_to_scale_from"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gpu_object_placer__distance_to_scale_to"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gpu_object_placer__distance_to_rotation_from"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__distance_to_rotation_to"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__distance_to_scale_pow"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("gpu_object_placer__distance_to_rotation_pow"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__use_distance_emitter"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distance_affect_decals"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distance_out_of_range"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("gpu_object_placer__boxBorderX"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("gpu_object_placer__boxBorderY"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("gpu_object_placer__boxBorderZ"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [36]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_fill_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_fill_ecs_query",
  make_span(gpu_object_placer_fill_ecs_query_comps+0, 10)/*rw*/,
  make_span(gpu_object_placer_fill_ecs_query_comps+10, 26)/*ro*/,
  make_span(gpu_object_placer_fill_ecs_query_comps+36, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_fill_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_fill_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__ri_asset_idx", int)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__visible_distance_squared", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__place_on_geometry", bool)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__min_gathered_triangle_size", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__triangle_edge_length_ratio_cutoff", float)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__current_distance_squared", float)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__filled", bool)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__buffer_offset", int)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__buffer_size", int)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__on_rendinst_geometry_count", int)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__on_terrain_geometry_count", int)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__object_density", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__object_max_count", int)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__object_scale_range", Point2)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_based_scale", Point2)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__min_scale_radius", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__object_up_vector_threshold", Point4)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_emitter_is_dirty", bool)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_emitter_eid", ecs::EntityId)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_to_scale_from", Point3)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_to_scale_to", Point3)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_to_rotation_from", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_to_rotation_to", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_to_scale_pow", Point3)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_to_rotation_pow", float)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__use_distance_emitter", bool)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_affect_decals", bool)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__distance_out_of_range", bool)
            , ECS_RW_COMP(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__surface_riex_handles", gpu_objects::riex_handles)
            , ECS_RO_COMP(gpu_object_placer_fill_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_PTR(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__boxBorderX", Point2)
            , ECS_RO_COMP_PTR(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__boxBorderY", Point2)
            , ECS_RO_COMP_PTR(gpu_object_placer_fill_ecs_query_comps, "gpu_object_placer__boxBorderZ", Point2)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_remove_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_rendinst_geometry_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__on_terrain_geometry_count"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [4]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [5]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_remove_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_remove_ecs_query",
  make_span(gpu_object_placer_remove_ecs_query_comps+0, 4)/*rw*/,
  make_span(gpu_object_placer_remove_ecs_query_comps+4, 1)/*ro*/,
  make_span(gpu_object_placer_remove_ecs_query_comps+5, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_remove_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_remove_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(gpu_object_placer_remove_ecs_query_comps, "gpu_object_placer__buffer_offset", int)
            , ECS_RW_COMP(gpu_object_placer_remove_ecs_query_comps, "gpu_object_placer__buffer_size", int)
            , ECS_RW_COMP(gpu_object_placer_remove_ecs_query_comps, "gpu_object_placer__on_rendinst_geometry_count", int)
            , ECS_RW_COMP(gpu_object_placer_remove_ecs_query_comps, "gpu_object_placer__on_terrain_geometry_count", int)
            , ECS_RO_COMP(gpu_object_placer_remove_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_copy_on_expand_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
//start of 3 ro components at [1]
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_copy_on_expand_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_copy_on_expand_ecs_query",
  make_span(gpu_object_placer_copy_on_expand_ecs_query_comps+0, 1)/*rw*/,
  make_span(gpu_object_placer_copy_on_expand_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_copy_on_expand_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_copy_on_expand_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(gpu_object_placer_copy_on_expand_ecs_query_comps, "gpu_object_placer__buffer_offset", int)
            , ECS_RO_COMP(gpu_object_placer_copy_on_expand_ecs_query_comps, "gpu_object_placer__buffer_size", int)
            , ECS_RO_COMP(gpu_object_placer_copy_on_expand_ecs_query_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
            , ECS_RO_COMP(gpu_object_placer_copy_on_expand_ecs_query_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_visibility_ecs_query_comps[] =
{
//start of 13 ro components at [0]
  {ECS_HASH("gpu_object_placer__ri_asset_idx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__current_distance_squared"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__opaque"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__decal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distorsion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__render_into_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("gpu_object_placer__filled"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [13]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_visibility_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_visibility_ecs_query",
  empty_span(),
  make_span(gpu_object_placer_visibility_ecs_query_comps+0, 13)/*ro*/,
  make_span(gpu_object_placer_visibility_ecs_query_comps+13, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__filled", bool)) )
            continue;
          function(
              ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__ri_asset_idx", int)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__buffer_size", int)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__current_distance_squared", float)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__buffer_offset", int)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__opaque", bool)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__decal", bool)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__distorsion", bool)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__render_into_shadows", bool)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "gpu_object_placer__distance_emitter_eid", ecs::EntityId)
            , ECS_RO_COMP(gpu_object_placer_visibility_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_invalidate_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("gpu_object_placer__filled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("gpu_object_placer__buffer_offset"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_buffer_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_decal_buffer_size"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_invalidate_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_invalidate_ecs_query",
  make_span(gpu_object_placer_invalidate_ecs_query_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_invalidate_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_invalidate_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(gpu_object_placer_invalidate_ecs_query_comps, "gpu_object_placer__filled", bool)
            , ECS_RW_COMP(gpu_object_placer_invalidate_ecs_query_comps, "gpu_object_placer__buffer_offset", int)
            , ECS_RW_COMP(gpu_object_placer_invalidate_ecs_query_comps, "gpu_object_placer__distance_emitter_buffer_size", int)
            , ECS_RW_COMP(gpu_object_placer_invalidate_ecs_query_comps, "gpu_object_placer__distance_emitter_decal_buffer_size", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_distance_emitter_update_after_placing_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("gpu_objects_distance_emitter__range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_objects_distance_emitter__scale_factor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gpu_objects_distance_emitter__rotation_factor"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_distance_emitter_update_after_placing_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_distance_emitter_update_after_placing_ecs_query",
  empty_span(),
  make_span(gpu_object_placer_distance_emitter_update_after_placing_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_distance_emitter_update_after_placing_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, gpu_object_placer_distance_emitter_update_after_placing_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(gpu_object_placer_distance_emitter_update_after_placing_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(gpu_object_placer_distance_emitter_update_after_placing_ecs_query_comps, "gpu_objects_distance_emitter__range", float)
            , ECS_RO_COMP(gpu_object_placer_distance_emitter_update_after_placing_ecs_query_comps, "gpu_objects_distance_emitter__scale_factor", float)
            , ECS_RO_COMP(gpu_object_placer_distance_emitter_update_after_placing_ecs_query_comps, "gpu_objects_distance_emitter__rotation_factor", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_select_closest_distance_emitter_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("gpu_objects_distance_emitter__range"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_select_closest_distance_emitter_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_select_closest_distance_emitter_ecs_query",
  empty_span(),
  make_span(gpu_object_placer_select_closest_distance_emitter_ecs_query_comps+0, 2)/*ro*/,
  make_span(gpu_object_placer_select_closest_distance_emitter_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_select_closest_distance_emitter_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_select_closest_distance_emitter_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gpu_object_placer_select_closest_distance_emitter_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(gpu_object_placer_select_closest_distance_emitter_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("gpu_object_placer__distance_emitter_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("gpu_object_placer__distance_emitter_is_dirty"), ecs::ComponentTypeInfo<bool>()},
//start of 2 ro components at [2]
  {ECS_HASH("gpu_object_placer__use_distance_emitter"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_distance_emitter_update_volume_placers_ecs_query",
  make_span(gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps+0, 2)/*rw*/,
  make_span(gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_distance_emitter_update_volume_placers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps, "gpu_object_placer__distance_emitter_eid", ecs::EntityId)
            , ECS_RW_COMP(gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps, "gpu_object_placer__distance_emitter_is_dirty", bool)
            , ECS_RO_COMP(gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps, "gpu_object_placer__use_distance_emitter", bool)
            , ECS_RO_COMP(gpu_object_placer_distance_emitter_update_volume_placers_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gpu_object_placer_get_current_distance_emitter_position_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc gpu_object_placer_get_current_distance_emitter_position_ecs_query_desc
(
  "gpu_objects::gpu_object_placer_get_current_distance_emitter_position_ecs_query",
  empty_span(),
  make_span(gpu_object_placer_get_current_distance_emitter_position_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gpu_objects::gpu_object_placer_get_current_distance_emitter_position_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, gpu_object_placer_get_current_distance_emitter_position_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(gpu_object_placer_get_current_distance_emitter_position_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
