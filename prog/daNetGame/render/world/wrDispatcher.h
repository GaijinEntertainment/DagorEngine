// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "render/light_mask_inc.hlsli"
#include <render/world/waterRenderMode.h>
#include <render/heroData.h>
#include <3d/dag_resPtr.h>


namespace eastl
{
template <typename T>
class optional;
};

namespace scene
{
class TiledScene;
};

namespace shaders
{
struct UniqueOverrideStateId;
};

struct SpotLight;
class BaseStreamingSceneHolder;

// Use these functions to dispatch into different components of
// worldRenderer without needing to include the entirety of
// private_worldRenderer.h.
// Necessary components return a reference, optional ones return
// a (maybe null) pointer.
// The actual implementation of these functions is hidden in
// worldRenderer.cpp.
struct WRDispatcher
{
  struct CommonOverrideStates
  {
    const shaders::UniqueOverrideStateId &depthClipState;
    const shaders::UniqueOverrideStateId &flipCullStateId;
    const shaders::UniqueOverrideStateId &zFuncAlwaysStateId;
    const shaders::UniqueOverrideStateId &additiveBlendStateId;
    const shaders::UniqueOverrideStateId &enabledDepthBoundsId;
    const shaders::UniqueOverrideStateId &zFuncEqualStateId;
  };

  static bool needSeparatedUI();

  static bool isUpsampling();

  static bool isFsrEnabled();
  static int getCurrentAntiAliasingMode();

  static struct CameraParams &getCurrentCameraParams();
  static const CameraParams &getPreviousCameraParams();
  static int getTemporalShadowFramesCount();
  static void updateTransformations(const DPoint3 &move,
    const TMatrix4_vec4 &jittered_cam_pos_to_unjittered_history_clip,
    const TMatrix4_vec4 &prev_origo_relative_view_proj_tm); // move currentWorldPos - prevWorldPos
  static class CameraViewVisibilityMgr *getMainCameraVisibilityMgr();

  static HeroWtmAndBox &getHeroData();

  static float getDaGdpRangeScale();
  static struct SkiesData *getMainPovSkiesData();

  static const ManagedTex &getFinalTargetFrame();
  static int getGbufferTargetGlobalFlags();

  static bool isReadyToUse();
  static bool hasHighResFx();

  static IPoint2 getSubSuperPixels();
  static Point3 getSunDirection();
  static Color3 getSunColor();

  static const DataBlock *getLevelSettings();

  static float GetWaterLevel();
  static WaterRenderMode determineWaterRenderMode(bool underWater, bool belowClouds);

  static void getDisplayResolution(int &w, int &h);
  static void getDownsampledDepthParams(uint32_t &downsampledTexturesMipCount, bool &storeDownsampledTexturesInEsram);

  static float getGameTime();
  static float getRealDeltaTime();

  static class EnviCover *getEnviCover();
  static bool isEnviCover();
  static bool isEnviCoverCompatible();

  static class AntiAliasing *getAntialiasing();
  static class ShadowsManager &getShadowsManager();
  static struct ClusteredLights &getClusteredLights();
  static class VolumeLight *getVolumeLight();
  static struct IShadowInfoProvider &getShadowInfoProvider();

  // TODO: remove this when mutex is removed from the corresponding WorldRenderer methods.
  // Then, it would be possible to just use getClusteredLights() defined above.
  static uint32_t addSpotLight(const SpotLight &light, SpotLightMaskType mask);
  static void setLight(uint32_t id_, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow);
  static void destroyLight(uint32_t id);

  static class Clipmap *getClipmap();
  static class LandMeshManager *getLandMeshManager();
  static class LandMeshRenderer *getLandMeshRenderer();

  static class BaseStreamingSceneHolder *getBinScene();

  static class DepthAOAboveContext *getDepthAOAboveCtx();

  static class DeformHeightmap *getDeformHeightmap();
  static class IndoorProbeManager *getIndoorProbeManager();

  static struct MotionBlurNodePointers getMotionBlurNodePointers();
  static const scene::TiledScene *getEnviProbeBoxesScene();
  static void ensureIndoorProbeDebugBuffersExist();
  static eastl::optional<Point4> getHmapDeformRect();
  static void getMinMaxZ(float &min, float &max);
  static bool usesDepthPrepass();
  static CommonOverrideStates getCommonOverrideStates();
  static bool shouldHideGui();
  static bool isBareMinimum();
  static void updateWorldBBox(const BBox3 &additional_bbox);
  enum RTDependentFeature : uint32_t
  {
    WATER = 1u << 0u,
    SSAO = 1u << 1u,
    GI = 1u << 2u,
    SSR = 1u << 3u,
    REINIT_TARGET = 1u << 4u,
  };
  static void recreateRayTracingDependentNodes(uint32_t features_to_reset);
};
