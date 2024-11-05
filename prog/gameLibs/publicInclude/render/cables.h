//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_hlsl_floatx.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_resPtr.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_set.h>
#include <ioSys/dag_genIo.h>
#include "cables.hlsli"

class Point3;

class Cables
{
public:
  enum
  {
    RENDER_PASS_SHADOW = 0,
    RENDER_PASS_OPAQUE = 1,
    RENDER_PASS_TRANS = 2,
    RENDER_PASS_BVH = 3
  };

  Cables();

  Cables(const Cables &) = delete;
  Cables &operator=(const Cables &) = delete;
  Cables(Cables &&) = delete;
  Cables &operator=(Cables &&) = delete;

  void loadCables(IGenLoad &crd);
  void setMaxCables(unsigned int max_cables);

  int addCable(const Point3 &p1, const Point3 &p2, float rad, float length);
  void setCable(int id, const Point3 &p1, const Point3 &p2, float rad, float length);

  bool beforeRender();                  // Use this function if you set pixel scale to shaders in another place
  bool beforeRender(float pixel_scale); // Use this function if there are no pixel scale variable in project
  void render(int render_pass);
  void afterReset();

  void gatherRIExtra(bool inside_rendinst_loading_block = false);
  void onRIExtraDestroyed(const TMatrix &tm, const BBox3 &box);
  void destroyCables();

  Tab<CableData> *getCablesData() { return &cables; };
  eastl::unordered_set<uint32_t> &getDirtyCables() { return dirtyCables; };

  uint32_t getMaxCables() const { return maxCables; }
  uint32_t getTranglesPerCable() const;

protected:
  struct CablePointInfo
  {
    uint16_t cableIndex : 15;
    uint16_t cableEnd : 1;
  };
  struct TiledArea
  {
    struct Tile
    {
      uint16_t start = 0;
      uint16_t count = 0;
    };
    eastl::vector<Tile> grid;
    eastl::vector<CablePointInfo> cables_points;
    IPoint2 gridSize;
    Point2 tileSize;
    Point2 gridBoundMin;
    Tile &getTile(int x, int y);
  };
  void generateTiles();

  TiledArea tiledArea;
  Tab<CableData> cables;
  eastl::unordered_set<uint32_t> dirtyCables;
  DynamicShaderHelper cablesRenderer;
  UniqueBufHolder cablesBuf;
  unsigned int maxCables = 0;
  bool changed = false;
  struct RIExtraInfo
  {
    TMatrix tm;
    BBox3 box;
  };
  Tab<RIExtraInfo> destroyedRIExtra;
};

Cables *get_cables_mgr();
void init_cables_mgr();
void close_cables_mgr();
