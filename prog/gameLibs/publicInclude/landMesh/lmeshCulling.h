//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// #include <stdlib.h>//for abs


#include <heightmap/heightmapCulling.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_bounds3.h>
#if DAGOR_DBGLEVEL > 0
#include <math/dag_TMatrix4.h>
#endif

class LandMeshManager;


#define LANDMESH_MAX_CELLS    (64 * 3 * 4 + 4 * 4 + 1) //(maxRadius*(maxmirroring=3)*(max_sides=4)+max_sides*MAX_LAND_MESH_REGIONS+1
#define LANDMESH_INVALID_CELL LANDMESH_MAX_CELLS

struct LandMeshCellDesc
{
  uint16_t next;
  int8_t borderX;
  int8_t borderY;
  uint8_t countX;
  uint8_t countY;
};

static constexpr int MAX_LAND_MESH_REGIONS = 4;

struct LandMeshCullingData
{
  LandMeshCullingData(IMemAlloc *mem = midmem) : heightmapData(mem) {}
  LandMeshCellDesc cells[LANDMESH_MAX_CELLS];
  int count = 0;
  struct Region
  {
    short head, tail;
  } regions[MAX_LAND_MESH_REGIONS];
  int regionsCount;
  LodGridCullData heightmapData;
#if DAGOR_DBGLEVEL > 0
  TMatrix4 culltm;
#endif
};

class LandMeshRenderer;
class Occlusion;
struct LandMeshCullingState
{

  /*
    static __forceinline int mirror_x(int x, int x0, int x1)//allows to mirror more then once
    {
      x -= x0;
      int w = x1-x0+1;
      x += (x<0 ? 1 : 0);
      int x_div_w = x/w;
      int abs_xmodw = abs(x%w);
      return x0 + ((x_div_w & 1) ? (w-1 - abs_xmodw) : abs_xmodw);
    }*/
  static __forceinline int mirror_x(int x, int x0, int x1) { return x < x0 ? 2 * x0 - 1 - x : (x > x1 ? 2 * x1 + 1 - x : x); }
  // LandMeshManagerAces states:
  BBox3 *cellBoundings;
  int cellBoundingsCount;
  float *cellBoundingsRadius;
  int cellBoundingsRadiusCount;

  IPoint2 origin;
  int mapSizeX, mapSizeY;

  real landCellSize;
  real gridCellSize;
  Point3 landMeshOffset;
  IBBox2 regions[MAX_LAND_MESH_REGIONS]; // every cell, that is in earliest region will be there. All cells, that not falling into any
                                         // region will be in last one

  // LandMeshRendererAces states:
  int visRange;
  IPoint2 centerCell;

  int numBorderCellsXPos;
  int numBorderCellsXNeg;
  int numBorderCellsZPos;
  int numBorderCellsZNeg;

  BBox3 renderInBBox;
  IBBox2 exclBox;
  bool useExclBox;

  // Culling functionality:
  BBox3 getBBox(int x, int y, float *sphere_radius = NULL);

  bool calcCellBox(LandMeshManager &provider, int borderX, int borderY, int x0, int y0, int x1, int y1, BBox3 &res);
  void cullDataWithBbox(LandMeshCullingData &dest_data, const LandMeshCullingData &src_data, const BBox2 &bbox);

  void cullCell(LandMeshManager &provider, int borderX, int borderY, int x0, int y0, int x1, int y1, const Frustum &frustum,
    const Occlusion *occlusion, LandMeshCullingData &data);

  void frustumCulling(LandMeshManager &provider, const Frustum &globtm, const Occlusion *occlusion, LandMeshCullingData &data,
    const IBBox2 *regions, int regions_count, const Point3 &hmap_origin, float hmap_camera_height, int hmapTankDetail,
    int hmap_sub_div = 0, float hmap_lod0_scale = 1.0f);


  void copyLandmeshState(LandMeshManager &provider, LandMeshRenderer &renderer);


  // Culling modes:
  enum CullMode
  {
    ASYNC_CULLING,
    SYNC_CULLING,
    NO_CULLING,
  };

  CullMode cullMode;

  LandMeshCullingState() { memset(this, 0, sizeof(*this)); }
};
