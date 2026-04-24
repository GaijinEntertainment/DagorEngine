//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <dag/dag_vector.h>

class ToroidalVoxelGrid
{
public:
  struct Settings
  {
    uint32_t xzDim = 128;
    uint32_t yDim = 32;
    uint32_t moveThreshold = 1; // in texels
    float voxelSize = 1.f;
    float heightOffsetRatio = 0.5f;         // 0 - lowest voxel = world.pos.y, 1 - highest voxel, 0.5 - in center
    bool useSmoothCascadeTransition = true; // eliminate toroidal movement "jumps" by sacrificing some border voxels

    bool operator==(const Settings &) const = default;
  };

  struct ShaderVars
  {
    IPoint4 texDim;
    IPoint4 origin;
    Point2 voxelSize;
    Point2 worldToTc;
    Point2 vignetteTcScale;
    Point3 originInTcSpace;
    Point3 wvpInTcSpaceClamped; // clapmed to origin +- moveThreshold / texDim
  };

  struct Region
  {
    IPoint3 offs;
    IPoint3 size;
  };

  enum class UpdateType : uint32_t
  {
    ALL_DIRECTIONS,
    ONE_DIRECTION,
  };

  enum class UpdateResult : uint32_t
  {
    NONE,
    FULL_UPDATE,
    PARTIAL_UPDATE,
  };

  ToroidalVoxelGrid();
  ToroidalVoxelGrid(const Settings &cfg);

  void invalidate();
  bool changeSettings(const Settings &cfg);
  UpdateResult updateOrigin(const Point3 &world_pos, UpdateType type);
  void updateWorldViewPos(const Point3 &world_pos); // only needed if Settings::useSmoothCascadeTransition=1

  const dag::Vector<Region> &getRegions() const { return regions; };
  const ShaderVars &getShaderVars() const { return vars; }
  const Settings &getSettings() const { return cfg; }

private:
  void calcVars();
  void calcTcVars();

  ShaderVars vars;
  IPoint3 origin;
  Point3 moveThresholdInTcSpace;
  Settings cfg;
  dag::Vector<Region> regions;
};