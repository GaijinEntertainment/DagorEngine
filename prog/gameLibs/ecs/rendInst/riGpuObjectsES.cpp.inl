// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/gpuObjects.h>
#include <rendInst/rendInstGen.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <math/dag_mathUtils.h>
#include <gpuObjects/placingParameters.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <ecs/rendInst/riExtra.h>

static rendinst::gpuobjects::AllowGpuRiCb allow_gpu_ri = nullptr;

void rendinst::gpuobjects::set_allow_gpu_ri_cb(rendinst::gpuobjects::AllowGpuRiCb cb) { allow_gpu_ri = cb; }

template <typename T>
static void perform_multiple_objects(T &&lambda, const ecs::Array &multiple_objects, gpu_objects::PlacingParameters &params)
{
  float weightsSum = params.sparseWeight;
  float weightOffset = 0;
  for (const auto &object : multiple_objects)
  {
    const ecs::Object &ecsObject = object.get<ecs::Object>();
    weightsSum += ecsObject[ECS_HASH("weight")].get<float>();
  }
  for (const auto &object : multiple_objects)
  {
    const ecs::Object &ecsObject = object.get<ecs::Object>();
    float weight = ecsObject[ECS_HASH("weight")].get<float>();
    const ecs::string &name = ecsObject[ECS_HASH("name")].get<ecs::string>();
    weight /= weightsSum;
    params.weightRange = Point2(weightOffset, weightOffset + weight);
    weightOffset += weight;
    lambda(name, params);
  }
}

gpu_objects::PlacingParameters gpu_objects::prepare_gpu_object_parameters(int ri_gpu_object__seed,
  const Point3 &ri_gpu_object__up_vector, float ri_gpu_object__incline_delta, const Point2 &ri_gpu_object__scale_range,
  const Point2 &ri_gpu_object__rotate_range, const Point4 &ri_gpu_object__biom_indexes, const bool ri_gpu_object__is_using_normal,
  const eastl::string &ri_gpu_object__map, const Point2 &ri_gpu_object__map_size, const Point2 &ri_gpu_object__map_offset,
  const E3DCOLOR &ri_gpu_object__color_from, const E3DCOLOR &ri_gpu_object__color_to, const Point2 &ri_gpu_object_slope_factor,
  const ecs::Array &ri_gpu_object__biome_params, const float &ri_gpu_object__hardness, const bool ri_gpu_object__decal,
  const bool ri_gpu_object__transparent, const bool ri_gpu_object__distorsion, const float &ri_gpu_object__sparse_weight,
  const bool ri_gpu_object__place_on_water, const bool ri_gpu_object__render_into_shadows, const Point2 &ri_gpu_object__coast_range,
  const bool ri_gpu_object__face_coast)
{
  gpu_objects::PlacingParameters parameters;
  parameters.seed = ri_gpu_object__seed % (1 << 14); // We assume, that 128x128 noise texture is used.
  if (!ri_gpu_object__is_using_normal)
  {
    parameters.upVector = ri_gpu_object__up_vector;
    parameters.upVector.normalize();
  }
  else
    parameters.upVector = Point3(0, 0, 0);
  parameters.inclineDelta = sin(DegToRad(ri_gpu_object__incline_delta));
  parameters.scale = ri_gpu_object__scale_range;
  const float rotateAngle = DegToRad(ri_gpu_object__rotate_range.x);
  const float rotateDelta = DegToRad(ri_gpu_object__rotate_range.y);
  parameters.rotate = Point2(rotateAngle - rotateDelta, rotateAngle + rotateDelta);
  parameters.map = ri_gpu_object__map;
  parameters.mapSizeOffset.x = 1.0 / ri_gpu_object__map_size.x;
  parameters.mapSizeOffset.y = -1.0 / ri_gpu_object__map_size.y;
  parameters.mapSizeOffset.z = -ri_gpu_object__map_offset.x / ri_gpu_object__map_size.x;
  parameters.mapSizeOffset.w = ri_gpu_object__map_offset.y / ri_gpu_object__map_size.y;
  parameters.colorFrom = ri_gpu_object__color_from;
  parameters.colorTo = ri_gpu_object__color_to;
  parameters.slopeFactor.r = ri_gpu_object_slope_factor.x;
  parameters.slopeFactor.g = 1 - 1.0 / ri_gpu_object_slope_factor.x;
  parameters.slopeFactor.b = ri_gpu_object_slope_factor.y > 0 ? 1 : -1;
  parameters.slopeFactor.a = abs(ri_gpu_object_slope_factor.y);
  parameters.hardness = ri_gpu_object__hardness;
  parameters.placeOnWater = ri_gpu_object__place_on_water;
  parameters.renderIntoShadows = ri_gpu_object__render_into_shadows;
  parameters.coastRange = ri_gpu_object__coast_range;
  parameters.faceCoast = ri_gpu_object__face_coast;

  for (const auto &params : ri_gpu_object__biome_params)
    parameters.biomes.push_back(params.get<Point4>());

  if (parameters.biomes.size() == 0)
  {
    if (ri_gpu_object__biom_indexes.x >= 0)
      parameters.biomes.push_back(Point4(ri_gpu_object__biom_indexes.x, 1, 0, 0));
    if (ri_gpu_object__biom_indexes.y >= 0)
      parameters.biomes.push_back(Point4(ri_gpu_object__biom_indexes.y, 1, 0, 0));
    if (ri_gpu_object__biom_indexes.z >= 0)
      parameters.biomes.push_back(Point4(ri_gpu_object__biom_indexes.z, 1, 0, 0));
    if (ri_gpu_object__biom_indexes.w >= 0)
      parameters.biomes.push_back(Point4(ri_gpu_object__biom_indexes.w, 1, 0, 0));
  }
  parameters.decal = ri_gpu_object__decal;
  parameters.transparent = ri_gpu_object__transparent;
  parameters.distorsion = ri_gpu_object__distorsion;
  parameters.sparseWeight = ri_gpu_object__sparse_weight;

  return parameters;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static __forceinline void ri_gpu_object_create_es_event_handler(const ecs::Event &, ecs::EntityId eid,
  const ecs::string &ri_gpu_object__name, int ri_gpu_object__grid_tiling, int ri_gpu_object__grid_size, float ri_gpu_object__cell_size,
  int ri_gpu_object__seed, const Point3 &ri_gpu_object__up_vector, float ri_gpu_object__incline_delta,
  const Point2 &ri_gpu_object__scale_range, const Point2 &ri_gpu_object__rotate_range, const Point4 &ri_gpu_object__biom_indexes,
  const bool ri_gpu_object__is_using_normal, const ecs::string &ri_gpu_object__map, const Point2 &ri_gpu_object__map_size,
  const Point2 &ri_gpu_object__map_offset, const E3DCOLOR &ri_gpu_object__color_from, const E3DCOLOR &ri_gpu_object__color_to,
  const Point2 &ri_gpu_object__slope_factor, const ecs::Array &ri_gpu_object__biome_params,
  const ecs::Array &ri_gpu_object__multiple_objects, float ri_gpu_object__sparse_weight, const float &ri_gpu_object__hardness,
  const bool ri_gpu_object__decal, const bool ri_gpu_object__transparent, const bool ri_gpu_object__distorsion,
  const bool ri_gpu_object__place_on_water, const bool ri_gpu_object__render_into_shadows, const Point2 &ri_gpu_object__coast_range,
  const bool ri_gpu_object__face_coast)
{
  if (allow_gpu_ri && !allow_gpu_ri())
  {
    if (!g_entity_mgr->has(eid, ECS_HASH("disabled_update")))
      add_sub_template_async(eid, "disabled_update");
    return;
  }

  gpu_objects::PlacingParameters parameters = gpu_objects::prepare_gpu_object_parameters(ri_gpu_object__seed, ri_gpu_object__up_vector,
    ri_gpu_object__incline_delta, ri_gpu_object__scale_range, ri_gpu_object__rotate_range, ri_gpu_object__biom_indexes,
    ri_gpu_object__is_using_normal, ri_gpu_object__map, ri_gpu_object__map_size, ri_gpu_object__map_offset, ri_gpu_object__color_from,
    ri_gpu_object__color_to, ri_gpu_object__slope_factor, ri_gpu_object__biome_params, ri_gpu_object__hardness, ri_gpu_object__decal,
    ri_gpu_object__transparent, ri_gpu_object__distorsion, ri_gpu_object__sparse_weight, ri_gpu_object__place_on_water,
    ri_gpu_object__render_into_shadows, ri_gpu_object__coast_range, ri_gpu_object__face_coast);

  if (ri_gpu_object__multiple_objects.empty())
    rendinst::gpuobjects::add(ri_gpu_object__name, ri_gpu_object__grid_tiling, ri_gpu_object__grid_size, ri_gpu_object__cell_size,
      parameters);
  else
    perform_multiple_objects(
      [&](const eastl::string &name, const gpu_objects::PlacingParameters &parameters) {
        rendinst::gpuobjects::add(name, ri_gpu_object__grid_tiling, ri_gpu_object__grid_size, ri_gpu_object__cell_size, parameters);
      },
      ri_gpu_object__multiple_objects, parameters);
}

ECS_REQUIRE_NOT(ecs::Tag disabled)
ECS_TAG(render)
ECS_TRACK(*)
static __forceinline void ri_gpu_object_update_params_es_event_handler(const ecs::Event &, const ecs::string &ri_gpu_object__name,
  int ri_gpu_object__seed, const Point3 &ri_gpu_object__up_vector, float ri_gpu_object__incline_delta,
  const Point2 &ri_gpu_object__scale_range, const Point2 &ri_gpu_object__rotate_range, const Point4 &ri_gpu_object__biom_indexes,
  const bool ri_gpu_object__is_using_normal, const ecs::string &ri_gpu_object__map, const Point2 &ri_gpu_object__map_size,
  const Point2 &ri_gpu_object__map_offset, const E3DCOLOR &ri_gpu_object__color_from, const E3DCOLOR &ri_gpu_object__color_to,
  const Point2 &ri_gpu_object__slope_factor, const ecs::Array &ri_gpu_object__biome_params,
  const ecs::Array &ri_gpu_object__multiple_objects, float ri_gpu_object__sparse_weight, const float &ri_gpu_object__hardness,
  const bool ri_gpu_object__decal, const bool ri_gpu_object__transparent, const bool ri_gpu_object__distorsion,
  const bool ri_gpu_object__place_on_water, const bool ri_gpu_object__render_into_shadows, const Point2 &ri_gpu_object__coast_range,
  const bool ri_gpu_object__face_coast)
{
  gpu_objects::PlacingParameters parameters = gpu_objects::prepare_gpu_object_parameters(ri_gpu_object__seed, ri_gpu_object__up_vector,
    ri_gpu_object__incline_delta, ri_gpu_object__scale_range, ri_gpu_object__rotate_range, ri_gpu_object__biom_indexes,
    ri_gpu_object__is_using_normal, ri_gpu_object__map, ri_gpu_object__map_size, ri_gpu_object__map_offset, ri_gpu_object__color_from,
    ri_gpu_object__color_to, ri_gpu_object__slope_factor, ri_gpu_object__biome_params, ri_gpu_object__hardness, ri_gpu_object__decal,
    ri_gpu_object__transparent, ri_gpu_object__distorsion, ri_gpu_object__sparse_weight, ri_gpu_object__place_on_water,
    ri_gpu_object__render_into_shadows, ri_gpu_object__coast_range, ri_gpu_object__face_coast);

  if (ri_gpu_object__multiple_objects.empty())
    rendinst::gpuobjects::change_parameters(ri_gpu_object__name, parameters);
  else
    perform_multiple_objects(
      [](const eastl::string &name, const gpu_objects::PlacingParameters &parameters) {
        rendinst::gpuobjects::change_parameters(name, parameters);
      },
      ri_gpu_object__multiple_objects, parameters);
}

ECS_REQUIRE_NOT(ecs::Tag disabled)
ECS_TAG(render)
ECS_TRACK(*)
static __forceinline void ri_gpu_object_track_es_event_handler(const ecs::Event &, const ecs::string &ri_gpu_object__name,
  int ri_gpu_object__grid_tiling, int ri_gpu_object__grid_size, float ri_gpu_object__cell_size,
  const ecs::Array &ri_gpu_object__multiple_objects)
{
  if (ri_gpu_object__multiple_objects.empty())
    rendinst::gpuobjects::change_grid(ri_gpu_object__name, ri_gpu_object__grid_tiling, ri_gpu_object__grid_size,
      ri_gpu_object__cell_size);
  else
  {
    gpu_objects::PlacingParameters parameters;
    perform_multiple_objects(
      [&](const eastl::string &name, const gpu_objects::PlacingParameters &) {
        rendinst::gpuobjects::change_grid(name, ri_gpu_object__grid_tiling, ri_gpu_object__grid_size, ri_gpu_object__cell_size);
      },
      ri_gpu_object__multiple_objects, parameters);
  }
}

struct GpuObjectRiResourcePreload
{
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    ecs::Array defArray;
    const ecs::Array &objects = res_cb.getOr<ecs::Array>(ECS_HASH("ri_gpu_object__multiple_objects"), defArray);
    if (objects.empty())
    {
      const ecs::string &riResName = res_cb.get<ecs::string>(ECS_HASH("ri_gpu_object__name"));
      request_ri_resources(res_cb, riResName.c_str());
    }
    else
    {
      for (const auto &object : objects)
      {
        const ecs::Object &ecsObject = object.get<ecs::Object>();
        const ecs::string &riResName = ecsObject[ECS_HASH("name")].get<ecs::string>();
        request_ri_resources(res_cb, riResName.c_str());
      }
    }
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(GpuObjectRiResourcePreload);
ECS_REGISTER_RELOCATABLE_TYPE(GpuObjectRiResourcePreload, nullptr);
