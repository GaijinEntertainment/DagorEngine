//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/vector.h>
#include <math/dag_Point3.h>
#include <3d/dag_resPtr.h>
#include <scene/dag_tiledScene.h>

class GIWindows
{
public:
  void init(eastl::unique_ptr<class scene::TiledScene> &&s);
  void validate();
  bool isValid() const { return currentCount == activeList.size(); }
  void invalidate() { centerPos += Point3(10000, -1000, 10000); }
  void updatePos(const Point3 &pos);
  eastl::unique_ptr<class scene::TiledScene> windows;
  Point3 centerPos = {-10000, 1000, -10000};
  float dist = 128.f;
  int bufferCount = 0, currentCount = -1, currentGridSize = 0, currentIndSize = 0;
  eastl::vector<mat43f> activeList;
  eastl::vector<bbox3f> activeListBox;
  UniqueBufHolder currentWindowsSB, currentWindowsSBInd, currentWindowsGridSB;
  UniqueBuf gridCntSB;
  eastl::unique_ptr<ComputeShaderElement> fill_windows_grid_range, clear_windows_grid_range, fill_windows_grid_range_fast;
  bool calc();
  ~GIWindows();
};