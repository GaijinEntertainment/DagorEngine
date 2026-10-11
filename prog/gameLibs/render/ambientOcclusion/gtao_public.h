// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ssao_base.h"
#include "ssao_common_public.h"
#include <3d/dag_resizableTex.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <render/viewDependentResource.h>

// Following code implements "Ground Truth Ambient Occlusion" algorithm
// Easiest way to understand the concept of it is to read
// "Practical Realtime Strategies for Accurate Indirect Occlusion" paper
namespace gtao_pass
{
struct FilterResources
{
  eastl::unique_ptr<PostFxRenderer> spatialFilterRenderer, temporalFilterRenderer;
  eastl::unique_ptr<ComputeShaderElement> spatialFilterRendererCS, temporalFilterRendererCS;
};

struct RenderResources
{
  eastl::unique_ptr<PostFxRenderer> aoRenderer;
  eastl::unique_ptr<ComputeShaderElement> aoRendererCS;
  FilterResources filter;
};

RenderResources create_render_resources(uint32_t flags);
FilterResources create_filter_resources(bool use_compute);

void resolve(const RenderResources &res, ao_common::FrameState &frame_state, ao_common::ReprojectionState *reprojection,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos, SubFrameSample sub_sample, bool is_main_view, int width,
  int height);

void spatiotemporal_filter(const FilterResources &res, BaseTexture *raw_tex, BaseTexture *prev_tex, BaseTexture *spatial_tex,
  int width, int height);

} // namespace gtao_pass

// Renderer class which manages the resources automatically.
// Used only in legacy code and will be removed in favor of daFG make_*_node() implementation.
class GTAORenderer : public ISSAORenderer
{
public:
  using ISSAORenderer::render;

  GTAORenderer() = delete;
  GTAORenderer(int w, int h, int views, uint32_t flags = SSAO_NONE, bool use_own_textures = true);
  virtual ~GTAORenderer();

  void reset() override;

  Texture *getSSAOTex() override;
  TEXTUREID getSSAOTexId() override;

  void setCurrentView(int viewIndex);

  void renderGTAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssao_tex, const DPoint3 *world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single);
  void applyGTAOFilter(BaseTexture *ssao_tex, BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex);

private:
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssaoDepthTexUse, BaseTexture *ssaoTex,
    BaseTexture *prevSsaoTex, BaseTexture *tmpTex, const DPoint3 *world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single) override;

  void updateCurFrameIdxs();

  uint32_t rawTargetIdx;
  uint32_t spatialTargetIdx;
  uint32_t historyIdx; // last frame's rawTargetIdx

  ViewDependentResource<ResizableTex, 2, 3> gtaoTex;
  gtao_pass::RenderResources resources;

  ViewDependentResource<ao_common::FrameState, 2> frameState;
  ViewDependentResource<ao_common::ReprojectionState, 2> reprojection;
  bool useOwnTextures;
};
