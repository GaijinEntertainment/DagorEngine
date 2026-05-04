//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "ssao_base.h"
#include "ssao_common.h"
#include <3d/dag_resizableTex.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <render/viewDependentResource.h>
#include <shaders/dag_dynamicResolutionStcode.h>

// Following class implements "Ground Truth Ambient Occlusion" algorithm
// Easiest way to understand the concept of it is to read
// "Practical Realtime Strategies for Accurate Indirect Occlusion" paper
class GTAORenderer : public ISSAORenderer
{
public:
  using ISSAORenderer::render;

  GTAORenderer() = delete;
  GTAORenderer(int w, int h, int views, uint32_t flags = SSAO_NONE, bool use_own_textures = true,
    bool use_own_reprojection_params = true);
  virtual ~GTAORenderer();

  void reset() override;
  void changeResolution(int w, int h) override;

  Texture *getSSAOTex() override;
  TEXTUREID getSSAOTexId() override;

  void setCurrentView(int viewIndex);

  void renderGTAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssao_tex, const DPoint3 *world_pos,
    const DynRes *dynamic_resolution = nullptr);
  void applyGTAOFilter(BaseTexture *ssao_tex, BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex,
    SubFrameSample sub_sample = SubFrameSample::Single, const DynRes *dynamic_resolution = nullptr);

private:
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssaoDepthTexUse, BaseTexture *ssaoTex,
    BaseTexture *prevSsaoTex, BaseTexture *tmpTex, const DPoint3 *world_pos, SubFrameSample sub_sample = SubFrameSample::Single,
    const DynRes *dynamic_resolution = nullptr) override;

  void updateCurFrameIdxs();
  void renderGTAO(BaseTexture *rawTex, const DynRes *dynamic_resolution);
  void applySpatialFilter(BaseTexture *rawTex, BaseTexture *spatialTex, const DynRes *dynamic_resolution);
  void applyTemporalFilter(BaseTexture *rawTex, BaseTexture *prevGtaoTex, BaseTexture *spatialTex, const DynRes *dynamic_resolution);

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
    TMatrix4 prevGlobTm, prevProjTm;
    Point4 prevViewVecLT;
    Point4 prevViewVecRT;
    Point4 prevViewVecLB;
    Point4 prevViewVecRB;
    DPoint3 prevWorldPos;
    uint32_t sampleNo;
  };

  ViewDependentResource<Afr, 2> afrs;
  bool useOwnTextures;
  bool useOwnReprojectionParams;

  eastl::optional<DynRes> prevDynRes;
};
