//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_tabFwd.h>

class DataBlock;
class ViewportWindow;
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

  virtual void setup(const char *app_dir, const DataBlock &app_blk) = 0;

  virtual void init() = 0;
  virtual void term() = 0;

  virtual void enableRender(bool enable) = 0;

  virtual void setEnvironmentSnapshot(const char *blk_fn, bool render_cubetex_from_snapshot = false) = 0;
  virtual bool hasEnvironmentSnapshot() = 0;

  virtual void renderEnviCubeTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd) = 0;
  virtual void renderEnviBkgTexture(BaseTexture *tex, const Color4 &scales) = 0;
  virtual void renderEnviVolTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd, const Color4 &scales, float tz) = 0;

  virtual void restartPostfx(const DataBlock &game_params) = 0;
  virtual void onLightingSettingsChanged() = 0;

  virtual void renderViewportFrame(ViewportWindow *vpw) = 0;
  virtual void renderScreenshot() = 0;

  virtual void renderFramesToGetStableAdaptation(float max_da, int max_frames) = 0;

  virtual void enableFrameProfiler(bool enable) = 0;
  virtual void profilerDumpFrame() = 0;

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

  virtual void getPostFxSettings(DemonPostFxSettings &set) = 0;
  virtual void setPostFxSettings(DemonPostFxSettings &set) = 0;

  virtual void afterD3DReset(bool full_reset) = 0;

  virtual BaseTexture *getRenderBuffer() = 0;
  virtual D3DRESID getRenderBufferId() = 0;

  virtual BaseTexture *getDepthBuffer() = 0;
  virtual D3DRESID getDepthBufferId() = 0;
  virtual const ManagedTex &getDownsampledFarDepth() = 0;

  virtual void toggleVrMode(){};
};
