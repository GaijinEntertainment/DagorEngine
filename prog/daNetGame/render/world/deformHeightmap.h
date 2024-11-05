// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>

#include <math/dag_Point2.h>
#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_decl.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStateId.h>

class Point3;
class ComputeShaderElement;

struct DeformHeightmapDesc
{
  int texSize = 1024;
  float rectSize = 60.f;
  bool raiseEdges = false;
  float minTerrainHeight = 0.f;
  float maxTerrainHeight = 100.f;
  float fpsCamDist = 0.08f;
  const char *paramsDataBlockName = "gamedata/deform_heightmap_params.blk";
};

class DeformHeightmap
{
public:
  DeformHeightmap(const DeformHeightmapDesc &desc);
  ~DeformHeightmap();

  DeformHeightmap(const DeformHeightmap &) = delete;
  DeformHeightmap(DeformHeightmap &&other) = delete;
  DeformHeightmap &operator=(const DeformHeightmap &) = delete;
  DeformHeightmap &operator=(DeformHeightmap &&other) = delete;

  bool isValid() const;
  bool isEnabled() const;
  void requestClear() { needClearDepth = true; }
  inline TMatrix getInverseViewTm() const { return iViewTm; }
  Point4 getDeformRect() const;
  void beforeRenderWorld(const Point3 &cam_pos);
  void beforeRenderDepth();
  void afterRenderDepth();

private:
  const int texSize;
  const int maskTexSize;
  const int numThreadGroupsOnTexXY;
  const int numThreadGroupsOnMaskXY;
  const float boxSize;
  const float zn;
  const float zf;
  const bool raiseEdges;
  const float fpsCamDist;
  TMatrix iViewTm, viewTm;
  TMatrix4 projTm;
  eastl::array<UniqueTex, 2> depthTextures;
  eastl::array<UniqueTex, 2> postFxTextures;
  eastl::array<UniqueTex, 2> maskGridTextures;
  eastl::array<UniqueTex, 2> deformInfoTextures;
  UniqueBuf postFxCellBuffer, clearCellBuffer;
  UniqueBuf indirectBuffer;
  UniqueBufHolder hmapDeformParamsBuffer;
  eastl::unique_ptr<PostFxRenderer> reprojectPostFx;
  eastl::unique_ptr<ComputeShaderElement> clearMaskCs, deformCs, clearIndirect, dispatcherCs, edgeDetectCs, blurCs, clearerCs;
  shaders::UniqueOverrideStateId zFuncAlwaysStateId;

  bool needClearDepth;
  int prevRtIdx, currentRtIdx;
  int prevMaskIdx, currentMaskIdx;
  int postFxSrcIdx, postFxDstIdx;
  Point2 currentCamPosXZ, prevCamPosXZ;
  Driver3dRenderTarget origRt;
  ViewProjMatrixContainer origViewProj;

  void clear();
  void saveStates();
  void restoreStates();

  bool isCameraInFpsView() const;
};