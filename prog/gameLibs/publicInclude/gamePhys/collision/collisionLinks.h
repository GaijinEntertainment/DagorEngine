//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/dag_TMatrix.h>
#include <memory/dag_framemem.h>

class DataBlock;

namespace dacoll
{

struct CollisionLinkData
{
  Point3 axis;
  Point3 offset;
  float capsuleRadiusScale;
  float oriParamMult;
  int16_t nameId;
  bool haveCollision;

  CollisionLinkData() :
    axis(1.f, 0.f, 0.f), offset(0.f, 0.f, 0.f), capsuleRadiusScale(1.0f), haveCollision(false), nameId(-1), oriParamMult(0.f)
  {}
};

using CollisionLinks = dag::RelocatableFixedVector<CollisionLinkData, 2>;

struct CollisionCapsuleProperties
{
  TMatrix tm;
  Point3 scale;
  int16_t nameId;
  bool haveCollision;
};

typedef dag::RelocatableFixedVector<CollisionCapsuleProperties, 2, true, framemem_allocator> tmp_collisions_t;

void load_collision_links(CollisionLinks &links, const DataBlock &blk, float scale);
void generate_collisions(const TMatrix &tm, const Point2 &ori_param, const CollisionLinks &links, tmp_collisions_t &out_collisions);
void lerp_collisions(tmp_collisions_t &a_collisions, const tmp_collisions_t &b_collisions, float factor);
int get_link_name_id(const char *name);
}; // namespace dacoll
