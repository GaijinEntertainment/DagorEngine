// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace eastl
{
template <typename T>
class optional;
};

namespace scene
{
class TiledScene;
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
  static class ShadowsManager &getShadowsManager();
  static struct ClusteredLights &getClusteredLights();
  static class VolumeLight *getVolumeLight();

  // TODO: remove this when mutex is removed from the corresponding WorldRenderer methods.
  // Then, it would be possible to just use getClusteredLights() defined above.
  static uint32_t addSpotLight(const SpotLight &light, uint8_t mask);
  static void setLight(uint32_t id_, const SpotLight &light, uint8_t mask, bool invalidate_shadow);
  static void destroyLight(uint32_t id);

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
  static bool shouldHideGui();
};
