//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "ssao_base.h"
#include "ssao_common.h"
#include <3d/dag_resizableTex.h>
#include <3d/dag_drv3dConsts.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <render/viewDependentResource.h>

// Following class implements "Ground Truth Ambient Occlusion" algorithm
// Easiest way to understand the concept of it is to read
// "Practical Realtime Strategies for Accurate Indirect Occlusion" paper
class GTAORenderer : public ISSAORenderer
{
public:
  using ISSAORenderer::render;

  GTAORenderer() = delete;
  GTAORenderer(int w, int h, int views, uint32_t flags = SSAO_NONE, bool use_own_textures = true);
  virtual ~GTAORenderer();

  void reset() override;
  void changeResolution(int w, int h) override;

  Texture *getSSAOTex() override;
  TEXTUREID getSSAOTexId() override;

  void setCurrentView(int viewIndex);

private:
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssaoDepthTexUse, const ManagedTex *ssaoTex,
    const ManagedTex *prevSsaoTex, const ManagedTex *tmpTex, const DPoint3 *world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single) override;

  void updateCurFrameIdxs();
  void renderGTAO(const ManagedTex &rawTex);
  void applySpatialFilter(const ManagedTex &rawTex, const ManagedTex &spatialTex);
  void applyTemporalFilter(const ManagedTex &rawTex, const ManagedTex &prevGtaoTex, const ManagedTex &spatialTex);

  static constexpr const char *gtao_sh_name = "gtao_main";
  static constexpr const char *spatial_filter_sh_name = "gtao_spatial";
  static constexpr const char *temporal_filter_sh_name = "gtao_temporal";

  bool even;
  uint32_t rawTargetIdx;
  uint32_t spatialTargetIdx;
  uint32_t temporalTargetIdx;
  uint32_t historyIdx; // last frame's temporalTargetIdx

  ViewDependentResource<ResizableTex, 2, 3> gtaoTex;
  eastl::unique_ptr<PostFxRenderer> spatialFilterRenderer, temporalFilterRenderer;

  eastl::unique_ptr<ComputeShaderElement> spatialFilterRendererCS, temporalFilterRendererCS;

  struct Afr
  {
    TMatrix4 prevGlobTm;
    Point4 prevViewVecLT;
    Point4 prevViewVecRT;
    Point4 prevViewVecLB;
    Point4 prevViewVecRB;
    DPoint3 prevWorldPos;
    uint32_t sampleNo;
  };

  ViewDependentResource<Afr, 2> afrs;
  bool useOwnTextures;
};
