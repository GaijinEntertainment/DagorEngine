//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_resizableTex.h>
#include <resourcePool/resourcePool.h>
#include <generic/dag_carray.h>
#include <render/dof/dofProperties.h>
#include <drv/3d/dag_texFlags.h>

struct DepthOfFieldTargets
{
  static constexpr int COC_COUNT = 3;
  static constexpr uint32_t LAYER_FORMAT = TEXFMT_A16B16G16R16F;
  static constexpr uint32_t COC_FORMAT = TEXFMT_R8;

  BaseTexture *nearLayer = nullptr;
  BaseTexture *farLayer = nullptr;
  BaseTexture *nearLayerTemp = nullptr;
  BaseTexture *farLayerTemp = nullptr;
  eastl::array<BaseTexture *, COC_COUNT> coc = {};
  BaseTexture *cocHistory = nullptr;
  BaseTexture *maxCocFar = nullptr;
};

class DepthOfFieldPS
{
public:
  DepthOfFieldPS(bool use_linear_blend = true) : width(0), height(0), dof_coc(), useLinearBlend(use_linear_blend) {}
  ~DepthOfFieldPS();
  void init(int w, int h);
  void changeResolution(int w, int h);
  // frame: frame or (w+1)/2 downsampled frame
  // fov_scale: can be just p.hk, which mean dof is set for 90 fov
  void perform(BaseTexture *frame, BaseTexture *close_depth, float zn, float zf, float fov_scale);
  void perform(const DepthOfFieldTargets &targets, BaseTexture *frame, BaseTexture *close_depth, float zn, float zf, float fov_scale);
  void apply(); // to current render target
  void setBlendDepthTex(BaseTexture *tex);
  void setDoFParams(const DOFProperties &focus_) { nFocus = focus_; }
  const DOFProperties &getDoFParams() const { return nFocus; }
  const DOFProperties &getUsedDoFParams() const { return cFocus; }
  void getBokehShape(float &kernelSize, float &bladesCount)
  {
    kernelSize = bokehShapeKernelSize;
    bladesCount = bokehShapeApertureBlades;
  }
  void setBokehShape(float kernelSize, float bladesCount)
  {
    bokehShapeKernelSize = clamp(kernelSize, 1.f, 8.f);
    bokehShapeApertureBlades = clamp(bladesCount, 3.f, 16.f);
  }
  // this is distance where to check if dof exists. If it is negatve it will be checked against znear
  // obviously, small znear(it is usually small) will usually HAVE some circle of confusion (if dof is on at all), so it is basically
  // "always on".
  //  Unlike far CoC/DOF, nearest has no limit and grows to infinity.
  void setMinCheckDistance(float dist) { minCheckDist = dist; }
  float getMinCheckDistance() const { return minCheckDist; }
  void setOn(bool on_);
  bool isOn() const { return on; }
  void setSimplifiedRendering(bool sr) { useSimplifiedRendering = sr; }
  bool isSimplifiedRendering() const { return useSimplifiedRendering; }
  void setCocAccumulation(bool acc) { useCoCAccumulation = acc; }
  bool isCocAccumulationEnabled() const { return useCoCAccumulation; }
  void setLinearBlend(bool use_linear_blend) { useLinearBlend = use_linear_blend; }
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  void releaseRTs();
  void closeTextures();
  void resetOutputVars();

protected:
  void initNear();
  void initFar();
  void closeNear();
  void closeFar();
  void changeNearResolution();
  void changeFarResolution();
  bool prepareFrame(BaseTexture *frame, float zn, float zf, float &fov_scale, bool &has_near_dof, bool &has_far_dof);
  void renderLayers(const DepthOfFieldTargets &targets, BaseTexture *frame, BaseTexture *close_depth, float zn, float zf,
    float fov_scale, bool has_near_dof, bool has_far_dof);

  bool useLinearBlend;
  bool useSimplifiedRendering = false;
  bool useCoCAccumulation = false;
  bool historyValid = false;
  uint32_t historyFrame = 0;
  PostFxRenderer dofGather, dofComposite, dofDownscale, dofTile;
  int width, height;
  int originalWidth = 0, originalHeight = 0;
  ResizableTex dof_max_coc_far;
  ResizableRTargetPool::Ptr dofLayerRTPool;
  ResizableRTarget::Ptr dof_far_layer, dof_near_layer;
  carray<ResizableTex, DepthOfFieldTargets::COC_COUNT> dof_coc;
  ResizableTex dof_coc_history;
  DOFProperties cFocus, nFocus;
  float bokehShapeKernelSize = 4, bokehShapeApertureBlades = 6;
  bool performedNearDof = false, performedFarDof = false;
  float minCheckDist = 0.08f;
  bool on = true;
  d3d::SamplerHandle clampSampler = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle clampPointSampler = d3d::INVALID_SAMPLER_HANDLE;
};
