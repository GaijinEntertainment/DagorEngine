//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/fixed_function.h>
#include <generic/dag_tabFwd.h>


class DataBlock;
struct Frustum;
class Point2;
class Point3;
class TMatrix;
struct TMatrix4_vec4;
class TMatrix4;
class BBox3;
class String;
class CascadeShadowsPrivate;
class BaseTexture;

typedef eastl::fixed_function<sizeof(void *) * 4, void(int num_cascades_to_render, bool clear_per_view)> csm_render_cascades_cb_t;

class ICascadeShadowsClient
{
public:
  virtual void getCascadeShadowAnchorPoint(float cascade_from, Point3 &out_anchor) = 0;
  virtual void prepareRenderShadowCascades(){};
  virtual void renderCascadeShadowDepth(int cascade_no, const Point2 &znzf) = 0;

  virtual void getCascadeShadowSparseUpdateParams(int cascade_no, const Frustum &cascade_frustum, float &out_min_sparse_dist,
    int &out_min_sparse_frame) = 0;
};


class CascadeShadows
{
public:
  static constexpr int MAX_CASCADES = 6;
  static constexpr int SSSS_CASCADES = 3;

  struct Settings
  {
    int cascadeWidth = 1024;
    bool cascadeDepthHighPrecision = false;
    int splitsW = 2;
    int splitsH = 2;
    float fadeOutMul = 1;
    float shadowFadeOut = 10.f;
    float shadowDepthBias = 0.01;
    float shadowConstDepthBias = 0.00002;
    float shadowDepthSlopeBias = 0.83;
    float zRangeToDepthBiasScale = 1e-4;
  };

  struct ModeSettings
  {
    ModeSettings();

    float powWeight;
    float maxDist;
    float shadowStart;
    int numCascades;
    float shadowCascadeZExpansion;
    float shadowCascadeRotationMargin;
    float cascade0Dist; // if cascade0Dist is >0, it will be used as minimum of auto calculated cascade dist and this one. This is to
                        // artificially create high quality cascade for 'cockpit'
    float overrideZNearForCascadeDistribution;
  };


private:
  CascadeShadows(ICascadeShadowsClient *in_client, const Settings &in_settings);

public:
  static CascadeShadows *make(ICascadeShadowsClient *in_client, const Settings &in_settings);
  ~CascadeShadows();

  void prepareShadowCascades(const CascadeShadows::ModeSettings &mode_settings, const Point3 &dir_to_sun, const TMatrix &view_matrix,
    const Point3 &camera_pos, const TMatrix4 &proj_tm, const Frustum &view_frustum, const Point2 &scene_z_near_far,
    float z_near_for_cascade_distribution);
  const Settings &getSettings() const;
  void setDepthBiasSettings(const Settings &set);
  void setCascadeWidth(int cascadeWidth);
  void renderShadowsCascades();
  void renderShadowsCascadesCb(csm_render_cascades_cb_t cb);
  void renderShadowCascadeDepth(int cascadeNo, bool clearPerView);

  void setCascadesToShader();
  void disable();
  bool isEnabled() const;
  void invalidate(); // Reset sparse counters.

  int getNumCascadesToRender() const;
  const Frustum &getFrustum(int cascade_no) const;
  const Point3 &getRenderCameraWorldViewPos(int cascade_no) const;
  const TMatrix &getShadowViewItm(int cascade_no) const;
  const TMatrix4_vec4 &getCameraRenderMatrix(int cascade_no) const;
  const TMatrix4_vec4 &getWorldCullingMatrix(int cascade_no) const;
  const TMatrix4_vec4 &getWorldRenderMatrix(int cascade_no) const;
  const TMatrix4_vec4 &getRenderViewMatrix(int cascade_no) const;
  const TMatrix4_vec4 &getRenderProjMatrix(int cascade_no) const;
  const Point3 &shadowWidth(int cascade_no) const;
  const BBox3 &getWorldBox(int cascade_no) const;
  bool shouldUpdateCascade(int cascade_no) const;
  bool isCascadeValid(int cascade_no) const;
  void copyFromSparsed(int cascade_no);
  float getMaxDistance() const;
  float getCascadeDistance(int cascade_no) const;
  float getMaxShadowDistance() const;
  const Frustum &getWholeCoveredFrustum() const;
  BaseTexture *getShadowsCascade() const;
  const Point2 &getZnZf(int cascade_no) const;

  const String &setShadowCascadeDistanceDbg(const Point2 &scene_z_near_far, int tex_size, int splits_w, int splits_h,
    float shadow_distance, float pow_weight);

  void debugSetParams(float shadow_depth_bias, float shadow_const_depth_bias, float shadow_depth_slope_bias);

  void debugGetParams(float &shadow_depth_bias, float &shadow_const_depth_bias, float &shadow_depth_slope_bias);
  void setNeedSsss(bool need_ssss);

private:
  CascadeShadowsPrivate *d;
};
