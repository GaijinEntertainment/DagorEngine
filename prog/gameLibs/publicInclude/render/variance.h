//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_drv3d.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>
#include <shaders/dag_overrideStateId.h>

class PseudoGaussBlur;

class Variance
{
public:
  float light_full_update_threshold; // = 0.04;
  float light_update_threshold;      // = 0.005;
  float box_full_update_threshold;   // = 50*50;
  float box_update_threshold;        // = 20*20;
  float enable_vsm_treshold;         // =0.8 (light dir cos)
  enum VsmType
  {
    VSM_SW,
    VSM_HW,
    VSM_BLEND
  };
  VsmType vsmType;
  void init(int w, int h, VsmType vsmTypeIn = VSM_HW);
  void close();
  bool isInited() const { return dest_tex != NULL || targ_tex != NULL; }

  Variance();
  ~Variance() { close(); }

  bool startShadowMap(const BBox3 &box, const Point3 &light_dir,
    float shadow_dist); // should render results, if true
  void endShadowMap();
  void debugRenderShadowMap();
  void forceUpdate() { isUpdateForced = true; }
  bool updateForced() const { return isUpdateForced; }
  bool needUpdate() const { return isUpdateForced; } // Ignore box position change or light direction change.
  bool needUpdate(const Point3 &light_dir) const;
  void setOff();
  void setDest(Texture *dest, TEXTUREID destId)
  {
    dest_tex = dest;
    dest_texId = destId;
  }
  void getDest(Texture *&dest, TEXTUREID &destId) const
  {
    dest = dest_tex;
    destId = dest_texId;
  }
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  float getShadowDist() const { return updateShadowDist; }

protected:
  PseudoGaussBlur *blur;
  int width, height;

  Texture *temp_tex, *targ_tex, *dest_tex;
  TEXTUREID temp_texId, targ_texId, dest_texId;
  Texture *temp_wrtex, *targ_wrtex;

  Driver3dRenderTarget oldrt;
  TMatrix4 lightProj, shadowProjMatrix;
  TMatrix4 svtm;
  Driver3dPerspective persp;
  bool perspOk;
  int vsmShadowProjXVarId, vsmShadowProjYVarId, vsmShadowProjZVarId, vsmShadowProjWVarId;
  int vsm_shadowmapVarId;
  int shadow_distVarId;
  int vsm_hw_filterVarId;
  int vsm_shadow_tex_sizeVarId;
  Point3 oldLightDir;
  BBox3 oldBox;
  unsigned update_state;

  BBox3 updateBox;
  Point3 updateLightDir;
  float updateShadowDist;
  bool isUpdateForced;
  shaders::UniqueOverrideStateId blendOverride, depthOnlyOverride;
};
