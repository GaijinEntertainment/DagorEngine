// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ssao_base.h"
#include "ssao_common_public.h"
#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <resourcePool/resourcePool.h>
#include <drv/3d/dag_consts.h>
#include <render/viewDependentResource.h>
#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <EASTL/utility.h>
#include <EASTL/unique_ptr.h>

// Following code implements an algorithm
// from "Bent Normals and Cones in Screen-space" paper
namespace ssao_pass
{
struct RenderResources
{
  eastl::unique_ptr<PostFxRenderer> aoRenderer, blurRenderer;
  UniqueBufWithShaderVar poissonPoints; // unbound if creation skipped it (SSAO_SKIP_POISSON_POINTS_GENERATION)
};

RenderResources create_render_resources(uint32_t flags, const char *poisson_tag = "");

void resolve(const RenderResources &res, ao_common::FrameState &frame_state, ao_common::ReprojectionState *reprojection,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos, SubFrameSample sub_sample, bool is_main_view);

void two_pass_blur(const RenderResources &res, uint32_t frame_no, BaseTexture *ssao_tex, BaseTexture *tmp_tex, int width, int height);

} // namespace ssao_pass

// Renderer class which manages the resources automatically.
// Used only in legacy code and will be removed in favor of daFG make_*_node() implementation.
class SSAORenderer : public ISSAORenderer
{
public:
  using ISSAORenderer::render;

  SSAORenderer() = delete;
  SSAORenderer(int w, int h, int num_views, uint32_t flags = SSAO_NONE, bool use_own_textures = true, const char *tag = "");
  virtual ~SSAORenderer();

  void reset() override;

  Texture *getSSAOTex() override;
  TEXTUREID getSSAOTexId() override;

  void setCurrentView(int view) override;

  void renderSSAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssao_tex, BaseTexture *prev_ssao_tex,
    const DPoint3 *world_pos, SubFrameSample sub_sample, bool clear_rt);

  void applySSAOBlur(BaseTexture *ssao_tex, BaseTexture *tmp_tex);

private:
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssaoDepthTexUse, BaseTexture *ssaoTex,
    BaseTexture *prevSsaoTex, BaseTexture *tmpTex, const DPoint3 *world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single) override;

  // Not virtual: called from the constructor, where a virtual call would not reach an override.
  void resetImpl();

  struct ViewSpecific
  {
    ao_common::FrameState frameState;
    ao_common::ReprojectionState reprojection;
    RTarget::Ptr ssaoTex;
  };

  RTargetPool::Ptr ssaoRTPool;
  ViewDependentResource<ViewSpecific, 2> viewSpecific;
  ssao_pass::RenderResources resources;

  bool useOwnTextures;
};
