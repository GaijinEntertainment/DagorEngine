// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvDistToBorder.h"

#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderObject.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

#include <3d/dag_render.h>

#include "math/dag_math2d.h"
#include "math/dag_math3d.h"
#include "math/dag_mathUtils.h"
#include "math/dag_bounds3.h"

#include "ui/uiShared.h"

using namespace darg;

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvDistToBorder, bhv_dist_to_border, cstr);

template <typename Callable>
static bool get_poly_capzone_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static bool get_sphere_capzone_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static bool get_box_capzone_ecs_query(ecs::EntityId, Callable c);

static float distance_to_polyCapzone(
  const Point2 *areaPoints, size_t numPoints, const Point3 &fromPos, float minHeight, float maxHeight)
{
  float resDistance = 0.0;
  const Point2 *point = areaPoints;
  Point2 a = point[numPoints - 1], b;
  float minSquareDist = VERY_BIG_NUMBER;
  for (const Point2 *pb = point, *pe = point + numPoints; pb != pe; a = b, ++pb)
  {
    b = *pb;
    float distanceSquare = distance_point_to_line_segment_square(Point2::xz(fromPos), a, b);
    if (distanceSquare < minSquareDist)
      minSquareDist = distanceSquare;
  }
  if (fromPos.y > maxHeight)
  {
    float heightAboveCapzone = fromPos.y - maxHeight;
    resDistance = sqrtf(heightAboveCapzone * heightAboveCapzone + minSquareDist);
  }
  else if (fromPos.y < minHeight)
  {
    float heightBelowCapzone = minHeight - fromPos.y;
    resDistance = sqrtf(heightBelowCapzone * heightBelowCapzone + minSquareDist);
  }
  else
    resDistance = sqrtf(minSquareDist);
  return resDistance;
}

static bool calc_distance_to_capzone_border(ecs::EntityId targetCapzoneEid, const Point3 &fromPos, float &distOut)
{
  if (get_poly_capzone_ecs_query(targetCapzoneEid,
        [&](const ecs::Point2List &capzone__areaPoints, const float capzone__minHeight, const float capzone__maxHeight) {
          distOut = distance_to_polyCapzone(capzone__areaPoints.data(), capzone__areaPoints.size(), fromPos, capzone__minHeight,
            capzone__maxHeight);
        }))
    return true;

  if (get_box_capzone_ecs_query(targetCapzoneEid, [&](ECS_REQUIRE(ecs::Tag box_zone) const TMatrix &transform) {
        BBox3 box = transform * BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
        const size_t numPoints = 4;
        Point2 boxPoints[numPoints];
        Point3 &minPoint = box.boxMin();
        Point3 &maxPoint = box.boxMax();
        Point3 delta = maxPoint - minPoint;
        boxPoints[0] = Point2::xz(minPoint);
        boxPoints[1] = Point2(minPoint.x + delta.x, minPoint.z);
        boxPoints[2] = Point2::xz(maxPoint);
        boxPoints[3] = Point2(minPoint.x, minPoint.z + delta.z);
        float minHeight = minPoint.y;
        float maxHeight = maxPoint.y;

        distOut = distance_to_polyCapzone(boxPoints, numPoints, fromPos, minHeight, maxHeight);
      }))
    return true;

  if (get_sphere_capzone_ecs_query(targetCapzoneEid, [&](const TMatrix &transform, const float sphere_zone__radius) {
        distOut = length(transform.getcol(3) - fromPos) - sphere_zone__radius;
      }))
    return true;

  return false;
}

BhvDistToBorder::BhvDistToBorder() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvDistToBorder::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_dist_to_border_update);
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  bool wasHidden = elem->isHidden();

  Sqrat::Table &scriptDesc = elem->props.scriptDesc;

  Point3 fromPos = uishared::view_itm.getcol(3);
  ecs::EntityId fromEid = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->fromEid, ecs::INVALID_ENTITY_ID);
  if (auto fromTransform = g_entity_mgr->getNullable<TMatrix>(fromEid, ECS_HASH("transform")))
    fromPos = fromTransform->getcol(3);

  float dist = 0.0;
  ecs::EntityId targetEid = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->targetEid, ecs::INVALID_ENTITY_ID);
  if (targetEid == ecs::INVALID_ENTITY_ID || !uishared::has_valid_world_3d_view())
    elem->setHidden(true);
  else if (calc_distance_to_capzone_border(targetEid, fromPos, dist))
  {
    float minDistance = scriptDesc.RawGetSlotValue(strings->minDistance, -100);
    if (dist > minDistance)
    {
      if (fabsf(float(atof(elem->props.text)) - floorf(dist + 0.5f)) > 1e-3)
        discard_text_cache(elem->robjParams);
      elem->props.text.printf(8, "%.f", dist);
      elem->setHidden(false);
    }
    else
      elem->setHidden(true);
  }
  else
    elem->setHidden(true);

  return (elem->isHidden() != wasHidden) ? darg::R_REBUILD_RENDER_AND_INPUT_LISTS : 0;
}
