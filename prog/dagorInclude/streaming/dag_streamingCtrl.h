//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>

// forward declarations of external classes
class IStreamingSceneManager;
class DataBlock;


//
// Controller for streaming scenes
//
class StreamingSceneController
{
public:
  StreamingSceneController(IStreamingSceneManager &_mgr, const DataBlock &blk, const char *folder_path);
  ~StreamingSceneController();

  void preloadAtPos(const Point3 &p, real overlap_rad = 0);
  void setObserverPos(const Point3 &p);
  void renderDbg();

  float getBinDumpOptima(unsigned bindump_id);

protected:
  IStreamingSceneManager &mgr;
  Point3 curObserverPos;

  NameMap sceneBin;

  struct ActionSphere
  {
    Point3 center;
    real rad, loadRad2, unloadRad2;
    int sceneBinId;
    int bindumpId;
  };
  Tab<ActionSphere> actionSph;
};
