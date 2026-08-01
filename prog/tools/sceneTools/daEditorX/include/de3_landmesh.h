//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bounds3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <drv/3d/dag_resId.h>
#include <drv/3d/dag_samplerHandle.h>

struct EditorHeightmapInfo
{
  TEXTUREID mainTexId = BAD_TEXTUREID;
  d3d::SamplerHandle mainSampler = d3d::SamplerHandle::Invalid;
  IPoint2 mainTexSize = {0, 0};
  Point2 worldOffset = {0, 0};
  Point2 worldSize = {0, 0};
  float hMin = 0.f, hMax = 0.f;
  float cellSize = 1.f;
  TEXTUREID detTexId = BAD_TEXTUREID;
  d3d::SamplerHandle detSampler = d3d::SamplerHandle::Invalid;
  Point2 detOrigin = {0, 0};
  Point2 detSize = {0, 0};
};

class LandMeshManager;
class LandMeshRenderer;
class DynamicMemGeneralSaveCB;

class ILandmesh
{
public:
  static constexpr unsigned HUID = 0xB73CF653u; // ILandmesh

  virtual BBox3 getBBoxWithHMapWBBox() const = 0;
  virtual bool isLandmeshRenderingMode() const = 0;
  virtual LandMeshManager *getLandMeshManager() const = 0;
  virtual LandMeshRenderer *getLandMeshRenderer() const = 0;
  virtual bool getEditorHeightmapInfo(EditorHeightmapInfo *out) const = 0;
  virtual bool buildEditorHeightmap(DynamicMemGeneralSaveCB *cb) = 0;
};
