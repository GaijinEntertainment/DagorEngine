//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/floatingVolume.h>
#include <daECS/core/componentType.h>

namespace rendinstfloating
{
enum InteractionType
{
  NO_INTERACTION,
  FREE_FLOAT,
  ELASTIC_STRING
};

struct RiFloatingPhys
{
  int riInstId = -1;
  Point3 omega;
  Point3 velocity;
  Point3 scale;
  gamephys::Loc staticLoc;
  gamephys::Loc location;
  gamephys::Loc prevLoc;
  gamephys::Loc visualLoc;
  gamephys::Loc prevVisualLoc;
  Point3 bboxHalfSize;
  Point2 xDir;
  int curTick = 0;
};

struct PhysFloatingModel
{
  int resIdx = -1;
  int processedRiTmCount = 0;
  float randMassMin = 0.0f;
  float randMassMax = 0.0f;
  float spheresRad = 0.0f;
  Point3 invMomentOfInertiaCoeff;
  BBox3 physBbox;
  dag::Vector<Point3> spheresCoords; // Coordinates inside phys bbox in range [-1, 1].
  eastl::vector<rendinstfloating::RiFloatingPhys> instances;
};

void init_floating_ri_res_groups(const DataBlock *ri_config, bool use_subblock_name = false);
}; // namespace rendinstfloating

ECS_DECLARE_RELOCATABLE_TYPE(rendinstfloating::PhysFloatingModel);
