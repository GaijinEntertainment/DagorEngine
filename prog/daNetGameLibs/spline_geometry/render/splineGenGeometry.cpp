// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometry.h"
#include "splineGenGeometryRepository.h"
#include <math/integer/dag_IPoint2.h>

extern int dynamicSceneBlockId, dynamicDepthSceneBlockId;


SplineGenGeometry::SplineGenGeometry() { reset(); }

SplineGenGeometry::~SplineGenGeometry() { reset(); }

void SplineGenGeometry::reset()
{
  if (isRendered())
  {
    SplineGenGeometryManager &manager = getManager();
    manager.unregisterInstance(id, isActive());
    if (manager.hasObj())
      manager.getAsset().closeInstancingData(batchIds);
  }
  managerPtr = nullptr;
  id = INVALID_INSTANCE_ID;
  instance.objCount = 0;
  instance.flags = 0;
  inactiveFrames = 0;
  instance.bbox_lim0 = float3(100000, 100000, 100000);
  instance.bbox_lim1 = float3(-100000, -100000, -100000);
}

void SplineGenGeometry::setLod(const eastl::string &template_name)
{
  reset();
  if (template_name.empty())
    managerPtr = nullptr;
  else
    managerPtr = &get_spline_gen_repository().getOrMakeManager(template_name);
  if (!isRendered())
    return;
  SplineGenGeometryManager &manager = getManager();
  id = manager.registerInstance();
}

SplineGenGeometryManager &SplineGenGeometry::getManager()
{
  G_ASSERTF(isRendered(), "Always test for isRendered() before trying to access manager.");
  return *managerPtr;
}

void SplineGenGeometry::requestActiveState(bool request_active)
{
  if (getManager().isReactivationInProcess())
  {
    inactiveFrames = 0;
  }
  else
  {
    bool wasActive = isActive();
    if (request_active)
    {
      inactiveFrames = 0;
      if (!wasActive)
        getManager().makeInstanceActive(id);
    }
    else
    {
      inactiveFrames++;
      if (!isActive() && wasActive)
        getManager().makeInstanceInctive(id);
    }
  }
}

void SplineGenGeometry::updateInstancingData(const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec,
  const Point3 &radius,
  float displacement_strength,
  uint32_t tiles_around,
  float tile_size_meters,
  float obj_size_mul,
  float meter_between_objs)
{
  G_ASSERT(isActive());
  SplineGenGeometryManager &manager = getManager();

  uint32_t slices = manager.slices;
  uint32_t stripes = manager.stripes;
  const IPoint2 &displacementTexSize = manager.getDisplacementTexSize();

  float splineLength = 0;
  BBox3 bbox;
  bbox += spline_vec[stripes].pos;

  for (int i = 0; i < stripes; i++)
  {
    splineLength += length(spline_vec[i + 1].pos - spline_vec[i].pos);
    bbox += spline_vec[i].pos;
  }

  instance.radius = radius;
  instance.tcMul = Point2(tiles_around, splineLength / tile_size_meters);
  float texelPerSample = min(static_cast<float>(displacementTexSize.x) * instance.tcMul.x / static_cast<float>(slices),
    static_cast<float>(displacementTexSize.y) * instance.tcMul.y / static_cast<float>(stripes));
  instance.displacementLod = texelPerSample > 0 ? max(log2f(texelPerSample), 0.0f) : 1000;
  instance.displacementStrength = displacement_strength;

  float extension = max(radius.x, max(radius.y, radius.z)) * (1 + displacement_strength);

  instance.objSizeMul = obj_size_mul;
  if (manager.hasObj())
  {
    SplineGenGeometryAsset &asset = manager.getAsset();
    extension += obj_size_mul * asset.getObjectDiameter();
    uint32_t requestedObjCount = meter_between_objs > 0.0f ? floor(splineLength / meter_between_objs) : 0;
    asset.updateInstancingData(instance, batchIds, requestedObjCount);
  }
  instance.objStripeMul = safediv(instance.objCount * meter_between_objs, splineLength);

  bbox.inflate(extension);
  instance.bbox_lim0 = bbox.lim[0];
  instance.bbox_lim1 = bbox.lim[1];


  manager.updateInstancingData(id, instance, spline_vec, batchIds);

  instance.flags |= PREV_VB_VALID | PREV_ATTACHMENT_DATA_VALID;
}

void SplineGenGeometry::updateInactive()
{
  G_ASSERT(!isActive());
  instance.flags = 0; // position doesn't change if inactive, so turn off reading the prev buffer
  getManager().updateInactive(id, instance, batchIds);
}

bool SplineGenGeometry::isActive() const { return inactiveFrames <= 2; }

bool SplineGenGeometry::isRendered() const { return static_cast<bool>(managerPtr); }
