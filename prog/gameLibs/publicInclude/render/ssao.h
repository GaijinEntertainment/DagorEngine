//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "ssao_base.h"
#include "ssao_common.h"
#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <resourcePool/resourcePool.h>
#include <drv/3d/dag_consts.h>
#include <render/viewDependentResource.h>
#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <EASTL/utility.h>
#include <shaders/dag_dynamicResolutionStcode.h>

// Following class implements an algorithm
// from "Bent Normals and Cones in Screen-space" paper
class SSAORenderer : public ISSAORenderer
{
public:
  using ISSAORenderer::render;

  SSAORenderer() = delete;
  SSAORenderer(int w, int h, int num_views, uint32_t flags = SSAO_NONE, bool use_own_textures = true, const char *tag = "",
    bool use_own_reprojection_params = true);
  virtual ~SSAORenderer();

  void reset() override;

  Texture *getSSAOTex() override;
  TEXTUREID getSSAOTexId() override;

  void setCurrentView(int view) override;

  void renderSSAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_tex_to_use, BaseTexture *ssao_tex,
    BaseTexture *prev_ssao_tex, const DPoint3 *world_pos, SubFrameSample sub_sample, bool clear_rt,
    const DynRes *dynamic_resolution = nullptr);

  void applySSAOBlur(BaseTexture *ssao_tex, BaseTexture *tmp_tex, const DynRes *dynamic_resolution = nullptr);

private:
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssaoDepthTexUse, BaseTexture *ssaoTex,
    BaseTexture *prevSsaoTex, BaseTexture *tmpTex, const DPoint3 *world_pos, SubFrameSample sub_sample = SubFrameSample::Single,
    const DynRes *dynamic_resolution = nullptr) override;

  void initRandomPattern();
  void renderRandomPattern();

  void updateViewSpecific(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos);
  void updateFrameNo();
  void renderSSAO(BaseTexture *depth_to_use, BaseTexture *ssaoTex, BaseTexture *prevSsaoTex, bool clear_rt,
    const DynRes *dynamic_resolution);
  void applyBlur(BaseTexture *ssaoTex, BaseTexture *tmpTex, const DynRes *dynamic_resolution);

  void generatePoissonPoints(int num_samples, int num_frames);

  void clearRandomPattern() { randomPatternTex.close(); };

  static constexpr const char *ssao_sh_name = "ssao";
  static constexpr const char *blur_sh_name = "ssao_blur";

  struct ViewSpecific
  {
    TMatrix4 prevGlobTm, prevProjTm;
    Point4 prevViewVecLT;
    Point4 prevViewVecRT;
    Point4 prevViewVecLB;
    Point4 prevViewVecRB;
    DPoint3 prevWorldPos;
    unsigned int frameNo = 0;
    RTarget::Ptr ssaoTex;
  };

  RTargetPool::Ptr ssaoRTPool;
  ViewDependentResource<ViewSpecific, 2> viewSpecific;
  UniqueTexHolder randomPatternTex;
  UniqueBufHolder poissonPoints;

  eastl::unique_ptr<PostFxRenderer> ssaoBlurRenderer{nullptr};
  bool useOwnTextures;
  bool useOwnReprojectionParams;

  String tag;

  eastl::optional<DynRes> prevDynRes;
};
