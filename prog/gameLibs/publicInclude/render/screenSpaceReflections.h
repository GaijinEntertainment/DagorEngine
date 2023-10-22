//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_resourcePool.h>
#include <render/viewDependentResource.h>
#include <render/subFrameSample.h>
#include <generic/dag_carray.h>

#include <EASTL/array.h>

enum class SSRQuality
{
  Low,
  Medium,
  High,
  Compute,
  ComputeHQ
};
class ComputeShaderElement;

class ScreenSpaceReflections
{
protected:
  ViewDependentResource<UniqueTex, 2, 1> ssrTarget;
  ViewDependentResource<UniqueTex, 2, 1> ssrPrevTarget;
  bool hasPrevTarget = false;
  RTargetPool::Ptr rtTemp, rtTempLow, rtTempR16F;

  struct Afr
  {
    int frameNo;
    TMatrix4 prevGlobTm;
    Point4 prevViewVecLT;
    Point4 prevViewVecRT;
    Point4 prevViewVecLB;
    Point4 prevViewVecRB;
    DPoint3 prevWorldPos;
  };

  ViewDependentResource<Afr, 2, 1> afrs;
  PostFxRenderer ssrQuad;
  PostFxRenderer ssrResolve;
  PostFxRenderer ssrMipChain;
  PostFxRenderer ssrVTR;
  PostFxRenderer ssrComposite;
  eastl::unique_ptr<ComputeShaderElement> ssrCompute;
  RTargetPool::Ptr ssrVTRRT;

  //
  eastl::unique_ptr<ComputeShaderElement> ssrClassify, ssrPrepareIndirectArgs, ssrIntersect, ssrReproject, ssrPrefilter, ssrTemporal;

  UniqueBufHolder rayListBuf, tileListBuf, countersBuf;
  UniqueBuf indirectArgsBuf;
  UniqueTex varianceTex[2], sampleCountTex[2];

  int ssrW, ssrH;
  uint32_t ssrflags;
  int randomizeOverNFrames;
  SSRQuality quality;
  bool ownTextures;

public:
  ScreenSpaceReflections(int ssr_w, int ssr_h, int num_views = 1, uint32_t fmt = 0, SSRQuality ssr_quality = SSRQuality::Low,
    bool own_textures = true);
  ~ScreenSpaceReflections();

  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos, SubFrameSample sub_sample,
    ManagedTexView ssrTex, ManagedTexView ssrPrevTex, ManagedTexView targetTex, const dag::ConstSpan<ManagedTexView> &tempTextures,
    int callId);
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single);
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    DPoint3 worldPos(0, 0, 0);
    render(view_tm, proj_tm, worldPos, SubFrameSample::Single);
  }
  void setRandomizationFrames(int v) { randomizeOverNFrames = v; }
  void setCurrentView(int index);
  int getRandomizationFrames() const { return randomizeOverNFrames; }
  void enablePrevTarget(int num_views);
  void disablePrevTarget();
  void updatePrevTarget();

  static void getRealQualityAndFmt(uint32_t &fmt, SSRQuality &ssr_quality);
  static int getMipCount(SSRQuality ssr_quality);
};
