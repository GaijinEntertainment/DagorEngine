//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_eventQueryHolder.h>
#include <math/dag_e3dColor.h>
#include <math/dag_color.h>
#include <shaders/dag_shaderCommon.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <shaders/dag_postFxRenderer.h>
#include <EASTL/unique_ptr.h>
#include <render/fx/flare.h>


class DataBlock;
class Sbuffer;
class TextureIDPair;
class ComputeShaderElement;

struct DemonPostFxSettings
{
  // Color matrix
  DemonPostFxSettings()
  {
    memset(this, 0, sizeof(DemonPostFxSettings));
    debugFlags = 0;
  }
  uint32_t debugFlags;
  enum
  {
    NO_SCENE = 1 << 0,
    NO_VFOG = 1 << 1,
    NO_GLOW = 1 << 2,
    NO_STARS = 1 << 3
  };
  real hueShift;
  Color3 saturationColor;
  real saturation;
  Color3 grayColor;
  Color3 contrastColor;
  real contrast;
  Color3 contrastPivotColor;
  real contrastPivot;
  Color3 brightnessColor;
  real brightness;

  real lum_min, lum_max, lum_avg;
  real lowBrightScale; // effect of low level threshold. should be as close to 1 as possible. 0.9 - default

  // Glow
  real glowRadius;

  real hdrDarkThreshold;
  real hdrGlowPower;
  real hdrGlowMul;

  // Fog
  real volfogRange;
  real volfogMul;
  Color3 volfogColor;
  Point3 sunDir; // must be normalized
  // bool useRawSkyMask;

  real volfogFade;
  real volfogMaxAngle;

  void setSunDir(const Point3 &dir2sun) { sunDir = dir2sun; }
};

static inline Point3 dir_from_polar(float azimuth, float zenith)
{
  return Point3(cosf(azimuth) * cosf(zenith), sinf(zenith), sinf(azimuth) * cosf(zenith));
}

struct DemonPostFxCallback
{
  virtual int process(int target_size_x, int target_size_y, Color4 target_quad_coeffs) = 0;
};

class DemonPostFx
{
public:
  // Color matrices combined with parameter-based color matrix.
  // Defaults to identity.
  TMatrix postColorMatrix, preColorMatrix;

  // Show source image alpha for debugging (fake HDR mode only).
  // NOTE: Image is affected by motion blur.
  bool showAlpha;

  DemonPostFx(const DataBlock &main_blk, const DataBlock &adaptation_blk, int target_w, int target_h, unsigned rtarget_flags,
    bool initPostfxGlow = true, bool initSunVolfog = true, bool initUIBlur = true);
  ~DemonPostFx();

  //! updates settings that don't require recreate, or return false to indicated need of recreate
  bool updateSettings(const DataBlock &main_blk);

  using PreCombineFxProc = void (*)();

  TextureIDPair downsample(Texture *input_tex, TEXTUREID input_id, const Point4 &input_uv_transform = Point4(1, 1, 0, 0));

  void apply(bool vr_mode, Texture *input_tex, TEXTUREID input_id, Texture *output_tex, TEXTUREID output_id, const TMatrix &view_tm,
    const TMatrix4 &proj_tm, Texture *depth_tex = nullptr, int eye = -1, int target_layer = 0,
    const Point4 &target_uv_transform = Point4(1, 1, 0, 0), const RectInt *output_viewport = nullptr,
    PreCombineFxProc pre_combine_fx_proc = nullptr);

  void prepareSkyMask(const TMatrix &view_tm); // if we want, we can call it before apply

  void getSettings(DemonPostFxSettings &set) const { set = current; }

  void setDunDir(const Point3 &sunDir) { current.sunDir = sunDir; }
  void setDebugSettings(uint32_t debugFlags) { current.debugFlags = debugFlags; }
  void setSettings(DemonPostFxSettings &set)
  {
    bool glowChanged = current.volfogFade != set.volfogFade || current.glowRadius != set.glowRadius;

    current = set;
    volfogMaxAngleCos = cosf(current.volfogMaxAngle);
    recalcColorMatrix();
    if (glowChanged)
      recalcGlow();
  }

  void setVignetteMultiplier(float vignette_mul) { vignetteMul = vignette_mul; }
  void setVignette(float vignette_mul, const Point2 &start, const Point2 &size)
  {
    setVignetteMultiplier(vignette_mul);

    vignetteStart = start;
    vignetteEnd = size;
    if (vignetteEnd.x < 0.001)
      vignetteEnd.x = 0.001;
    if (vignetteEnd.y < 0.001)
      vignetteEnd.y = 0.001;
    vignetteEnd += start;
  }

  void setLdrVignette(float mul, float width)
  {
    ldrVignetteMul = mul;
    ldrVignetteWidth = width;
  }

  void setSunDir(const Point3 &dir2sun) { current.setSunDir(dir2sun); }

  const UniqueTex &getPrevFrameLowResTex() const { return prevFrameLowResTex; }
  TextureIDPair getUIBlurTex() const { return TextureIDPair(uiBlurTex.getTex2D(), uiBlurTex.getTexId()); }

  void setLenseFlareEnabled(bool enabled);

  void delayedCombineFx(TEXTUREID textureId);

  static bool globalDisableAdaptation;

  const TMatrix &getParamColorMatrix() { return paramColorMatrix; }
  void setExternalSkyMaskTexId(TEXTUREID texId) { externalSkyMaskTexId = texId; }

  DemonPostFxCallback *combineCallback;
  DemonPostFxCallback *volFogCallback;
  // set combineCallback or NULL
  void setCombineCallback(DemonPostFxCallback *cb) { combineCallback = cb; }
  void setVolFogCallback(DemonPostFxCallback *cb) { volFogCallback = cb; }

  void initLenseFlare(const char *lense_covering_tex_name, const char *lense_radial_tex_name);
  void closeLenseFlare();

protected:
  bool skyMaskPrepared;
  void applyLenseFlare(const UniqueTex &src_tex);

  void calcGlowGraphics();
  void calcGlowCompute();

  Flare lensFlare;

  bool eventFinished;

  TMatrix paramColorMatrix;

  UniqueTex lowResLumTexA, lowResLumTexB;

  UniqueTex glowTex, glowTmpTex, tmpTex, uiBlurTex, prevFrameLowResTex;
  UniqueTex ldrTempTex;
  TEXTUREID externalSkyMaskTexId; // for external sky mask, for deferred shading for example

  IPoint2 resSize;
  IPoint2 lowResSize;

  real glowEdgeK, glowFadeK;
  real glowAngle[2];
  real volfogMaxAngleCos;

  real vignetteMul;
  Point2 vignetteStart;
  Point2 vignetteEnd;

  real ldrVignetteMul = 1.f;
  real ldrVignetteWidth = 0.f;

  real screenAspect;

  bool lenseFlareEnabled;

  bool useSimpleMode;
  PostFxRenderer downsample4x;

  PostFxRenderer glowBlurXFx, glowBlurYFx, glowBlur2XFx, glowBlur2YFx, combineFx, radialBlur1Fx, radialBlur2Fx, render2lut;
  eastl::unique_ptr<ComputeShaderElement> glowBlurXFxCS, glowBlurYFxCS, glowBlur2XFxCS, glowBlur2YFxCS;

  DemonPostFxSettings current;

  void recalcColorMatrix();
  void recalcGlow();
  void downsample(Texture *to, TEXTUREID src, int srcW, int srcH, const Point4 &uv_transform = Point4(1, 1, 0, 0));

  Color4 glowWeights0X, glowWeights1X;

  bool useCompute;
};
