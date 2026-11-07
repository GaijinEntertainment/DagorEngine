//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_functionRef.h>
#include <generic/dag_tabFwd.h>
#include <EditorCore/ec_interface_ex.h>

class DataBlock;
class ViewportWindow;
class DynamicRenderableSceneInstance;
class Point3;
class TexStreamingContext;
class TMatrix;
struct DemonPostFxSettings;
struct Color4;


class IDynRenderService
{
public:
  static constexpr unsigned HUID = 0xE9011AA3u; // IDynRenderService

  enum
  {
    RTYPE_CLASSIC,
    RTYPE_DYNAMIC_A,
    RTYPE_DYNAMIC_DEFERRED,
    RTYPE_DNG_BASED,
  };

  enum
  {
    ROPT_SHADOWS,
    ROPT_SHADOWS_VSM,
    ROPT_SHADOWS_FOM,
    ROPT_WATER_REFL,
    ROPT_ENVI_ORDER,
    ROPT_SSAO,
    ROPT_SSR,
    ROPT_NO_SUN,
    ROPT_NO_AMB,
    ROPT_NO_ENV_REFL,
    ROPT_NO_TRANSP,
    ROPT_NO_POSTFX,

    ROPT_COUNT
  };
  using Stage = IRenderingService::Stage;

  virtual void setup(const char *app_dir, const DataBlock &app_blk) = 0;

  virtual void init() = 0;
  virtual void term() = 0;

  virtual void enableRender(bool enable) = 0;

  // Suppress presenting the scene's rendered frame to the screen, unless a modal dialog needs to be shown.
  virtual void suppressScenePresent(bool suppress) = 0;

  // Set the service's scene as the current game scene (with dagor_select_game_scene()).
  virtual void selectAsGameScene() = 0;

  virtual void setEnvironmentSnapshot(const char *blk_fn, bool render_cubetex_from_snapshot = false) = 0;
  virtual bool hasEnvironmentSnapshot() = 0;

  virtual void restartPostfx(const DataBlock &game_params) = 0;
  virtual void onLightingSettingsChanged() = 0;

  virtual void reloadCharacterMicroDetails(const DataBlock &deferred_blk, const DataBlock &level_blk) = 0;

  virtual TexStreamingContext getMainViewStreamingContext() = 0;
  virtual void renderViewportFrame(ViewportWindow *vpw) = 0;
  virtual void renderScreenshot(const Driver3dPerspective &persp, bool is_ortho, bool force_disable_wireframe) = 0;

  virtual dag::ConstSpan<int> getSupportedRenderTypes() const = 0;
  virtual int getRenderType() const = 0;
  virtual bool setRenderType(int t) = 0;

  virtual dag::ConstSpan<const char *> getDebugShowTypeNames() const = 0;
  virtual int getDebugShowType() const = 0;
  virtual bool setDebugShowType(int t) = 0;

  virtual bool getRenderOptSupported(int ropt) = 0;
  virtual const char *getRenderOptName(int ropt) = 0;
  virtual void setRenderOptEnabled(int ropt, bool enable) = 0;
  virtual bool getRenderOptEnabled(int ropt) = 0;

  virtual dag::ConstSpan<const char *> getShadowQualityNames() const = 0;
  virtual void setShadowQuality(int q) = 0;
  virtual int getShadowQuality() const = 0;

  virtual bool hasExposure() const = 0;
  virtual void setExposure(float exposure) = 0;
  virtual float getExposure() = 0;
  virtual float getEffectiveExposure() { return getExposure(); }

  virtual void getPostFxSettings(DemonPostFxSettings &set) = 0;
  virtual void setPostFxSettings(DemonPostFxSettings &set) = 0;

  virtual void beforeD3DReset(bool full_reset) = 0;
  virtual void afterD3DReset(bool full_reset) = 0;

  virtual BaseTexture *getRenderBuffer() = 0;
  virtual D3DRESID getRenderBufferId() = 0;

  virtual BaseTexture *getDepthBuffer() = 0;
  virtual D3DRESID getDepthBufferId() = 0;
  virtual const ManagedTex &getDownsampledFarDepth() = 0;
  virtual const ManagedTex &getPrevDownsampledFarDepth() = 0;

  virtual void toggleVrMode() = 0;

  virtual void renderOneDynModelInstance(DynamicRenderableSceneInstance *sceneInstance, Stage stage, int *optional_inst_seed = nullptr,
    bool raw_render = false) = 0;

  // False when renderIsolated() returns empty for every call.
  virtual bool canRenderIsolated() const { return false; }

  // Returns a new A8R8G8B8 texture with only what render_opaque draws into the bound gbuffer, and alpha 1 only where it
  // drew. The texture is sRGB encoded, so read it as display ready colors.
  // The scene sun and environment light the drawn objects, with no SSAO or SSR. Shadows, exposure and tonemap depend on
  // the renderer.
  // Returns empty when not supported. A renderer can also return empty while the scene is not ready to render (see
  // isEnvironmentLightingReady()).
  // render_opaque adds world_offset to each position it draws: a renderer can move the render away from the scene, so
  // that its world space data (for example static shadows) does not show on it.
  virtual TexPtr renderIsolated(int width, int height, const TMatrix &view_itm, const Driver3dPerspective &persp,
    dag::FunctionRef<void(const Point3 &world_offset)> render_opaque) = 0;

  // Returns false while the scene is not ready to render or the environment light is not up to date, for example
  // shortly after the project load. A render made then is empty or has a wrong ambient light.
  virtual bool isEnvironmentLightingReady() { return true; }

  virtual void invalidateEditorClipmap() {}
  virtual void updateEditorLandmesh() {}
  virtual void setEditorHmapMirroring(bool) {}

  virtual void onGrassCreated(const DataBlock *) {}

  virtual bool isThermalVisionActive() const { return false; }

  virtual void beforeProjectSceneReset() {}
};

class IRenderHelperService
{
public:
  static constexpr unsigned HUID = 0xF75DBE09u; // IRenderHelperService

  virtual void init() = 0;
  virtual void term() = 0;

  virtual void enableFrameProfiler(bool enable) = 0;
  virtual void profilerDumpFrame() = 0;
  virtual void dumpPendingProfilerDumpOnFrameEnd() = 0;

  virtual void renderEnviCubeTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd) = 0;
  virtual void renderEnviBkgTexture(BaseTexture *tex, d3d::SamplerHandle smp, const Color4 &scales) = 0;
  virtual void renderEnviVolTexture(BaseTexture *tex, d3d::SamplerHandle smp, const Color4 &cMul, const Color4 &cAdd,
    const Color4 &scales, float tz) = 0;
};
