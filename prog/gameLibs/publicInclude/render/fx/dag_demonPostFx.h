//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_drv3dCmd.h>
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

  // Adaptation
  real targetBrightness;
  real targetBrightnessUp, targetBrightnessEffect;

  real adaptMin, adaptMax;
  real adaptTimeUp, adaptTimeDown;

  float perceptionLinear;
  float perceptionPower;

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


  DemonPostFx(const DataBlock &main_blk, const DataBlock &hdr_blk, int target_w, int target_h, unsigned rtarget_flags);
  ~DemonPostFx();

  //! updates settings that don't require recreate, or return false to indicated need of recreate
  bool updateSettings(const DataBlock &main_blk, const DataBlock &hdr_blk);

  void apply(bool vr_mode, Texture *input_tex, TEXTUREID input_id, Texture *output_tex, TEXTUREID output_id,
    Texture *depth_tex = nullptr, int eye = -1, int target_layer = 0, const Point4 &target_uv_transform = Point4(1, 1, 0, 0),
    const RectInt *output_viewport = nullptr);

  void prepareSkyMask(); // if we want, we can call it before apply

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

  void update(float dt);

  const UniqueTex &getPrevFrameLowResTex() const { return prevFrameLowResTex; }
  TextureIDPair getUIBlurTex() const { return TextureIDPair(uiBlurTex.getTex2D(), uiBlurTex.getTexId()); }

  void resetAdaptation(float value = 1)
  {
    adaptationResetValue = value;
    framesBeforeValidHistogram = 2;
    removeQueries();
  }

  //! returns current adaptation value; slow due to need to lock RT to read value!
  float getCurrentAdaptationValueSlow();

  void setUseAdaptation(bool use);
  bool getUseAdaptation() { return useAdaptation; }
  void setAdaptationPause(bool pause) { pauseAdaptationUpdate = pause; };
  void setShowHistogram(bool show) { showHistogram = show; }

  void setLenseFlareEnabled(bool enabled);


  void onBeforeReset3dDevice();
  void delayedCombineFx(TEXTUREID textureId);

  static bool globalDisableAdaptation;

  //! use LUT-based shaders (default=false)
  static bool useLutTexture;

  const TMatrix &getParamColorMatrix() { return paramColorMatrix; }
  void setExternalSkyMaskTexId(TEXTUREID texId) { externalSkyMaskTexId = texId; }

  DemonPostFxCallback *combineCallback;
  DemonPostFxCallback *volFogCallback;
  // set combineCallback or NULL
  void setCombineCallback(DemonPostFxCallback *cb) { combineCallback = cb; }
  void setVolFogCallback(DemonPostFxCallback *cb) { volFogCallback = cb; }

  void initLenseFlare(const char *lense_covering_tex_name, const char *lense_radial_tex_name);
  void closeLenseFlare();

  bool getCenterWeightedAdaptation() const;
  void setCenterWeightedAdaptation(bool value);

protected:
  bool skyMaskPrepared;
  void applyLenseFlare(const UniqueTex &src_tex);

  void calcGlowGraphics();
  void calcGlowCompute();

  Flare lensFlare;

  friend class DemonPostFxConProc;
  UniqueBufHolder hist_verts; // for debug
  UniqueBufHolder hist_inds;  // for debug
  enum
  {
    MAX_HISTOGRAM_COLUMNS_BITS = 8,
    MAX_HISTOGRAM_COLUMNS = 1 << MAX_HISTOGRAM_COLUMNS_BITS
  }; // up to 256 is reasonable. SHOULD BE power of TWO.
  uint8_t histogramColumnsBits;
  uint16_t histogramColumns;
  unsigned histogram[MAX_HISTOGRAM_COLUMNS];

  bool eventFinished;
  bool showHistogram;
  void drawHistogram(unsigned *histogram, int max_pixels, const Point2 &lt, const Point2 &rb);
  void drawHistogram(Texture *from, float max_bright, const Point2 &lt, const Point2 &rb);

  TMatrix paramColorMatrix;

  UniqueTex lowResLumTexA, lowResLumTexB, lutTex2d;
  UniqueTex lutTex3d;

  UniqueTex histogramTex, glowTex, glowTmpTex, tmpTex, uiBlurTex, prevFrameLowResTex;
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

  real timePassed;
  float adaptationResetValue;
  float lastHistogramAvgBright;
  int framesBeforeValidHistogram;

  bool useSimpleMode;
  enum
  {
    MAX_QUERY = 2,
    HISTOGRAM_TEXTURES = 3
  };
  static constexpr int MAX_ADAPTATIONS = 3; // max of above
  bool useAdaptation;
  bool pauseAdaptationUpdate;
  carray<float, MAX_ADAPTATIONS> currentOverbright; // adapted ::hdr_max_overbright
  carray<float, MAX_ADAPTATIONS> currentLowBright;  // adapted
  carray<float, MAX_ADAPTATIONS> lumHistScale;
  PostFxRenderer downsample4x;

  PostFxRenderer glowBlurXFx, glowBlurYFx, glowBlur2XFx, glowBlur2YFx, combineFx, radialBlur1Fx, radialBlur2Fx, render2lut;
  eastl::unique_ptr<ComputeShaderElement> glowBlurXFxCS, glowBlurYFxCS, glowBlur2XFxCS, glowBlur2YFxCS;

  DemonPostFxSettings current;

  void recalcColorMatrix();
  void recalcGlow();
  void getAdaptation();
  void startGetAdaptation();
  void removeQueries();
  void downsample(Texture *to, TEXTUREID src, int srcW, int srcH, const Point4 &uv_transform = Point4(1, 1, 0, 0));

  UniqueTex lowresBrightness1;
  bool lowresBrightness2Valid;
  PostFxRenderer histogramQuery;
  int queryTail, queryHead;
  void *histogramQueries[MAX_QUERY][MAX_HISTOGRAM_COLUMNS];
  bool histogramQueriesGot[MAX_QUERY]; /// if query processed

  carray<EventQueryHolder, HISTOGRAM_TEXTURES> histogramTextureEvent;
  carray<UniqueTex, HISTOGRAM_TEXTURES> histogramTexture;
  carray<bool, HISTOGRAM_TEXTURES> histogramTextureCopied;
  static constexpr int INTERLACE_HEIGHT = 64;
  UniqueTex histogramInterlacedTexture;
  int currentHistogramTexture;
  int histogramLockCounter;
  UniqueBuf histogramVb;
  bool centerWeightedAdaptation;
  ShaderMaterial *histogramCalcMat;
  ShaderElement *histogramCalcElement;

  PostFxRenderer histogramAccum;

  PostFxRenderer luminance;
  void initAdaptation();
  void closeAdaptation();
  void initHistogramVb();
  Color4 glowWeights0X, glowWeights1X;

  bool useCompute;

  struct ExposureValues
  {
    float lowBright, avgBright;

    void adapt(float du, float dd, const ExposureValues &next);
    static float adaptValue(float du, float dd, float prev, float next);
    ExposureValues() : lowBright(0), avgBright(0.7), adaptationScale(1) {}
    float adaptationScale; // calculatable
  } currentExposure;
};
