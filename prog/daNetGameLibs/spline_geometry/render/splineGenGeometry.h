// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "splineGenGeometryPointers.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

class SplineGenGeometryManager;

// Entity component of spline_gen_geometry
// It registers into SplineGenGeometryManager, which reserves buffer space for it
// Negotiates attached objects with SplineGenGeometryManager's SplineGenGeometryAsset class (if hasObj)
// Can be marked inactive from entity. The first frame it's set inactive it still requires valid inputs.
// After that the caller code needs to guarantee that the state doesn't change, until it's active again for minimum a frame.
// Internally it's active for 2 frames at least, and reactivates if the buffers are reallocated.
class SplineGenGeometry
{
public:
  SplineGenGeometry();
  SplineGenGeometry(SplineGenGeometry &&) = default;
  ~SplineGenGeometry();
  bool onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid);
  void reset();
  void setLod(const eastl::string &template_name);
  SplineGenGeometryManager &getManager();
  void requestActiveState(bool request_active);
  bool isActive() const;
  bool isRendered() const;
  void updateInstancingData(const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec,
    float max_radius,
    float displacement_strength,
    uint32_t tiles_around,
    float tile_size_meters,
    float obj_size_mul,
    float meter_between_objs,
    const Point4 &emissive_color,
    float cylinder_start_offset,
    float index_of_refraction,
    const Point4 &uv_scroll_first_offset_and_scale,
    const Point4 &uv_scroll_second_offset_and_scale,
    float uv_scroll_interpolation_value,
    float surface_opaqueness,
    const Point2 &additional_thickness_bounds);
  void updateAttachmentBatchIds();

private:
  SplineGenGeometryManagerPtr managerPtr;
  InstanceId id = INVALID_INSTANCE_ID;
  SplineGenInstance instance;
  eastl::vector<BatchId> batchIds;
  int inactiveFrames = 0;
};

ECS_DECLARE_BOXED_TYPE(SplineGenGeometry);
