#pragma once

#include "hmlSplinePoint.h"
#include <de3_splineClassData.h>


inline splineclass::RoadData *getRoad(SplinePointObject *p, bool for_geom = true)
{
  if (!p->getSplineClass() || !p->getSplineClass()->road)
    return NULL;

  splineclass::RoadData *r = p->getSplineClass()->road;

  if (for_geom)
    return r->buildGeom ? r : NULL;

  return r->buildRoadMap ? r : NULL;
}
