//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_resourcePool.h>
#include <render/viewDependentResource.h>
#include <render/subFrameSample.h>
#include <generic/dag_carray.h>
#include <util/dag_bitFlagsMask.h>

#include <EASTL/array.h>

enum class SSRQuality
{
  Low,
  Medium,
  Compute,
};
class ComputeShaderElement;

enum class SSRFlag
{
  None = 0,
  ExternalDenoiser = 1 << 0,
  CreateTextures = 1 << 1,
};

using SSRFlags = BitFlagsMask<SSRFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(SSRFlag);

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
    TMatrix4 prevGlobTm, prevProjTm;
    Point4 prevViewVecLT;
    Point4 prevViewVecRT;
    Point4 prevViewVecLB;
    Point4 prevViewVecRB;
    DPoint3 prevWorldPos;
  };

  ViewDependentResource<Afr, 2, 1> afrs;
  PostFxRenderer ssrQuad, ssrMipChain;
  eastl::unique_ptr<ComputeShaderElement> ssrCompute;
  RTargetPool::Ptr ssrVTRRT;

  int ssrW, ssrH;
  uint32_t ssrflags;
  int randomizeOverNFrames;
  SSRQuality quality;
  bool ownTextures;
  bool denoiser;

  void updateSamplers() const;

public:
  ScreenSpaceReflections(int ssr_w, int ssr_h, int num_views = 1, uint32_t fmt = 0, SSRQuality ssr_quality = SSRQuality::Low,
    SSRFlags flags = SSRFlag::CreateTextures);
  ~ScreenSpaceReflections();

  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos, SubFrameSample sub_sample,
    ManagedTexView ssrTex, ManagedTexView ssrPrevTex, ManagedTexView targetTex, int callId);
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single);
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    DPoint3 worldPos = dpoint3(orthonormalized_inverse(view_tm).getcol(3));
    render(view_tm, proj_tm, worldPos, SubFrameSample::Single);
  }
  void setRandomizationFrames(int v) { randomizeOverNFrames = v; }
  void setCurrentView(int index);
  int getRandomizationFrames() const { return randomizeOverNFrames; }
  void enablePrevTarget(int num_views);
  void disablePrevTarget();
  void updatePrevTarget();

  SSRQuality getQuality() const { return quality; }
  void changeDynamicResolution(int new_width, int new_height);

  static void getRealQualityAndFmt(uint32_t &fmt, SSRQuality &ssr_quality);
  static int getMipCount(SSRQuality ssr_quality);
};
