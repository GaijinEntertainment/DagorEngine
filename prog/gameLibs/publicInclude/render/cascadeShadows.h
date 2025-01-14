//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_tabFwd.h>


class DataBlock;
struct Frustum;
class Point2;
class Point3;
class Point4;
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
  // The returned point is used as an origin for shadow pixel-wise alignment.
  // If .w > 0.0, then it is treated as a sphere around which to render the cascade.
  // If .w <= 0.0 for cascade i, then it must be <= 0 for all the subsequent cascades.
  virtual Point4 getCascadeShadowAnchor(int cascade_no) = 0;
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
    // Skip rendering to CSM any destructable whose bounding box radius is less than
    // (static shadow texel size) * (this multiplier)
    float destructablesMinBboxRadiusTexelMul = 0.f;
    enum class ResourceAccessStrategy
    {
      Internal,
      External
    } resourceAccessStrategy = ResourceAccessStrategy::Internal;
  };

  struct ModeSettings
  {
    ModeSettings();

    float powWeight;
    float maxDist;
    float shadowStart;
    int numCascades;
    float shadowCascadeZExpansion;
    float shadowCascadeZExpansion_znearOffset;
    float shadowCascadeRotationMargin;
    float cascade0Dist; // if cascade0Dist is >0, it will be used as minimum of auto calculated cascade dist and this one. This is to
                        // artificially create high quality cascade for 'cockpit'
    float overrideZNearForCascadeDistribution;
    bool useFixedShadowCascade;
  };


private:
  CascadeShadows(ICascadeShadowsClient *in_client, const Settings &in_settings);

public:
  static CascadeShadows *make(ICascadeShadowsClient *in_client, const Settings &in_settings, String &&res_postfix = String());
  ~CascadeShadows();

  void prepareShadowCascades(const CascadeShadows::ModeSettings &mode_settings, const Point3 &dir_to_sun, const TMatrix &view_matrix,
    const Point3 &camera_pos, const TMatrix4 &proj_tm, const Frustum &view_frustum, const Point2 &scene_z_near_far,
    float z_near_for_cascade_distribution);
  const Settings &getSettings() const;
  void setDepthBiasSettings(const Settings &set);
  void setCascadeWidth(int cascadeWidth);
  void renderShadowsCascades();
  void renderShadowsCascadesCb(const csm_render_cascades_cb_t &cb, ManagedTexView external_cascades = {});
  void renderShadowCascadeDepth(int cascadeNo, bool clearPerView, ManagedTexView external_cascades = {});

  void setSamplingBiasToShader(float value);
  void setCascadesToShader();
  void setNearCullingNearPlaneToShader();
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
  d3d::SamplerHandle getShadowsCascadeSampler() const;
  const TextureInfo &getShadowCascadeTexInfo() const;
  const Point2 &getZnZf(int cascade_no) const;

  const String &setShadowCascadeDistanceDbg(const Point2 &scene_z_near_far, int tex_size, int splits_w, int splits_h,
    float shadow_distance, float pow_weight);

  void debugSetParams(float shadow_depth_bias, float shadow_const_depth_bias, float shadow_depth_slope_bias);

  void debugGetParams(float &shadow_depth_bias, float &shadow_const_depth_bias, float &shadow_depth_slope_bias);
  void setNeedSsss(bool need_ssss);

private:
  CascadeShadowsPrivate *d;
};
