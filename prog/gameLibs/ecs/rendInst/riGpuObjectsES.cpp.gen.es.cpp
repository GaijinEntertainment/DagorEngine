#include "riGpuObjectsES.cpp.inl"
ECS_DEF_PULL_VAR(riGpuObjects);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ri_gpu_object_create_es_event_handler_comps[] =
{
//start of 30 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("ri_gpu_object__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_gpu_object__grid_tiling"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ri_gpu_object__grid_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ri_gpu_object__cell_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__seed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ri_gpu_object__up_vector"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_gpu_object__incline_delta"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__scale_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__rotate_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__biom_indexes"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("ri_gpu_object__is_using_normal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__map"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_gpu_object__map_size"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__map_offset"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__color_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("ri_gpu_object__color_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("ri_gpu_object__slope_factor"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__biome_params"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("ri_gpu_object__multiple_objects"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("ri_gpu_object__sparse_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__hardness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__decal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__transparent"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__distorsion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__place_on_water"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__enable_displacement"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__render_into_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__coast_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__face_coast"), ecs::ComponentTypeInfo<bool>()}
};
static void ri_gpu_object_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_gpu_object_create_es_event_handler(evt
        , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__name", ecs::string)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__grid_tiling", int)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__grid_size", int)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__cell_size", float)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__seed", int)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__up_vector", Point3)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__incline_delta", float)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__scale_range", Point2)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__rotate_range", Point2)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__biom_indexes", Point4)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__is_using_normal", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__map", ecs::string)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__map_size", Point2)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__map_offset", Point2)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__color_from", E3DCOLOR)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__color_to", E3DCOLOR)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__slope_factor", Point2)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__biome_params", ecs::Array)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__multiple_objects", ecs::Array)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__sparse_weight", float)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__hardness", float)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__decal", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__transparent", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__distorsion", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__place_on_water", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__enable_displacement", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__render_into_shadows", bool)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__coast_range", Point2)
    , ECS_RO_COMP(ri_gpu_object_create_es_event_handler_comps, "ri_gpu_object__face_coast", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_gpu_object_create_es_event_handler_es_desc
(
  "ri_gpu_object_create_es",
  "prog/gameLibs/ecs/rendInst/./riGpuObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_gpu_object_create_es_event_handler_all_events),
  empty_span(),
  make_span(ri_gpu_object_create_es_event_handler_comps+0, 30)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc ri_gpu_object_update_params_es_event_handler_comps[] =
{
//start of 26 ro components at [0]
  {ECS_HASH("ri_gpu_object__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_gpu_object__seed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ri_gpu_object__up_vector"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_gpu_object__incline_delta"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__scale_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__rotate_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__biom_indexes"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("ri_gpu_object__is_using_normal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__map"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_gpu_object__map_size"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__map_offset"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__color_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("ri_gpu_object__color_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("ri_gpu_object__slope_factor"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__biome_params"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("ri_gpu_object__multiple_objects"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("ri_gpu_object__sparse_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__hardness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__decal"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__transparent"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__distorsion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__place_on_water"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__enable_displacement"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__render_into_shadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_gpu_object__coast_range"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("ri_gpu_object__face_coast"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [26]
  {ECS_HASH("disabled"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ri_gpu_object_update_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_gpu_object_update_params_es_event_handler(evt
        , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__name", ecs::string)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__seed", int)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__up_vector", Point3)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__incline_delta", float)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__scale_range", Point2)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__rotate_range", Point2)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__biom_indexes", Point4)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__is_using_normal", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__map", ecs::string)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__map_size", Point2)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__map_offset", Point2)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__color_from", E3DCOLOR)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__color_to", E3DCOLOR)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__slope_factor", Point2)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__biome_params", ecs::Array)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__multiple_objects", ecs::Array)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__sparse_weight", float)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__hardness", float)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__decal", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__transparent", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__distorsion", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__place_on_water", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__enable_displacement", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__render_into_shadows", bool)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__coast_range", Point2)
    , ECS_RO_COMP(ri_gpu_object_update_params_es_event_handler_comps, "ri_gpu_object__face_coast", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_gpu_object_update_params_es_event_handler_es_desc
(
  "ri_gpu_object_update_params_es",
  "prog/gameLibs/ecs/rendInst/./riGpuObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_gpu_object_update_params_es_event_handler_all_events),
  empty_span(),
  make_span(ri_gpu_object_update_params_es_event_handler_comps+0, 26)/*ro*/,
  empty_span(),
  make_span(ri_gpu_object_update_params_es_event_handler_comps+26, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc ri_gpu_object_track_es_event_handler_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("ri_gpu_object__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_gpu_object__grid_tiling"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ri_gpu_object__grid_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ri_gpu_object__cell_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("ri_gpu_object__multiple_objects"), ecs::ComponentTypeInfo<ecs::Array>()},
//start of 1 no components at [5]
  {ECS_HASH("disabled"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ri_gpu_object_track_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_gpu_object_track_es_event_handler(evt
        , ECS_RO_COMP(ri_gpu_object_track_es_event_handler_comps, "ri_gpu_object__name", ecs::string)
    , ECS_RO_COMP(ri_gpu_object_track_es_event_handler_comps, "ri_gpu_object__grid_tiling", int)
    , ECS_RO_COMP(ri_gpu_object_track_es_event_handler_comps, "ri_gpu_object__grid_size", int)
    , ECS_RO_COMP(ri_gpu_object_track_es_event_handler_comps, "ri_gpu_object__cell_size", float)
    , ECS_RO_COMP(ri_gpu_object_track_es_event_handler_comps, "ri_gpu_object__multiple_objects", ecs::Array)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_gpu_object_track_es_event_handler_es_desc
(
  "ri_gpu_object_track_es",
  "prog/gameLibs/ecs/rendInst/./riGpuObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_gpu_object_track_es_event_handler_all_events),
  empty_span(),
  make_span(ri_gpu_object_track_es_event_handler_comps+0, 5)/*ro*/,
  empty_span(),
  make_span(ri_gpu_object_track_es_event_handler_comps+5, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  0
,"render","*");
