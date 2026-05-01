// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// this file should never be included outside of /renderer/ subfolder!
#ifndef INSIDE_RENDERER
#error Do NOT (in any case) include this file outside of renderer sub folder.
#error This is internal implementation, and should not be included in server or anywhere else!
#endif

#include "render/rendererFeatures.h"
#include "render/renderer.h"

#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>
#include <math/dag_frustum.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <resourcePool/resourcePool.h>
#include <3d/dag_textureIDHolder.h>
#include <render/toroidalHelper.h>
#include <render/clusteredLights.h>
#include <rendInst/rendInstExtra.h>
#include <landMesh/lmeshCulling.h>
#include "gpuDeformObjects.h"
#include <EASTL/unique_ptr.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_overrideStateId.h>
#include <render/world/fomShadowsManager.h>
#include <render/world/deformHeightmap.h>
#include <render/world/rendInstHeightmap.h>
#include <render/gpuVisibilityTest.h>
#include <ioSys/dag_dataBlock.h>
#include <render/fx/fx.h>
#include <daECS/core/componentTypes.h>
#include <3d/dag_texStreamingContext.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <math/dag_TMatrix4D.h>
#include <render/world/frameGraphNodes/motionBlurNode.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/deferredRenderer.h>
#include <render/antialiasing.h>
#include <render/antiAliasing_legacy.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <render/motionVectorAccess.h>
#include <render/heroData.h>
#include "satelliteRenderer.h"

#include "antiAliasingMode.h"
#include "cameraParams.h"
#include "shadowsManager.h"
#include "postFxManager.h"
#include "partitionSphere.h"
#include "shoreRenderer.h"

class CollisionResource;
class Point2;
class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture Texture2D;
struct Frustum;
class DataBlock;
class BaseStreamingSceneHolder;
class RenderScene;
class LandMeshRenderer;
class LandMeshManager;
struct LandMeshCullingState;
struct LandMeshCullingData;
class ComputeShaderElement;
class AntiAliasing;
class LatencyInputEventListener;
class EmissionColorMaps;
class DynamicQuality;
class DynamicResolution;
class MultiFramePGF;

class GIWindows;
class DaGI;
struct LRUCollisionVoxelization;

class DaSkies;
struct SkiesData;
class DebugTexOverlay;
class DebugTonemapOverlay;
class Clipmap;
class VolumeLight;
class AdaptationCS;
class DeferredRT;
class RenderDynamicCube;
struct RiGenVisibility;
class DynamicShaderHelper;
class DepthAOAboveContext;
class RenderDepthAOCB;
class RendInstHeightmap;
struct GrassRenderer;
class DebugLightProbeSpheres;
class LightProbeSpecularCubesContainer;
class LRURendinstCollision;
class DebugBoxRenderer;
class IndoorProbeNodes;
class IndoorProbeManager;
class TreesAboveDepth;
class Occlusion;
class GaussMipRenderer;
union LightShadowParams;
struct AimRenderingData;
class DebugLightProbeShapeRenderer;
class DebugCollisionDensityRenderer;
class IndoorProbeScenes;
class DynamicShadowRenderExtender;
class EnviCover;

enum class WaterRenderMode;
enum class MainNodeRenderPass;

namespace light_probe
{
class Cube;
extern void destroy(Cube *cube);
}; // namespace light_probe
namespace scene
{
class TiledScene;
};
namespace rendinst
{
struct RIOcclusionData;
};

// if flag is set - feature is available
class WorldRenderer;

class WorldRenderer final : public IRenderWorld, public IShadowInfoProvider
{
  friend RenderDepthAOCB;
  friend RendInstHeightmap;
  friend GrassRenderer;
  friend ShoreRenderer;

  friend struct WRDispatcher;

  friend eastl::fixed_vector<dafg::NodeHandle, 3> makeControlOpaqueDynamicsNodes(const char *prev_region_ns);
  friend dafg::NodeHandle makeOpaqueDynamicsNode(MainNodeRenderPass);
  friend eastl::fixed_vector<dafg::NodeHandle, 4> makeControlOpaqueStaticsNodes(const char *prev_region_ns);
  friend eastl::fixed_vector<dafg::NodeHandle, 2> makeOpaqueMainNodes(dafg::NameSpace, bool, MainNodeRenderPass);
  friend eastl::fixed_vector<dafg::NodeHandle, 8> makeOpaqueStaticNodes(bool);

  friend dafg::NodeHandle makePrepareGbufferNode(
    uint32_t global_flags, uint32_t gbuf_cnt, eastl::span<uint32_t> main_gbuf_fmts, bool has_motion_vectors, bool is_rr_enabled);
  friend dafg::NodeHandle makePrepareLightsNode();
  friend dafg::NodeHandle makeDepthWithTransparencyNode();
  friend dafg::NodeHandle makeDownsampleDepthWithTransparencyNode();
  friend dafg::NodeHandle makeSSAANode();
  friend eastl::fixed_vector<dafg::NodeHandle, 3> makeFsrNodes();
  friend dafg::NodeHandle makeStaticUpsampleNode(const char *source_name);
  friend resource_slot::NodeHandleWithSlotsAccess makePreparePostFxNode();
  friend dafg::NodeHandle makePrepareTiledLightsNode();
  friend void makeDebugVisualizationNodes(eastl::vector<dafg::NodeHandle> &);

  // new GI
  friend dafg::NodeHandle makeGiScreenDebugNode();
  friend dafg::NodeHandle makeGiScreenDebugDepthNode();
  friend dafg::NodeHandle makeGiCalcNode();
  friend dafg::NodeHandle makeGiFeedbackNode();

  friend dafg::NodeHandle makePrepareWaterNode();
  friend eastl::fixed_vector<dafg::NodeHandle, 5, false> makeSubsamplingNodes(bool sub_sampling, bool super_sampling);
  friend dafg::NodeHandle makeFrameToPresentProducerNode();
  friend eastl::array<dafg::NodeHandle, 2> makeExternalFinalFrameControlNodes(bool requires_multisampling);
  friend dafg::NodeHandle makePostfxTargetProducerNode(bool requires_multisampling);
  friend dafg::NodeHandle makeAimDofRestoreNode();
  friend dafg::NodeHandle makeGroundNode(bool early, MainNodeRenderPass mode);
  friend eastl::array<dafg::NodeHandle, 2> makeSceneShadowPassNodes(const DataBlock *level_blk);
  friend dafg::NodeHandle makeTransparentSceneLateNode(MainNodeRenderPass mode);
  friend dafg::NodeHandle makeAcesFxTransparentNode();

  friend dafg::NodeHandle makeAfterWorldRenderNode();
  friend eastl::array<dafg::NodeHandle, 9> makeVolumetricLightsNodes();
  friend dafg::NodeHandle makeWaterNode(WaterRenderMode mode);
  friend eastl::fixed_vector<dafg::NodeHandle, 4, false> makeWaterSSRNode(WaterRenderMode mode);
  friend void acesfx::finish_update(const TMatrix4 &tm, Occlusion *occlusion);
  friend eastl::fixed_vector<dafg::NodeHandle, 2, false> makeCameraInCameraSetupNodes();

  friend dafg::NodeHandle makeReprojectedHzbImportNode(); // remove when non-multiplexed camera-view params will be added to fg

  // platform/game specific default initialized once
  FeatureRenderFlagMask defaultFeatureRenderFlags;

  struct GiFullInvalidationRequest
  {
    bool force;
  };
  struct GiPartialInvalidateRequest
  {
    BBox3 modelBbox;
    TMatrix tm;
    BBox3 approx;
  };
  eastl::optional<GiFullInvalidationRequest> pendingFullGiInvalidationRequest;
  eastl::optional<GiPartialInvalidateRequest> pendingPartialGiInvalidationRequest;
  void processGIInvalidationRequests();

  bool hasPendingHeroTeleportation = false;
  void processHeroTeleportation();

public:
  void onHeroTeleportation() { hasPendingHeroTeleportation = true; };

  bool hasFeature(FeatureRenderFlags f) const { return renderer_has_feature(f); }
  FeatureRenderFlagMask getFeatures() const { return get_current_render_features(); }

  WorldRenderer();
  WorldRenderer(const WorldRenderer &) = delete;
  ~WorldRenderer();

  // IRenderWorld
  virtual void update(float dt, float real_dt, const TMatrix &itm); // can be called independent on draw
  void setUpView(const TMatrix &view_itm, const DPoint3 &view_pos, const Driver3dPerspective &persp, const TMatrix4D &proj_tm);
  virtual void beforeRender(float scaled_dt,
    float act_dt,
    float real_dt,
    float game_time,
    const TMatrix &view_itm,
    const DPoint3 &view_pos,
    const Driver3dPerspective &persp,
    const TMatrix4D &proj_tm) override;

  void updateLodsScaling();

  void startOcclusionAndSwRaster();
  virtual void draw(uint32_t frame_id, float real_dt) override;
  virtual void debugDraw() override;
  virtual void beforeLoadLevel(const DataBlock &level_blk) override; // to be called from main thread
  // to be called from main thread. It will keep reference on LandMeshManager *lmesh and OWN BaseStreamingSceneHolder *scn
  virtual void onLevelLoaded(const DataBlock &level_blk);

  void initIndoorProbeShapes(eastl::unique_ptr<IndoorProbeScenes> &&scene_ptr);

  virtual void setWater(FFTWater *water); // to be called from main thread. It will keep reference on water!
  virtual FFTWater *getWater();
  virtual void setMaxWaterTessellation(int value) override;
  ShoreRenderer *getShore() override;

  // to be called from loading thread
  virtual void onSceneLoaded(BaseStreamingSceneHolder *scn);
  // to be called from loading thread
  virtual void onLightmapSet(TEXTUREID lmap_tid);
  // to be called from loading thread
  virtual void onLandmeshLoaded(const DataBlock &level_blk, const char *bin_scene, LandMeshManager *lmesh);
  void onUIFinished();

  virtual void unloadLevel();
  virtual void closeNBSShaders();
  void beforeResetNBS();
  void afterResetNBS();

  const ManagedTex &getFinalTargetTex() const override
  {
    G_ASSERT(finalTargetFrame);
    return *finalTargetFrame;
  }
  Texture *getFinalTargetTex2D() const override
  {
    return finalTargetFrame && *finalTargetFrame ? finalTargetFrame->getTex2D() : nullptr;
  }
  const ManagedTex &getBackbufferTex() const { return backbufferTex; }
  const ManagedTex *getFinalTargetManagedTex() const { return finalTargetFrame; }

  FxRenderTargetOverride getFxRtOverride() const { return fxRtOverride; }

  ManagedTexView getSuperResolutionScreenshot() const override { return screenshotSuperFrame; }
  AntiAliasing *getAntialiasing() { return antiAliasing.get(); }

  virtual void onSettingsChanged(const FastNameMap &changed_fields, bool apply_after_reset);
  virtual void beforeDeviceReset(bool full_reset);
  virtual void afterDeviceReset(bool full_reset);

  void windowResized() override;

  virtual void updateFinalTargetFrame() override;

  virtual void preloadLevelTextures() override;

  //
  // todo: instead of direct mapping wecan create some grid structire to hide and remove lights which are too far
  void destroyLight(light_id id) { lights.destroyLight(id); } // this one has to be replaced with queue

  light_id addOmniLight(const OmniLight &light, OmniLightMaskType mask = OmniLightMaskType::OMNI_LIGHT_MASK_DEFAULT)
  {
    return lights.addOmniLight(light, mask);
  }

  // keep mask
  void setLight(light_id id, const OmniLight &light, bool invalidate_shadow)
  {
    lights.setLight(id, light, invalidate_shadow);
  } // todo:this one has to be replaced with queue

  void setLightWithMask(light_id id, const OmniLight &light, OmniLightMaskType mask, bool invalidate_shadow)
  {
    lights.setLightWithMask(id, light, mask, invalidate_shadow);
  } // todo:this one has to be replaced with queue

  OmniLight getOmniLight(light_id id) const { return lights.getOmniLight(id); }

  void setLight(light_id id, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow)
  {
    lights.setLight(id, light, mask, invalidate_shadow);
  } // todo:this one has to be replaced with queue

  SpotLight getSpotLight(light_id id) const { return lights.getSpotLight(id); }

  light_id addSpotLight(const SpotLight &light, SpotLightMaskType mask) { return lights.addSpotLight(light, mask); }

  void getSpotLightShadowViewProj(light_id id, mat44f &view_itm, mat44f &proj)
  {
    return lights.getSpotLightShadowViewProj(id, view_itm, proj);
  }

  bool isLightVisible(light_id id) const { return lights.isLightVisible(id); }

  bool addShadowToLight(light_id id,
    ShadowCastersFlag casters,
    bool hint_dynamic,
    uint16_t quality,
    uint8_t priority,
    uint8_t shadow_size_srl,
    DynamicShadowRenderGPUObjects render_gpu_objects)
  {
    return lights.addShadowToLight(id, casters, hint_dynamic, quality, priority, shadow_size_srl, render_gpu_objects);
  }
  bool getShadowProperties(uint32_t id,
    ShadowCastersFlag &casters,
    bool &hint_dynamic,
    uint16_t &quality,
    uint8_t &priority,
    uint8_t &shadow_size_srl,
    DynamicShadowRenderGPUObjects &render_gpu_objects) const
  {
    return lights.getShadowProperties(id, casters, hint_dynamic, quality, priority, shadow_size_srl, render_gpu_objects);
  }
  void removeShadow(light_id id) { return lights.removeShadow(id); }

  void updateLightShadow(light_id light, const LightShadowParams &shadow_params);

  ShadowsQuality getSettingsShadowsQuality() const override { return shadowsManager.getSettingsQuality(); }

  void getSunSph0Color(float height, Color3 &oSun, Color3 &sph0); // actually, should be part of interface. todo: move to IRenderWorld

  void shadowsInvalidate(const BBox3 &box) override { shadowsManager.shadowsInvalidate(box); }
  void shadowsAddInvalidBBox(const BBox3 &box) override { shadowsManager.shadowsAddInvalidBBox(box); }

  dynamic_shadow_render::QualityParams getShadowRenderQualityParams() const override;
  DynamicShadowRenderExtender::Handle registerShadowRenderExtension(DynamicShadowRenderExtender::Extension &&extension) override;

  //
  void startRenderLightProbe(const Point3 &view_pos, const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void renderLightProbeOpaque(const Point3 &view_pos, const TMatrix &view_itm, const Frustum &);
  void renderLightProbeEnvi(const TMatrix &view, const TMatrix4 &proj, const Driver3dPerspective &persp);
  void prepareLightProbeRIVisibility(const mat44f &globtm, const Point3 &view_pos);
  void prepareLightProbeRIVisibilityAsync(const mat44f &globtm, const Point3 &view_pos);
  void endRenderLightProbe();

  inline void invalidateCubeReloadOnly() { enviProbeNeedsReload = true; }

  // just other public
  void init();
  void createNodes();
  // Recreates nodes right before rendering the frame, causing a lag
  // spike. SHould only be used for debug (e.g. pulling convars)
  void debugRecreateNodes();

  void createFinalOpaqueControlNodes();
  void createGiNodes();

  static WaterRenderMode determineWaterRenderMode(bool underWater, bool belowClouds);
  struct AntiAliasingAppGlue : public render::antialiasing::AppGlue
  {
    TMatrix4 getUvReprojectionToPrevFrameTmNoJitter() const override { return TMatrix4::IDENT; }
    IPoint2 getInputResolution() const override
    {
      if (auto *wr = get_wr())
      {
        IPoint2 result;
        wr->getRenderingResolution(result.x, result.y);
        return result;
      }
      return IPoint2::ZERO;
    }
    IPoint2 getOutputResolution() const override
    {
      if (auto *wr = get_wr())
      {
        IPoint2 result;
        wr->getPostFxInternalResolution(result.x, result.y);
        return result;
      }
      return IPoint2::ZERO;
    }
    IPoint2 getDisplayResolution() const override
    {
      if (auto *wr = get_wr())
      {
        IPoint2 result;
        wr->getDisplayResolution(result.x, result.y);
        return result;
      }
      return IPoint2::ZERO;
    }
    bool hasMinimumRenderFeatures() const override { return true; }
    bool isTiledRender() const override { return false; }
    bool useMobileCodepath() const override { return false; }
    bool isRayTracingEnabled() const override { return false; }
    int getTargetFormat() const override { return 0; }
    int getHangarPassValue() const override { return 0; }
    SGSR2Interface *createSGSR2(const IPoint2 &, const IPoint2 &) const override { return nullptr; };
    bool isVrHmdEnabled() const override { return false; }
    bool pushSSAAoption() const override { return true; }

  private:
    static WorldRenderer *get_wr() { return static_cast<WorldRenderer *>(get_world_renderer()); }
  };

  void setResolution() override;
  void setAntialiasing();
  void applyAutoResolution();
  void close();
  bool haveVolumeLights() { return volumeLight != nullptr; }
  void enableVolumeFogOptionalShader(const String &shader_name, bool enable);
  void enableEnviCoverOptionalShader(const String &shader_name, bool enable);
  // void initGIWalls(eastl::unique_ptr<scene::TiledScene> &&walls);
  void initGIWindows(eastl::unique_ptr<scene::TiledScene> &&windows);
  void initRestrictionBoxes(eastl::unique_ptr<scene::TiledScene> &&walls);
  void invalidateGI(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx) override;
  void invalidateVolumeLight();

  void cullFrustumLights(Occlusion *occlusion, vec3f viewPos, mat44f_cref globtm, mat44f_cref view, mat44f_cref proj, float zn);
  void getMaxPossibleRenderingResolution(int &width, int &height) const;
  void getRenderingResolution(int &w, int &h) const override;
  void getPostFxInternalResolution(int &w, int &h) const override;
  void getDisplayResolution(int &w, int &h) const override;
  void overrideResolution(IPoint2 res) override;
  void resetResolutionOverride() override;
  int getSubPixels() const;
  int getSuperPixels() const;
  BBox3 getWorldBBox3() const;
  bool isUpsampling() const;
  bool isUpsamplingBeforePostfx() const;
  bool isGeneratingFrames() const;
  void printResolutionScaleInfo() const;
  void forceStaticResolutionScaleOff(const bool force);
  void getMinMaxZ(float &minHt, float &maxHt) const;
  float getMipmapBias() const; // Returns mipmap bias per pixel. subPixels and superPixels factors don't apply.

  enum class GiAlgorithm
  {
    OFF,
    LOW,
    MEDIUM,
    HIGH
  };
  struct GiQuality
  {
    GiAlgorithm algorithm;
    float algorithmQuality;
  };
  GiQuality getGiQuality() const;
  bool isThinGBuffer() const;
  bool isDepthAccessible() const { return true; }
  bool isSSREnabled() const;
  bool isWaterSSREnabled() const;
  bool isLowResLUT() const;

  bool hasSSRAlternateReflections() const { return ssrWantsAlternateReflections; }
  bool hasWaterSSRAlternateReflections() const;

  void invalidateAllShadows()
  {
    OSSpinlockScopedLock scopedLock{lights.lightLock};
    return lights.invalidateAllShadows();
  }

  void setPostFx(const ecs::Object &postFx);
  void postFxTonemapperChanged();

  virtual bool getBoxAround(const Point3 &position, TMatrix &box) const override;

  SkiesData *getMainSkiesData() { return main_pov_data; }

  void loadFogNodes(const String &graph_name, float low_range, float high_range, float low_height, float high_height);
  void loadEnviCoverNodes(const String &graph_name);

  void changePreset();
  FeatureRenderFlagMask getOverridenRenderFeatures();
  FeatureRenderFlagMask getPresetFeatures();
  void setFeatureFromSettings(const FeatureRenderFlags f);
  void setChromaticAberrationFromSettings();
  void setFilmGrainFromSettings();
  void toggleFeatures(const FeatureRenderFlagMask &f, bool turn_on);
  void changeFeatures(const FeatureRenderFlagMask &f);
  void validateFeatures(FeatureRenderFlagMask &features, const FeatureRenderFlagMask &changed_features);

  bool forceEnableMotionVectors() const;
  bool needMotionVectors() const;
  bool needSeparatedUI() const override;
  bool needUIBlendingForScreenshot() const override;

  void removePuddlesInCrater(const Point3 &pos, float radius);
  void delayedInvalidateAfterHeightmapChange(const BBox3 &box);

  virtual void setWorldBBox(const BBox3 &bbox) override
  {
    worldBBox = bbox;
    additionalBBox.setempty();
  }

  bool getLandHeight(const Point2 &p, float &ht, Point3 *normal) const;

  bool getIgnoreStaticShadowsForGI() const { return ignoreStaticShadowsForGI; }
  void setIgnoreStaticShadowsForGI(bool value) { ignoreStaticShadowsForGI = value; }

  PartitionSphere getTransparentPartitionSphere() const { return transparentPartitionSphere; }

protected:
  void ctorCommon();
  void ctorDeferred();

  void initResetable();
  void closeResetable();
  friend class RendererConsole;
  friend class RandomGrassRenderHelper;
  friend class LandmeshCMRenderer;
  friend struct HmapPatchesCallback;
  friend bool fog_shader_compiler(
    uint32_t variant_id, const String &name, const String &code, const DataBlock &shader_blk, String &out_errors);

  void setDirToSun();
  void updateSky(float realDt);

  void initOverrideStates();
  void closeOverrideStates();

  ///*
  void closeGround();

  void closeClipmap();
  void initClipmap();
  void invalidateClipmap(bool force);
  void dumpClipmap();
  void prepareLastClip();
  bool useClipmapFeedbackOversampling();

  void initRendinstVisibility();
  void closeRendinstVisibility();
  friend struct VisibilityPrepareJob;
  friend struct GroundCullingJob;
  friend struct GroundReflectionCullingJob;
  friend struct LightProbeVisibilityJob;
  // starts async culling
  void startVisibility();
  // we have to call it after all other ground renders are complete
  void startGroundVisibility();
  // we have to call it after all other ground renders are complete
  void startGroundReflectionVisibility();
  // should be called after startVisibility
  void startLightsCullingJob();
  void waitAllJobs();

  // optional heightmap
  void updateDistantHeightmap();
  void closeDistantHeightmap();

  // water
  void renderWaterHeightmapLowres();

  // shadows
  void changeStateFOMShadows();
  void performVolfogMediaInjection();
  // shadows

  void initDisplacement();
  void initHmapPatches(int texSize);
  void renderDisplacementAndHmapPatches(const Point3 &origin, float distance);
  void renderDisplacementRiLandclasses(const Point3 &camera_pos);

  void renderGround(const LandMeshCullingData &lmesh_culling_data, bool is_first_iter);

  void renderStaticSceneOpaque(int shadow_cascade, const Point3 &camera_pos, const TMatrix &view_itm, const Frustum &culling_frustum);
  void renderDynamicOpaque(
    int shadow_cascade, const TMatrix &view_itm, const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point3 &cam_pos);

  void renderRendinst(int shadow_cascade, const TMatrix &view_itm);
  void renderRITreePrepass(const TMatrix &view_itm);
  void renderFullresRITreePrepass(const TMatrix &view_itm);
  void renderRITree();
  void renderFullresRITree();
  void setRendinstTesselation();
  void setSharpeningFromSettings();

  void initPortalRendererCallbacks();

  struct DelayedRenderContext
  {
    struct //-V730
    {
      bool render = false;
      const ManagedTex *texPtr = nullptr;
      Point3 position;
      int faceNumber;
    } lightProbeData;

    struct
    {
      bool render = false;
      TMatrix itm;
    } depthAOAboveData;

    void invalidate()
    {
      lightProbeData.render = false;
      lightProbeData.texPtr = nullptr;
      depthAOAboveData.render = false;
    }
  } delayedRenderCtx;

  bool prepareDelayedRender(const TMatrix &itm); // Return true if some tp jobs were added

public:
  void renderDelayedCube();
  void renderDelayedDepthAbove();

protected:
  struct VehicleCockpitOcclusionData
  {
    mat44f curTm = {};
    mat44f prevTm = {};
    float cockpitDistance = 0.f;
    bool occlusionAvailable = false;
  } vehicleCockpitOcclusionData;
  void copyClipmapUAVFeedback();

  void onBareMinimumSettingsChanged();

  void updateTransformations(const DPoint3 &move,
    const TMatrix4_vec4 &jittered_cam_pos_to_unjittered_history_clip,
    const TMatrix4_vec4 &prev_origo_relative_view_proj_tm);
  void prepareClipmap(const Point3 &origin, const TMatrix &view_itm, const TMatrix4 &globtm, float wk, int minimum_zoom);
  void prepareDeformHmap();
  enum class DistantWater : bool
  {
    No,
    Yes
  };
  void renderWater(const CameraParams &camera, DistantWater render_distant_water, bool render_ssr);
  void renderWaterSSR(const TMatrix &itm, const Driver3dPerspective &persp);

  void generatePaintingTexture();
  void prefetchPartsOfPaintingTexture();
  SharedTex localPaintTex, globalPaintTex;
  TEXTUREID localPaintTexId = BAD_TEXTUREID; // ID stored like this to prefetch it again after device reset

  enum EnviProbeRenderFlags
  {
    ENVI_PROBE_USE_GEOMETRY = 0x1,
    ENVI_PROBE_SUN_ENABLED = 0x2
  };
  uint32_t getEnviProbeRenderFlags() const;

  bool enviProbeInvalid = false;
  bool specularCubesContainerNeedsReinit = false;
  inline void setSpecularCubesContainerForReinit() { specularCubesContainerNeedsReinit = true; }
  inline void invalidateCube()
  {
    enviProbeInvalid = true;
    setSpecularCubesContainerForReinit();
  }
  void reinitCubeIfInvalid();
  void reinitSpecularCubesContainerIfNeeded(int cube_size = -1);

  uint32_t cubeResolution = 128;
  void reinitCube(int ew, const Point3 &at);
  void reinitCube(const Point3 &at);
  inline void reinitCube() { reinitCube(enviProbePos); }

  bool enviProbeNeedsReload = false;
  void reloadCube(bool first);

  void updateSkyProbeDiffuse();
  uint32_t attemptsToUpdateSkySph = 0;
  void setEnviProbePos(const Point3 &at) { enviProbePos = at; }

  String showTexCommand(const char *argv[], int argc);
  String showTonemapCommand(const char *argv[], int argc);
  void createDebugBoxRender();
  String showBoxesCommand(const char *argv[], int argc);
  void setRIVerifyDistance(float);

  void loadLandMicroDetails(const DataBlock *blk);
  void closeLandMicroDetails();
  void loadCharacterMicroDetails(const DataBlock *blk);
  void closeCharacterMicroDetails();

  void applySettingsChanged();

  bool hasPendingFgRecreation = false;
  void processPendingFgRecreation();
  void requestFgRecreation(const char *tag)
  {
    debug("WR:mark FG for recompilation [%s]", tag);
    hasPendingFgRecreation = true;
  }

  IPoint2 getFsrScaledResolution(const IPoint2 &orig_resolution) const;
  void updateLevelGraphicsSettings(const DataBlock &level_blk);
  bool forceStaticResolutionOff = false;

  void invalidateLightProbes();
  void initIndoorProbesIfNecessary();

protected:
  friend struct PostFxManager;
  PostFxManager postfx;

  void beforeDrawPostFx();
  void initPostFx();
  void setPostFxResolution(int w, int h);

  void closePostFx() { postfx.close(); }

  UniqueTex uiBlurFallbackTexId;
  void initUiBlurFallback();

  void setDeformsOrigin(const Point3 &);
  void createDeforms();
  void closeDeforms();

  eastl::unique_ptr<FomShadowsManager> fomShadowManager;

  GpuDeformObjectsManager gpuDeformObjectsManager;

  eastl::unique_ptr<RenderDynamicCube> cube;

protected:
  light_probe::Cube *enviProbe = nullptr;
  Point3 enviProbePos = {0, 0, 0};
  eastl::unique_ptr<LightProbeSpecularCubesContainer> specularCubesContainer;
  eastl::unique_ptr<IndoorProbeNodes> indoorProbeNodes;
  eastl::unique_ptr<IndoorProbeManager> indoorProbeMgr;
  eastl::unique_ptr<IndoorProbeScenes> indoorProbesScene;
  eastl::unique_ptr<scene::TiledScene> restrictionBoxesScene;
  eastl::unique_ptr<DataBlock> levelSettings;

  void toggleProbeReflectionQuality();
  bool hqProbesReflection = true;
  shaders::UniqueOverrideStateId nocullState;

  shaders::UniqueOverrideStateId depthClipState;
  shaders::UniqueOverrideStateId flipCullStateId;
  shaders::UniqueOverrideStateId zFuncAlwaysStateId;
  shaders::UniqueOverrideStateId additiveBlendStateId;
  shaders::UniqueOverrideStateId enabledDepthBoundsId;
  shaders::UniqueOverrideStateId zFuncEqualStateId;
  UniqueTexHolder last_clip;
  d3d::SamplerInfo last_clip_sampler;

  float glassShadowK = 0.7;

  float cachedStaticResolutionScale = 100.f;
  UniqueTexHolder distantHeightmapTex;
  UniqueTexHolder paintColorsTex;

  TEXTUREID landMicrodetailsId = BAD_TEXTUREID, characterMicrodetailsId = BAD_TEXTUREID;

  BaseStreamingSceneHolder *binScene = nullptr; // owned by corresponding entity
  BBox3 binSceneBbox;

  TEXTUREID lightmapTexId = BAD_TEXTUREID;
  LandMeshManager *lmeshMgr = nullptr; // not owned!
  LandMeshRenderer *lmeshRenderer = nullptr;
  int displacementSubDiv = 2;
  int subdivSettings = 0;
  void initSubDivSettings();
  float cameraHeight = 0.f;
  float cameraActualHeight = 0.f;
  float lodDistanceScaleBase = 1.3f;

  // queried in every frame in before draw
  bool canChangeAltitudeUnexpectedly = false;
  bool useFullresClouds = false;
  bool waterReflectionEnabled = true;

  Color3 sun;
  struct DirToSun
  {
    Point3 prev = Point3(0, -1, 0);
    Point3 curr = Point3(0, -1, 0);
    Point3 realTime = Point3(0, -1, 0);
    enum SunDirectionUpdateStage
    {
      NO_UPDATE,
      RENDINST_GLOBAL_SHADOWS,
      STATIC_SHADOW,
      RELOAD_CUBE,
      DUMMY, // Because reloading cube updates sky too...
      IMPOSTOR_PRESHADOWS,
      FINISHED
    };
    int sunDirectionUpdateStage = NO_UPDATE;
    bool hasPendingForceUpdate = false;
  } dir_to_sun;
  carray<Color4, 7> sphValues;

  PartitionSphere transparentPartitionSphere;

  carray<RiGenVisibility *, ShadowsManager::CSM_MAX_CASCADES> rendinst_shadows_visibility = {};
  RiGenVisibility *rendinst_dynamic_shadow_visibility = nullptr;


  RiGenVisibility *rendinst_cube_visibility = nullptr;

  DataBlock originalLevelBlk;
  void resetVolumeLights();
  eastl::unique_ptr<VolumeLight> volumeLight;
  float volumeLightLowRange = 256, volumeLightHighRange = 512, volumeLightLowHeight = 8, volumeLightHighHeight = 128;


  eastl::unique_ptr<EmissionColorMaps> emissionColorMaps;

  bool storeDownsampledTexturesInEsram = false;
  uint32_t downsampledTexturesMipCount = 1;

  struct Afr
  {
    TMatrix4 prevGlobTm;
    Point4 prevViewVecLT;
    Point4 prevViewVecRT;
    Point4 prevViewVecLB;
    Point4 prevViewVecRB;
    DPoint3 prevWorldPos;
  };
  Afr afr;

  int water_ssr_id = -1;

  eastl::unique_ptr<DynamicQuality> dynamicQuality;
  void resetDynamicQuality();

  PostFxRenderer copyDepth;
  eastl::unique_ptr<DynamicResolution> dynamicResolution;
  // Unlike setResolution() these functions don't recreate all textures, only switch aliases.
  // But it work only when d3d::alias_tex() is supported and new resolution can't be greater then original.
  // Also we don't change resolution for both paired texture, only for one that will be written this
  // frame, because another is used for read. But we change resolution for another in next frame anyway.
  void changeSingleTexturesResolution(int width, int height);

public:
  int getDynamicResolutionTargetFps() const;
  eastl::unique_ptr<EnviCover> enviCover;

protected:
  ResizableRTargetPool::Ptr renderFramePool;
  const ManagedTex *finalTargetFrame = nullptr;
  ExternalTex backbufferTex;
  UniqueTex ownedBackbufferTex;

  UniqueBuf superResolutionUpscalingConsts;
  UniqueBuf superResolutionSharpeningConsts;
  float fsrScale = 1.0f;
  float fsrMipBias = 0.0f;
  bool fsrEnabled = false;

  bool overrideDisplayRes = false;
  IPoint2 overridenDisplayRes = {};

  enum class RtControl
  {
    BACKBUFFER,
    RT,
    OWNED_RT
  } rtControl{RtControl::OWNED_RT};
  void updateBackBufferTex();
  void resetBackBufferTex();
  void setFinalTargetTex(const ManagedTex *tex, const char *ctx_mark = "");

  bool isFXAAEnabled();

  enum ResolutionScaleMode
  {
    RESOLUTION_SCALE_SUB,
    RESOLUTION_SCALE_NATIVE,
    RESOLUTION_SCALE_POSTFX
  };
  ResolutionScaleMode resolutionScaleMode = RESOLUTION_SCALE_NATIVE;
  IPoint2 getStaticScaledResolution(const IPoint2 &orig_resolution) const;
  void initStaticUpsample(const IPoint2 &origResolution, const IPoint2 &scaledResolution);
  void closeStaticUpsample();
  bool isStaticUpsampleEnabled() const;
  void applyStaticUpsampleQuality();

  AntiAliasingAppGlue aaGlue;
  AntiAliasingMode currentAntiAliasingMode = AntiAliasingMode::TSR;
  eastl::unique_ptr<AntiAliasing> antiAliasing;
#if !_TARGET_PC && !_TARGET_ANDROID && !_TARGET_IOS && !_TARGET_C3
  IPoint2 fixedPostfxResolution;
  IPoint2 getFixedPostfxResolution(const IPoint2 &orig_resolution) const;
#endif

  void loadAntiAliasingSettings();
  void applyFXAASettings();
  void loadFsrSettings();
  bool isFsrEnabled() const;
  bool hasMotionVectors = false;
  constexpr static int GBUF_TARGET_GLOBAL_FLAGS = TEXCF_ESRAM_ONLY;
  void initTarget();
  bool isEnviCoverCompatible = false;
  void updateEnviCoverCompatibility();
  void initGbufferDepthProducer();
  void toggleMotionVectors();

  dafg::NodeHandle ssaaNode;
  float ssaaMipBias = 0.0f;
  bool isSSAAEnabled() const { return currentAntiAliasingMode == AntiAliasingMode::SSAA; }
  IPoint2 getSSAAResolution(const IPoint2 &orig_resolution) const { return orig_resolution * 2; }

  void setSettingsSSR();
  void updateSettingsSSR(int w, int h);

  enum class AoQuality
  {
    LOW,
    MEDIUM,
    HIGH
  };
  AoQuality aoQuality = AoQuality::MEDIUM;
  void resetSSAOImpl();
  FFTWater *water = nullptr;
  ShoreRenderer shoreRenderer;
  static constexpr int lowresWaterHeightmapSize = 1024;
  UniqueTexHolder lowresWaterHeightmap;
  PostFxRenderer lowresWaterHeightmapRenderer;

  int fftWaterQualitySetting = 0;
  PostFxRenderer waterDistant[2];
  SkiesData *main_pov_data = nullptr;
  SkiesData *cube_pov_data = nullptr;

  SkiesData *refl_pov_data = nullptr;

  UniqueTexHolder cloudsVolume;

  eastl::unique_ptr<GaussMipRenderer> planarReflectionMipRenderer;
  TMatrix waterPlanarReflectionViewItm;
  TMatrix waterPlanarReflectionViewTm;
  TMatrix4 waterPlanarReflectionProjTm;
  Driver3dPerspective waterPlanarReflectionPersp;
  dafg::NodeHandle waterPlanarReflectionCloudsNode;
  dafg::NodeHandle waterPlanarReflectionTerrainNode;

  void initWaterPlanarReflection();
  void closeWaterPlanarReflection();
  void initWaterPlanarReflectionTerrainNode();
  bool isWaterPlanarReflectionTerrainEnabled() const;
  void calcWaterPlanarReflectionMatrix();
  void renderLmeshReflection(const LandMeshCullingData &lmesh_refl_culling_data);
  void recreateCloudsVolume();

public:
  CollisionResource *getStaticSceneCollisionResource() const { return staticSceneCollisionResource.get(); }
  eastl::unique_ptr<DeferredRT> target;
  void setupSkyPanoramaAndReflectionFromSetting();
  float daGdpRangeScale = 1;
  float gameTime = 0.f;
  float realDeltaTime = 0.f;
  uint32_t renderedFrames = 0;
  Clipmap *getClipmap() const { return clipmap; }
  bool getBareMinimumPreset() const { return bareMinimumPreset; }
  AntiAliasingMode getAntiAliasingMode() { return currentAntiAliasingMode; }
  DaGI *getGI() override;
  uint32_t getGIHistoryFrames() const;
  void setGILightsToShader(bool allow_frustum_lights);

private:
  void setupSkyPanoramaAndReflection(bool use_panorama);
  void switchVolumetricAndPanoramicClouds(); // for debugging

  UniqueTex screenshotSuperFrame;
  PostFxRenderer underWater;
  float cameraSpeed = 0;

  Clipmap *clipmap = nullptr;
  int clipmapCutoffZoom = -1, clipmapCutoffMipsCnt = -1;

  eastl::unique_ptr<MultiFramePGF> preIntegratedGF;

  UniqueTexHolder heightmapAround;
  UniqueTexHolder hmapPatchesDepthTex;
  UniqueTexHolder hmapPatchesTex;
  bool hmapPatchesEnabled = false;
  PostFxRenderer processHmapPatchesDepth;
  ToroidalHelper displacementData;
  Tab<BBox2> displacementInvalidationBoxes;
  ToroidalHelper hmapPatchesData;
  RiGenVisibility *rendinstHmapPatchesVisibility = nullptr;

  Tab<BBox3> invalidationBoxesAfterHeightmapChange;
  int hmapTerrainStateVersion = -1;
  void invalidateAfterHeightmapChange();

  // TODO: separate it from WR
  UniqueTexHolder riLandclassDepthTextureArr;
  dag::Vector<int> riLandclassIndices;
  dag::Vector<int> riLandclassIndicesPrev;
  dag::Vector<ToroidalHelper> riLandclassDisplacementDataArr;

  eastl::unique_ptr<DeformHeightmap> deformHmap;
  void initDeformHeightmap();
  void closeDeformHeightmap();

  bool tiledRenderArch = false;
  bool bareMinimumPreset = false;
  void updateImpostorSettings();

  eastl::unique_ptr<TreesAboveDepth> treesAbove;
  rendinst::RIOcclusionData *riOcclusionData = nullptr;

  DebugTexOverlay *debug_tex_overlay = nullptr;
  void createDebugTexOverlay(const int w, const int h);
  eastl::unique_ptr<DebugTonemapOverlay> debug_tonemap_overlay;
  float waterLevel = 0;

  bool applySettingsAfterResetDevice = false;
  bool initClipmapAfterResetDevice = false;

  bool ignoreStaticShadowsForGI = false;
  bool globalGiLightsPrepared = false;

  void renderDebugCollisionDensity();
  eastl::unique_ptr<DebugCollisionDensityRenderer> debugCollisionDensityRenderer;

  eastl::unique_ptr<DepthAOAboveContext> depthAOAboveCtx;
  ClusteredLights lights;
  eastl::unique_ptr<DynamicShadowRenderExtender> shadowRenderExtender;
  PostFxRenderer debugFillGbufferShader;
  shaders::UniqueOverrideStateId specularOverride, diffuseOverride;

  void changeGrassShadowQuality();
  int getDynamicShadowQuality();
  void changeDynamicShadowResolution();
  void setDynamicShadowsMaxUpdatePerFrame();

  // IShadowInfoProvider -- callbacks for shadowsManager
  const CameraParams &getCurrentFrameCameraParams() const override { return currentFrameCamera; }
  Point3 getDirToSun(DirToSun::SunDirectionUpdateStage stage) const
  {
    if (stage >= DirToSun::FINISHED)
      return dir_to_sun.realTime;
    if (stage <= dir_to_sun.sunDirectionUpdateStage)
      return dir_to_sun.curr;
    else
      return dir_to_sun.prev;
  }
  Point3 getDirToSun(DirToSunType type) const override
  {
    return getDirToSun(type == DirToSunType::CSM ? DirToSun::FINISHED : DirToSun::STATIC_SHADOW);
  }
  void dirToSunFlush()
  {
    dir_to_sun.sunDirectionUpdateStage = DirToSun::NO_UPDATE;
    dir_to_sun.hasPendingForceUpdate = true;
    dir_to_sun.prev = dir_to_sun.curr = dir_to_sun.realTime;
  }
  eastl::span<RiGenVisibility *> getRendinstShadowVisibilities() override
  {
    return {rendinst_shadows_visibility.data(), rendinst_shadows_visibility.size()};
  }
  bool hasRenderFeature(FeatureRenderFlags f) const override { return hasFeature(f); }
  BBox3 getStaticShadowsBBox() const override
  {
    BBox3 res = getWorldBBox3();
    getMinMaxZ(res[0].y, res[1].y);
    return res;
  }
  float getWaterLevel() const override { return waterLevel; }
  float getCameraHeight() const override { return cameraHeight; }
  const DataBlock *getLevelSettings() const override { return levelSettings.get(); }
  int getShadowFramesCount() const override;         // will return 8 if no temporality available
  int getTemporalShadowFramesCount() const override; // will return 1 if no temporality available
  bool timeDynamic = false;
  bool isTimeDynamic() const override;

  void renderStaticSceneForShadowPass(
    int cascade, const Point3 &camera_pos, const TMatrix &view_itm, const Frustum &culling_frustum) override
  {
    renderStaticSceneOpaque(cascade, camera_pos, view_itm, culling_frustum);
  }
  void renderDynamicsForShadowPass(int cascade, const TMatrix &itm, const Point3 &cam_pos) override;

  // Note: this has to be initialized after ClusteredLights as it
  // depends on it.
  ShadowsManager shadowsManager{*this, lights};

  bool legacyGPUObjectsVisibilityInitialized = false;
  void clearLegacyGPUObjectsVisibility();
  RiGenVisibility *rendinst_trees_visibility = nullptr;
  DaGI *daGI2 = 0;
  eastl::unique_ptr<GIWindows> giWindows;
  float giDynamicQuality = 1;
  LRUCollisionVoxelization *voxelizeCollision = nullptr;
  int giUpdatePosFrameCounter = 0, giInvalidateDeferred = 0;
  float giHmapOffsetFromLow = 8.0, giHmapOffsetFromUp = 32 - 8.0f;
  float giIntersectedAmount = 0.125f, giNonIntersectedAmount = 0.125f;
  float giMaxUpdateHeightAboveFloor = 32.0f;
  BBox3 giCurrentGlobalLightsBox;
  bool giGlobalLightsPrepared = false;
  RiGenVisibility *rendinst_voxelize_visibility = nullptr;
  eastl::unique_ptr<LRURendinstCollision> lruCollision;
  eastl::unique_ptr<DaGISettings> giSettingsOverride;

  PostFxRenderer ambientBilateralBlur, ambientPerPixelVoxelsRT;
  eastl::unique_ptr<DataBlock> giBlk;
  void initGI();
  void closeGI();
  void resetGI();
  void giBeforeRender();
  void renderGiCollision(const TMatrix &view_itm, const Frustum &);
  void updateGIPos(const Point3 &pos, const TMatrix &view_itm, float hmin, float hmax);
  void drawGIDebug(const Frustum &camera_frustum);
  void invalidateGI(bool force);
  void doInvalidateGI(const bool force);
  void doInvalidateGI(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx);
  void setGIQualityFromSettings();
  bool giNeedsReprojection();
  void overrideGISettings(const DaGISettings &settings) override;
  void resetGISettingsOverride() override;
  DaGISettings getGISettings() const;

  void initFsr(const IPoint2 &postfx_resolution, const IPoint2 &display_resolution);
  void closeFsr();

  const scene::TiledScene *getWallsScene() const;
  const scene::TiledScene *getWindowsScene() const;
  const scene::TiledScene *getEnviProbeBoxesScene() const;
  const scene::TiledScene *getRestrictionBoxScene() const;

  void prerunFx();
  FxRenderTargetOverride fxRtOverride = FX_RT_OVERRIDE_DEFAULT;
  void setFxQuality();

  void setEnviromentDetailsQuality();
  void setNbsQuality();

  void resetLatencyMode();
  void resetVsyncMode();
  void resetPerformanceMetrics();

  void updateHeroData();

  HeroWtmAndBox heroData;
  CameraViewVisibilityMgr mainCameraVisibilityMgr{RI_EXTRA_VB_CTX_ASYNC, "main_cam"};
  CameraViewVisibilityMgr camcamVisibilityMgr{RI_EXTRA_VB_CTX_CAMCAM_ASYNC, "cam_in_cam", COCKPIT_NO_REPROJECT};
  eastl::optional<CameraParams> camcamParams;
  eastl::optional<CameraParams> prevCamcamParams;
  CameraParams currentFrameCamera;
  CameraParams prevFrameCamera;
  TexStreamingContext currentTexCtx = TexStreamingContext(0);

public:
  RiGenVisibility *getMainCameraRiMainVisibility() { return mainCameraVisibilityMgr.getRiMainVisibility(); }
  Occlusion *getMainCameraOcclusion();
  void waitOcclusionAsyncReadbackGpu();
  const TMatrix &getCameraViewItm() const { return currentFrameCamera.viewItm; }
  const TMatrix4_vec4 &getCameraProjtm() const { return currentFrameCamera.noJitterProjTm; }
  TexStreamingContext getTexCtx() const { return currentTexCtx; }
  void setCockpitReprojectionMode(CockpitReprojectionMode mode) { mainCameraVisibilityMgr.setCockpitReprojectionMode(mode); }

private:
  int maxWaterTessellation = 7;
  void setupWaterQuality();

  bool isDebugLogFrameTiming = false;
  void debugLogFrameTiming();

  static eastl::optional<IPoint2> getDimsForVrsTexture(int screenWidth, int screenHeight);

private:
  void setSkies(DaSkies *skies);
  void resetMainSkies(DaSkies *skies);

  bool shouldToggleVRS(const AimRenderingData &aim_data);

  eastl::unique_ptr<CollisionResource> staticSceneCollisionResource;

  BBox3 worldBBox;
  BBox3 additionalBBox;

  SatelliteRenderer satelliteRenderer;

  eastl::vector<dafg::NodeHandle> fgNodeHandles;
  eastl::fixed_vector<dafg::NodeHandle, 4> vrsNodeHandles;
  eastl::vector<resource_slot::NodeHandleWithSlotsAccess> resSlotHandles;
  dafg::NodeHandle prepareGbufferDepthFGNode;
  dafg::NodeHandle prepareGbufferFGNode;

  eastl::fixed_vector<dafg::NodeHandle, 4> finalOpaqueControlNodes;

  eastl::fixed_vector<dafg::NodeHandle, 3> ssrFGNodes;
  eastl::array<dafg::NodeHandle, 3> aoFGNodes;
  dafg::NodeHandle giRenderFGNode;

  dafg::NodeHandle giCalcFGNode;
  dafg::NodeHandle giFeedbackFGNode;
  dafg::NodeHandle giScreenDebugFGNode;
  dafg::NodeHandle giScreenDebugDepthFGNode;

  eastl::fixed_vector<dafg::NodeHandle, 5, false> subSuperSamplingNodes;
  dafg::NodeHandle frameToPresentProducerNode;
  eastl::array<dafg::NodeHandle, 2> externalFinalFrameControlNodes;
  dafg::NodeHandle distortionFxNode;
  dafg::NodeHandle postfxTargetProducerNode;
  dafg::NodeHandle prepareForPostfxNoAANode;
  dafg::NodeHandle motionBlurAccumulateNode;
  dafg::NodeHandle motionBlurApplyNode;
  MotionBlurNodeStatus motionBlurStatus = MotionBlurNodeStatus::UNINITIALIZED;
  dafg::NodeHandle fxaaNode;

  int superPixels = 1;
  int subPixels = 1;
  bool requiresSubsampling = false;
  bool requiresSupersampling = false;
  bool ssrWantsAlternateReflections = false;

private:
  bool needTransparentDepthAOAbove = false;
  int transparentDepthAOAboveRequests = 0;
  bool isEnviCover = false;

public:
  void requestDepthAboveRenderTransparent();
  void revokeDepthAboveRenderTransparent();
  void setEnviCover(bool envi_cover);

private:
  UniqueTex hzbUploadStagingTex;

public:
  BaseTexture *initAndGetHZBUploadStagingTex();
};

float get_fpv_shadow_dist();
void set_up_omni_lights();
