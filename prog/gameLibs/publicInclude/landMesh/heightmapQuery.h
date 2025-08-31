//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "gpuReadbackQuery/gpuReadbackResult.h"
#include <dag/dag_vector.h>
#include <math/dag_Point4.h>

class Point2;
class Point3;
struct HeightmapQueryResult;

// This is needed for dascript bindings
struct HeightmapQueryResultWrapper
{
  float hitDistNoOffset;
  float hitDistWithOffset;
  float hitDistWithOffsetDeform;
  Point3 normal;
};

struct LandclassQueryData
{
  dag::Vector<Point3> landclass_gravs;
  dag::Vector<int> indices;
  dag::Vector<Point4> pos_radiusSqr;
};

namespace heightmap_query
{
void init();
void close();
void update();

void update_landclass_data(LandclassQueryData &data);

int query(const Point3 &world_pos, const Point3 &grav_dir);
GpuReadbackResultState get_query_result(int query_id, HeightmapQueryResult &result);
GpuReadbackResultState get_query_result(int query_id, HeightmapQueryResultWrapper &wrapped_result);
} // namespace heightmap_query
