// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <math/dag_TMatrix4.h>
#include <resourcePool/resourcePool.h>
#include <render/viewDependentResource.h>
#include <render/subFrameSample.h>
#include <render/noiseTex.h>
#include <render/viewVecs.h>
#include <generic/dag_carray.h>

#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>

enum class SSRQuality
{
  Low,
  Medium,
  Compute,
};
class ComputeShaderElement;

namespace ssr_pass
{

// How many mips the ssr texture have with this quality.
int32_t get_mip_count(SSRQuality quality);
// Clamps the requested quality to what the driver supports and completes the texture create flags.
void get_real_quality_and_fmt(uint32_t &fmt, SSRQuality &quality);

struct RenderResources
{
  // PS path.
  PostFxRenderer quad, mipChain;
  // CS path.
  eastl::unique_ptr<ComputeShaderElement> compute;
  BlueNoiseRef blueNoise;
};

RenderResources create_render_resources(SSRQuality quality);

struct CameraState
{
  TMatrix viewTm = TMatrix::IDENT;
  TMatrix4 projTm = TMatrix4::IDENT;
  DPoint3 worldPos = DPoint3(0, 0, 0);
  ViewVecs viewVecs = {};
};

// Sets the reprojection shader vars.
void apply_reprojection(const CameraState &cur, const CameraState &prev);

struct TraceParams
{
  int32_t width = 0;
  int32_t height = 0;
  int32_t targetMipCount = 1;
  int32_t callId = 0;
  bool isHistoryValid = true;
};

void trace(const RenderResources &res, const TraceParams &params, const CameraState &cur, const CameraState &prev);

void build_mip_chain(const RenderResources &res, BaseTexture *ssr_tex, int32_t width, int32_t height);

} // namespace ssr_pass

class ScreenSpaceReflections
{
protected:
  ViewDependentResource<UniqueTex, 2, 1> ssrTarget;
  RTargetPool::Ptr rtTemp;

  struct ViewSpecific
  {
    ssr_pass::CameraState prevCamera;
    int frameNo = 0;
  };

  ViewDependentResource<ViewSpecific, 2, 1> viewSpecific;
  ssr_pass::RenderResources resources;

  int ssrW, ssrH;
  bool ownTextures;

  void updateSamplers() const;

public:
  ScreenSpaceReflections(int ssr_w, int ssr_h, int num_views = 1, uint32_t fmt = 0, SSRQuality ssr_quality = SSRQuality::Low,
    bool own_textures = true, const char *tag = "");
  ~ScreenSpaceReflections();

  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos, SubFrameSample sub_sample,
    BaseTexture *ssrTex, BaseTexture *ssrPrevTex, BaseTexture *targetTex, int callId);
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
    SubFrameSample sub_sample = SubFrameSample::Single);
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    DPoint3 worldPos = dpoint3(orthonormalized_inverse(view_tm).getcol(3));
    render(view_tm, proj_tm, worldPos, SubFrameSample::Single);
  }
};
