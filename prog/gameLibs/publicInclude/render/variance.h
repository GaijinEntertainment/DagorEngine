//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_resPtr.h>
#include <resourcePool/resourcePool.h>

class PseudoGaussBlur;

class Variance
{
public:
  struct CullingInfo
  {
    TMatrix viewInv;
    TMatrix4_vec4 viewProj;
  };

  float light_full_update_threshold; // = 0.04;
  float light_update_threshold;      // = 0.005;
  float box_full_update_threshold;   // = 50*50;
  float box_update_threshold;        // = 20*20;
  float enable_vsm_treshold;         // =0.8 (light dir cos)
  enum VsmType
  {
    VSM_HW,
    VSM_BLEND
  };
  VsmType vsmType = VsmType::VSM_HW;
  void init(int w, int h, VsmType vsmTypeIn = VSM_HW);
  void close();
  bool isInited() const { return inited; }

  Variance();
  ~Variance() { close(); }

  // should render results, if true
  bool startShadowMap(const BBox3 &box, const Point3 &light_dir, float shadow_dist, CullingInfo *out_culling_info = nullptr);
  void endShadowMap();
  void debugRenderShadowMap();
  void forceUpdate(int frameDelay = 0) { forceUpdateFrameDelay = frameDelay; }
  bool isUpdateForced() const { return forceUpdateFrameDelay == 0; }
  bool needUpdate() const { return isUpdateForced(); } // Ignore box position change or light direction change.
  bool needUpdate(const Point3 &light_dir) const;
  void setOff();
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  float getShadowDist() const { return updateShadowDist; }

  Texture *getVsmShadowmap() const { return targ_tex ? targ_tex->getTex2D() : nullptr; }
  d3d::SamplerHandle getVsmSampler() const { return vsmSampler; }

protected:
  bool inited = false;

  PseudoGaussBlur *blur;
  int width, height;

  RTarget::Ptr temp_tex;
  RTarget::Ptr targ_tex;
  UniqueTex dest_tex;
  d3d::SamplerHandle vsmSampler = d3d::INVALID_SAMPLER_HANDLE;

  RTargetPool::Ptr vsmRTPool;

  Driver3dRenderTarget oldrt;
  TMatrix4 lightProj, shadowProjMatrix;
  ViewProjMatrixContainer savedViewProj;
  int vsmShadowProjXVarId, vsmShadowProjYVarId, vsmShadowProjZVarId, vsmShadowProjWVarId;
  int vsm_shadowmapVarId;
  int shadow_distVarId;
  int vsm_shadow_tex_sizeVarId;
  Point3 oldLightDir;
  BBox3 oldBox;
  unsigned update_state;

  BBox3 updateBox;
  Point3 updateLightDir;
  float updateShadowDist;
  int forceUpdateFrameDelay = -1;
  shaders::UniqueOverrideStateId blendOverride, depthOnlyOverride;
};
