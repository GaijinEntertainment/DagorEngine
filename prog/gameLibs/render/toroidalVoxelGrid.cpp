// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/toroidalVoxelGrid.h>
#include <math/random/dag_random.h>
#include <math/integer/dag_IBBox3.h>
#include "shaders/tor_voxel_grid_inc.hlsli"

static const IPoint3 g_invalid_origin = {0xffffff, 0xffffff, 0xffffff};

ToroidalVoxelGrid::ToroidalVoxelGrid() : origin(g_invalid_origin) {}

ToroidalVoxelGrid::ToroidalVoxelGrid(const Settings &c) : cfg(c), origin(g_invalid_origin) { calcVars(); }

void ToroidalVoxelGrid::invalidate() { origin = g_invalid_origin; }

bool ToroidalVoxelGrid::changeSettings(const Settings &c)
{
  if (cfg == c)
    return false;

  cfg = c;
  calcVars();
  invalidate();
  return true;
}

void ToroidalVoxelGrid::calcVars()
{
  G_ASSERT(cfg.voxelSize > 0);

  int hx = cfg.xzDim / 2;
  int hy = cfg.yDim / 2;

  vars.texDim = IPoint4(cfg.xzDim, cfg.yDim, hx, hy);
  vars.voxelSize = Point2(cfg.voxelSize, 1.f / cfg.voxelSize);
  vars.worldToTc = Point2(1.f / (cfg.voxelSize * cfg.xzDim), 1.f / (cfg.voxelSize * cfg.yDim));
  // +1 to reduce border from each side, so we wont trilinear sample outside for toroidal borders
  // *2 to rescale -0.5..0.5 rel tc to -1..1
  // + extra padding (opt) for smooth cascade transtion

  int pad = cfg.useSmoothCascadeTransition ? 1 + cfg.moveThreshold : 1;

  Point2 rad = {cfg.xzDim * 0.5f, cfg.yDim * 0.5f};
  vars.vignetteTcScale.x = 2.f / ((rad.x - pad) / rad.x);
  vars.vignetteTcScale.y = 2.f / ((rad.y - pad) / rad.y);
  vars.wvpInTcSpaceClamped = {0, 0, 0};

  moveThresholdInTcSpace = {(float)cfg.moveThreshold, (float)cfg.moveThreshold, (float)cfg.moveThreshold};
  moveThresholdInTcSpace.x *= 1.f / cfg.xzDim;
  moveThresholdInTcSpace.y *= 1.f / cfg.xzDim;
  moveThresholdInTcSpace.z *= 1.f / cfg.yDim;
}

void ToroidalVoxelGrid::calcTcVars()
{
  vars.origin = IPoint4(origin.x, origin.y, origin.z, 0);
  vars.originInTcSpace = Point3(origin.x, origin.z, origin.y) * cfg.voxelSize;
  vars.originInTcSpace.x *= vars.worldToTc.x;
  vars.originInTcSpace.y *= vars.worldToTc.x;
  vars.originInTcSpace.z *= vars.worldToTc.y;

  if (!cfg.useSmoothCascadeTransition)
    vars.wvpInTcSpaceClamped = vars.originInTcSpace;
}

inline bool origin_limit_check(int v)
{
  constexpr uint32_t limit = (1u << 31) - TOR_VGRID_WRAP_OFFSET;
  return v >= -(int)TOR_VGRID_WRAP_OFFSET && v <= (int)limit; // so we can use cheap % in tor_vgrid_world_voxel_to_tex_tci
}

inline void push_toroidal_reg(int move, int attr, const IBBox3 &nb, const IBBox3 &pb, dag::Vector<ToroidalVoxelGrid::Region> &regions)
{
  if (move == 0)
    return;

  IBBox3 b = nb;
  if (move > 0)
    b.lim[0][attr] = pb.lim[1][attr];
  else
    b.lim[1][attr] = pb.lim[0][attr];

  regions.push_back({b.lim[0], b.width()});
}

static float calc_height_offset(const ToroidalVoxelGrid::Settings &cfg) { return cfg.yDim * 0.5f - cfg.heightOffsetRatio * cfg.yDim; }

ToroidalVoxelGrid::UpdateResult ToroidalVoxelGrid::updateOrigin(const Point3 &world_pos, UpdateType type)
{
  G_ASSERT(cfg.voxelSize > 0);
  IPoint3 newOrig = ipoint3(world_pos / cfg.voxelSize);

  if (!origin_limit_check(newOrig.x) || !origin_limit_check(newOrig.y) || !origin_limit_check(newOrig.z))
  {
    logerr("ToroidalVoxelGrid: origin outside of the possible range");
    return UpdateResult::NONE;
  }

  newOrig.y = newOrig.y + calc_height_offset(cfg);

  regions.clear();

  if (newOrig == origin)
    return UpdateResult::NONE;

  IPoint3 move = newOrig - origin;
  IPoint3 am = {abs(move.x), abs(move.y), abs(move.z)};
  if (max(am.x, am.z) >= cfg.xzDim || am.y > cfg.yDim)
  {
    origin = newOrig;
    calcTcVars();
    return UpdateResult::FULL_UPDATE;
  }

  if (max(max(am.x, am.y), am.z) < cfg.moveThreshold)
    return UpdateResult::NONE;

  if (type == UpdateType::ONE_DIRECTION)
  {
    if (am.x >= am.y && am.x >= am.z)
    {
      move.y = 0;
      move.z = 0;
    }
    else if (am.y >= am.x && am.y >= am.z)
    {
      move.x = 0;
      move.z = 0;
    }
    else
    {
      G_ASSERT(am.z >= am.x && am.z >= am.y);
      move.x = 0;
      move.y = 0;
    }
  }

  IPoint3 halfOfs = {int(cfg.xzDim) / 2, int(cfg.yDim) / 2, int(cfg.xzDim) / 2};
  IBBox3 prevWorldBox;
  prevWorldBox.lim[0] = origin - halfOfs;
  prevWorldBox.lim[1] = origin + halfOfs;

  IBBox3 newWorldBox = prevWorldBox;
  newWorldBox.lim[0] += move;
  newWorldBox.lim[1] += move;

  IBBox3 clipBox = newWorldBox;
  prevWorldBox.clipBox(clipBox);

  origin += move;
  calcTcVars();

  push_toroidal_reg(move.x, 0, newWorldBox, prevWorldBox, regions);
  push_toroidal_reg(move.y, 1, newWorldBox, prevWorldBox, regions);
  push_toroidal_reg(move.z, 2, newWorldBox, prevWorldBox, regions);

  int updatedVolume = 0;
  for (const Region &r : regions)
  {
    G_ASSERT(r.size.x * r.size.y * r.size.z > 0);
    updatedVolume += r.size.x * r.size.y * r.size.z;
  }

  int maxVolume = cfg.xzDim * cfg.xzDim * cfg.yDim;
  if (updatedVolume >= maxVolume)
  {
    regions.clear();
    return UpdateResult::FULL_UPDATE;
  }

  return UpdateResult::PARTIAL_UPDATE;
}

void ToroidalVoxelGrid::updateWorldViewPos(const Point3 &wvp)
{
  Point3 &w = vars.wvpInTcSpaceClamped;
  if (!cfg.useSmoothCascadeTransition)
  {
    w = vars.originInTcSpace;
    return;
  }

  const Point3 &b = moveThresholdInTcSpace;
  const Point3 &o = vars.originInTcSpace;

  w = {wvp.x, wvp.z, wvp.y + calc_height_offset(cfg) * cfg.voxelSize};
  w.x *= vars.worldToTc.x;
  w.y *= vars.worldToTc.x;
  w.z *= vars.worldToTc.y;

  w.x = clamp(w.x, o.x - b.x, o.x + b.x);
  w.y = clamp(w.y, o.y - b.y, o.y + b.y);
  w.z = clamp(w.z, o.z - b.z, o.z + b.z);
}