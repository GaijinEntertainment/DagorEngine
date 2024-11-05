// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_bounds3.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <ioSys/dag_dataBlock.h>
#include <render/toroidalHelper.h>


class LandMeshManager;
class RenderScene;

class PuddlesManager
{
public:
  PuddlesManager() = default;

  void init(const LandMeshManager *lmesh_mgr, const DataBlock &puddles_settings, int forced_max_resolution = -1);
  void reinit_same_settings(const LandMeshManager *lmesh_mgr, int forced_max_resolution = -1);
  void close();

  void puddlesAfterDeviceReset();

  void setPuddlesScene(RenderScene *scn);
  void preparePuddles(const Point3 &at);

  void invalidatePuddles(bool force);
  void removePuddlesInCrater(const Point3 &pos, float radius);

private:
  const LandMeshManager *lmeshMgr = nullptr;

  ToroidalHelper puddlesHelper;
  UniqueTexHolder puddles;
  bool removedPuddlesNeedUpdate = false;
  uint32_t removedPuddlesIndexToAdd = 0;
  uint32_t removedPuddlesActualSize = 0;
  dag::Vector<Point4> puddlesRemoved;
  UniqueBufHolder puddlesRemovedBuf;
  PostFxRenderer puddlesRenderer;
  DynamicShaderHelper puddlesRemover;
  float puddlesDist = 512;
  int puddleLod = 1;
  DataBlock puddlesSettings;

  shaders::UniqueOverrideStateId blendMaxState;

  RenderScene *puddlesScene = 0;
  BBox3 puddlesSceneBbox;
};
