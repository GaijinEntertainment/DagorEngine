#include <stdio.h>
#include <util/dag_globDef.h>
#include <util/dag_lag.h>
#include <math/random/dag_random.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d_platform.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_colorMatrix.h>
#include <render/fx/dag_demonPostFx.h>
#include <fx/dag_hdrRender.h>
#include <startup/dag_globalSettings.h>
#include <supp/dag_prefetch.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <util/dag_convar.h>

#define PROFILE_DEMONPOSTFX 0
#define HIST_USE_INSTANCING 0 // instancing works AWFULLY SLOW
#if PROFILE_DEMONPOSTFX
#include <perfMon/dag_pix.h>
#define PROFILE_START(name) perfmon::begin_named_event(name);
#define PROFILE_END()       perfmon::end_named_event();
#else
#define PROFILE_START(name)
#define PROFILE_END()
#endif // PROFILE_DEMONPOSTFX

#include <shaders/dag_DynamicShaderHelper.h>

// #include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>

// #define __CHECK_ADAPTATION__ 1

static int adaptation_scaleGVarId = -1, adaptation_minGVarId = -1;
static int darkThresholdVarId = -1, darkThreshold0VarId = -1, darkThreshold1VarId = -1;
static int texVarId = -1, texSzVarId = -1, texUvTransformVarId = -1;
static int adaptationOnGVarId = -1, columnsVarId = -1, adaptation_use_center_weightedVarId = -1, adaptation_cw_samples_countVarId = -1;
static int glowTexVarId = -1, glowTexSzVarId = -1, glowScaleVarId = -1, hasPostFxGlowVarId = -1;
static int volfogTexVarId = -1, volfogScaleVarId = -1;
static int volFogOnVarId = -1;
static int thresholdVarId = -1, srcTexVarId = -1;
static int vignette_multiplierGlobVarId = -1;
static int vignette_start_endGlobVarId = -1;
static int hdr_overbrightGVarId = -1;
static int texelOffsetXVarId = -1, texelOffsetYVarId = -1;
static int motionBlurAlphaGId = -1;
static int motionBlurNumberGId = -1;
static int lutTexVarId = -1;
static int targetTextureSizeVarId = -1;
static int ldr_vignette_multiplierVarId = -1;
static int ldr_vignette_widthVarId = -1;
static int ldr_vignette_aspectVarId = -1;

// DoF variables
static int max_hdr_overbrightGVarId = -1;

static int colorMatrixRVarId = -1, colorMatrixGVarId = -1, colorMatrixBVarId = -1;
static int renderCombinePassGVarId = -1;

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2
static bool use_queries = true;
#else
static bool use_queries = false;
#endif

const int LUT_TEXTURE_SIZE = 16;
const int CENTER_WEIGHTED_ADAPTATION_SIZE = 2048;

#define BLUR_SAMPLES 8
static int weight0VarId, weight1VarId;
static int weightVarId[BLUR_SAMPLES], texTmVarId[BLUR_SAMPLES];

bool DemonPostFx::globalDisableAdaptation = false;
bool DemonPostFx::useLutTexture = false;

CONSOLE_BOOL_VAL("render", perceptualAdaptation, true);
CONSOLE_BOOL_VAL("render", useCenterWeightedAdaptation, true);

static DemonPostFx *postFx = NULL;

static inline Color4 calcQuadCoeffs0001(int w, int h)
{
  return Color4(0.5f, -0.5f, 0.50001f + HALF_TEXEL_OFSF / w, 0.50001f + HALF_TEXEL_OFSF / h);
}
static inline Color4 calcQuadCoeffs(int w, int h)
{
  return Color4(0.5f, -0.5f, 0.5000f + HALF_TEXEL_OFSF / w, 0.5000f + HALF_TEXEL_OFSF / h);
}

void DemonPostFx::initAdaptation()
{
  // init adaptation resources
  unsigned flags = TEXCF_RTARGET;
#if _TARGET_C1 | _TARGET_C2

#else
  if ((d3d::get_texformat_usage(TEXFMT_L8) & d3d::USAGE_RTARGET))
    flags |= TEXFMT_L8;
  else
    flags |= TEXFMT_A8R8G8B8;
#endif
  mem_set_0(histogramTexture);
  mem_set_0(histogramTextureCopied);
  if (d3d::get_driver_desc().caps.hasAsyncCopy && (d3d::get_texformat_usage(flags) & d3d::USAGE_VERTEXTEXTURE) &&
      (d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_RTARGET))
  {
    uint32_t interlaceFmt = 0;
    if (lowResSize.y * lowResSize.x / INTERLACE_HEIGHT < (1 << 11)) // mantissa of half float is 11 bit
    {
      debug("max pixels in each hist pixel is guaranteed to be less than 2048 (%d < 12048)",
        lowResSize.y * lowResSize.x / INTERLACE_HEIGHT);
      if ((d3d::get_texformat_usage(TEXFMT_R16F) & (d3d::USAGE_RTARGET | d3d::USAGE_BLEND)) == (d3d::USAGE_RTARGET | d3d::USAGE_BLEND))
      {
        interlaceFmt = TEXFMT_R16F;
      }
      else
      {
        debug("r16f is not supporting blend");
      }
    }

    if (!interlaceFmt &&
        (d3d::get_texformat_usage(TEXFMT_R32F) & (d3d::USAGE_RTARGET | d3d::USAGE_BLEND)) == (d3d::USAGE_RTARGET | d3d::USAGE_BLEND))
      interlaceFmt = TEXFMT_R32F;

#if _TARGET_C1 | _TARGET_C2 // PS4 shader expects 32-bit RT

#endif

    histogramCalcElement = NULL;
    if (interlaceFmt)
    {
      histogramCalcMat = new_shader_material_by_name("calcHistogram");
      if (histogramCalcMat)
        histogramCalcElement = histogramCalcMat->make_elem();
      if (!histogramCalcElement)
        del_it(histogramCalcMat);
    }

    if (histogramCalcElement)
    {
      debug("use gpu adaptation, with %X interlaceFmt", interlaceFmt);
      use_queries = false;
      histogramColumns = MAX_HISTOGRAM_COLUMNS;
      histogramColumnsBits = MAX_HISTOGRAM_COLUMNS_BITS;
      currentHistogramTexture = 0;
      histogramLockCounter = 0;
      histogramInterlacedTexture =
        dag::create_tex(NULL, histogramColumns, INTERLACE_HEIGHT, TEXCF_RTARGET | interlaceFmt, 1, "postfx_interlacedHist");
      d3d_err(histogramInterlacedTexture.getTex2D());
      histogramInterlacedTexture->texaddr(TEXADDR_CLAMP);
      histogramInterlacedTexture->texfilter(TEXFILTER_POINT);
      for (int i = 0; i < histogramTexture.size(); i++)
      {
        String name(16, "histogram%d", i);
        histogramTexture[i] =
          dag::create_tex(NULL, histogramColumns, 1, TEXCF_RTARGET | TEXCF_LINEAR_LAYOUT | TEXFMT_R32F, 1, name.c_str());
        histogramTextureEvent[i].reset(d3d::create_event_query());
      }

      initHistogramVb();

      Color4 col(1. / lowResSize.x, 1. / lowResSize.y, lowResSize.x, lowResSize.y);
      histogramCalcMat->set_color4_param(texSzVarId, col);
      histogramAccum.init("accumHistogram");
      col = Color4(1. / histogramColumns, 1. / INTERLACE_HEIGHT, histogramColumns, INTERLACE_HEIGHT);
      histogramAccum.getMat()->set_color4_param(texSzVarId, col);
      histogramAccum.getMat()->set_texture_param(texVarId, histogramInterlacedTexture.getTexId());
    }
  }

  memset(histogramQueries, 0, sizeof(histogramQueries));
  memset(histogramQueriesGot, 0, sizeof(histogramQueriesGot));
  queryTail = queryHead = 0;

  lowresBrightness1 = dag::create_tex(NULL, lowResSize.x, lowResSize.y, flags, 1, "postfx_lowresBrightness1");
  d3d_err(lowresBrightness1.getTex2D());
  lowresBrightness1->texaddr(TEXADDR_CLAMP);
  lowresBrightness1->texfilter(TEXFILTER_POINT);
  lowresBrightness2Valid = false;

  luminance.init("demon_postfx_luminance_histogram");
  Color4 quad_coefs = calcQuadCoeffs(lowResSize.x, lowResSize.y);
  luminance.getMat()->set_color4_param(texSzVarId, quad_coefs);
  luminance.getMat()->set_texture_param(texVarId, prevFrameLowResTex.getTexId());
  if (use_queries)
  {
    histogramQuery.init("calc_query");
    if (histogramQuery.getMat())
    {
      histogramQuery.getMat()->set_color4_param(texSzVarId, quad_coefs);
      histogramQuery.getMat()->set_texture_param(texVarId, lowresBrightness1.getTexId());
    }
  }

  columnsVarId = ::get_shader_variable_id("columns", true);
  adaptationOnGVarId = ::get_shader_glob_var_id("adaptationOn", true);
  adaptation_use_center_weightedVarId = ::get_shader_glob_var_id("adaptation_use_center_weighted", true);
  adaptation_cw_samples_countVarId = ::get_shader_glob_var_id("adaptation_cw_samples_count", true);
  adaptation_scaleGVarId = ::get_shader_glob_var_id("adaptation_scale", true);
  adaptation_minGVarId = ::get_shader_glob_var_id("adaptation_min", true);
  for (int i = 0; i < currentOverbright.size(); ++i)
    currentOverbright[i] = 1.0f;

  motionBlurAlphaGId = ::get_shader_glob_var_id("motionBlurAlpha", true);
  motionBlurNumberGId = ::get_shader_glob_var_id("motionBlurNumber", true);

  targetTextureSizeVarId = ::get_shader_glob_var_id("targetTextureSize", true);

  resetAdaptation();
}

void DemonPostFx::closeAdaptation()
{
  histogramVb.close();
  histogramCalcElement = NULL;
  del_it(histogramCalcMat);
  for (int i = 0; i < histogramTexture.size(); ++i)
  {
    histogramTexture[i].close();
    histogramTextureEvent[i].reset();
  }

  histogramQuery.clear();
  histogramAccum.clear();
  histogramInterlacedTexture.close();
  lowresBrightness1.close();
}

struct HistogramVertex
{
  uint16_t x, y;
};

void DemonPostFx::HistBufferFiller::reloadD3dRes(Sbuffer *buf)
{
  HistogramVertex *vert;
  d3d_err(buf->lock(0, 0, (void **)&vert, VBLOCK_WRITEONLY));
#if !HIST_USE_INSTANCING
  if (vert)
  {
    for (int y = 0; y < vbSize.y; ++y)
      for (int x = 0; x < vbSize.x; ++x, vert++)
        vert->x = x, vert->y = y;
  }
#endif
  buf->unlock();
}

void DemonPostFx::initHistogramVb()
{
  histogramVb.close();
  IPoint2 vbSize = centerWeightedAdaptation ? IPoint2(CENTER_WEIGHTED_ADAPTATION_SIZE, 1) : lowResSize;
  ShaderGlobal::set_real(adaptation_cw_samples_countVarId, CENTER_WEIGHTED_ADAPTATION_SIZE);
  ShaderGlobal::set_int(adaptation_use_center_weightedVarId, centerWeightedAdaptation ? 1 : 0);

#if HIST_USE_INSTANCING
  histogramVb = dag::create_vb(sizeof(HistogramVertex), SBCF_MAYBELOST, "histogramInst");
#else
  histogramVb = dag::create_vb(vbSize.x * vbSize.y * sizeof(HistogramVertex), SBCF_MAYBELOST, "histogram");
#endif
  d3d_err(histogramVb.getBuf());
  histBufferFiller.vbSize = vbSize;
  histBufferFiller.reloadD3dRes(histogramVb.getBuf());
  histogramVb.getBuf()->setReloadCallback(&histBufferFiller);
}

float DemonPostFx::getCurrentAdaptationValueSlow() { return currentExposure.adaptationScale; }


// TODO: parallel histogram computation (4 threads)
template <class HistogramType>
static void getHistogramTexCPU(unsigned char *__restrict data, int w, int h, int stride, unsigned *__restrict histogram,
  unsigned size_bits, int &max_pixels, int &total_pixels)
{
  G_UNREFERENCED(size_bits);
  // analyze rendertarget
  max_pixels = w * h;
  total_pixels = w * h; // if we want to ignore black pixels it will be different
  if (stride == w)
  {
    HistogramType histograms[8][256];
    memset(histograms, 0, sizeof(histograms));
    for (int i = max_pixels / 16; i; i--, data += 16)
    {
      PREFETCH_DATA(256, data);
#define LOOP_BODY(j)            \
  histograms[0][data[j + 0]]++; \
  histograms[1][data[j + 1]]++; \
  histograms[2][data[j + 2]]++; \
  histograms[3][data[j + 3]]++; \
  histograms[4][data[j + 4]]++; \
  histograms[5][data[j + 5]]++; \
  histograms[6][data[j + 6]]++; \
  histograms[7][data[j + 7]]++;

      LOOP_BODY(0)
      LOOP_BODY(8)
#undef LOOP_BODY
    }
    for (int i = 0; i < 256; i++)
      histogram[i] = histograms[0][i] + histograms[1][i] + histograms[2][i] + histograms[3][i] + histograms[4][i] + histograms[5][i] +
                     histograms[6][i] + histograms[7][i];
  }
  else
  {
    HistogramType histograms[4][256];
    memset(histograms, 0, sizeof(histograms));
#if _TARGET_PC
    // a8r8g8 format
    for (int y = h; y; y--, data += stride - w * 4)
    {
      PREFETCH_DATA(256, data);
      for (int i = w / 4; i; i--, data += 16)
      {
        histograms[0][data[0]]++;
        histograms[1][data[4]]++;
        histograms[2][data[8]]++;
        histograms[3][data[12]]++;
      }
    }
#else
    for (int y = h; y; y--, data += stride - w)
    {
      PREFETCH_DATA(256, data);
      for (int i = w / 4; i; i--, data += 4)
      {
        histograms[0][data[0]]++;
        histograms[1][data[1]]++;
        histograms[2][data[2]]++;
        histograms[3][data[3]]++;
      }
    }
#endif
    for (int i = 0; i < 256; i++)
      histogram[i] = histograms[0][i] + histograms[1][i] + histograms[2][i] + histograms[3][i];
  }
}

void DemonPostFx::drawHistogram(unsigned *, int, const Point2 &, const Point2 &) {}

void DemonPostFx::drawHistogram(Texture *, float, const Point2 &, const Point2 &) {}

DemonPostFx::DemonPostFx(const DataBlock &main_blk, const DataBlock &hdr_blk, int target_w, int target_h, unsigned rtarget_flags,
  bool initPostfxGlow, bool initSunVolfog) :
  paramColorMatrix(1),
  preColorMatrix(1),
  postColorMatrix(1),
  useSimpleMode(false),
  useAdaptation(true),
  timePassed(0),
  adaptationResetValue(1),
  lastHistogramAvgBright(0.f),
  framesBeforeValidHistogram(2),
  showHistogram(false),
  combineCallback(NULL),
  skyMaskPrepared(false),
  volFogCallback(NULL),
  centerWeightedAdaptation(true),
  histogramCalcMat(NULL),
  histogramCalcElement(NULL),
  pauseAdaptationUpdate(false),
  eventFinished(false),
  externalSkyMaskTexId(0)
{
  const unsigned int workingFlags = d3d::USAGE_FILTER | d3d::USAGE_BLEND | d3d::USAGE_RTARGET;
  unsigned sky_fmt = TEXFMT_DEFAULT;
  if ((d3d::get_texformat_usage(TEXFMT_R11G11B10F) & workingFlags) == workingFlags)
    sky_fmt = TEXFMT_R11G11B10F;

  // This needs to be changed so we should check for d3d::get_texformat_usage and compute on all platforms.
  useCompute = d3d::should_use_compute_for_image_processing({rtarget_flags, sky_fmt});

  mem_set_0(histogramTexture);

  histogramColumnsBits = MAX_HISTOGRAM_COLUMNS_BITS;
  histogramColumns = 1 << histogramColumnsBits;
  memset(histogramQueries, 0, sizeof(histogramQueries));
  memset(histogramQueriesGot, 0, sizeof(histogramQueriesGot));
  useSimpleMode = (::hdr_render_mode == HDR_MODE_NONE);

  useAdaptation = !globalDisableAdaptation && !useSimpleMode && hdr_blk.getBool("useAdaptation", true);
  centerWeightedAdaptation = hdr_blk.getBool("centerWeightedAdaptation", true);

  hdr_overbrightGVarId = ::get_shader_glob_var_id("hdr_overbright", true);
  darkThresholdVarId = ::get_shader_variable_id("darkThreshold", true);
  darkThreshold0VarId = ::get_shader_variable_id("darkThreshold0");
  darkThreshold1VarId = ::get_shader_variable_id("darkThreshold1");

  vignetteMul = 1.0;
  vignetteStart = Point2(1, 0.7);
  vignetteEnd = Point2(1.5, 1.01);

  resSize = IPoint2(target_w, target_h);
  lowResSize.x = target_w / 4;
  lowResSize.y = target_h / 4;
  if (useCompute)
  {
    // When using compute shaders, we need to have a multiple of 8 so we can
    // use 8x8 thread groups without checking for out-of-bounds access
    lowResSize.x &= ~7;
    lowResSize.y &= ~7;
  }

  if ((!lowResSize.x || !lowResSize.y) && !useSimpleMode)
  {
    logwarn_ctx("switched to useSimpleMode/HDR_MODE_NONE due to lowResSize=%dx%d (target=%dx%d)", lowResSize.x, lowResSize.y, target_w,
      target_h);
    useSimpleMode = true;
    useAdaptation = false;
    ::hdr_render_mode = HDR_MODE_NONE;
  }

  texVarId = get_shader_variable_id("tex");
  texSzVarId = get_shader_variable_id("texsz_consts");
  texUvTransformVarId = get_shader_variable_id("texUvTransform");

  colorMatrixRVarId = get_shader_variable_id("colorMatrixR", true);
  colorMatrixGVarId = get_shader_variable_id("colorMatrixG", true);
  colorMatrixBVarId = get_shader_variable_id("colorMatrixB", true);

  if (useLutTexture)
  {
    lutTexVarId = get_shader_variable_id("lut_tex");
  }

  vignette_multiplierGlobVarId = ::get_shader_glob_var_id("vignette_multiplier", true);
  vignette_start_endGlobVarId = ::get_shader_glob_var_id("vignette_start_end", true);

  ldr_vignette_multiplierVarId = get_shader_variable_id("ldr_vignette_multiplier", true);
  ldr_vignette_widthVarId = get_shader_variable_id("ldr_vignette_width", true);
  ldr_vignette_aspectVarId = get_shader_variable_id("ldr_vignette_aspect", true);


  if (!useSimpleMode)
  {
    if (useCompute)
    {
      glowBlurXFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));
      glowBlurYFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));
      glowBlur2XFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));
      glowBlur2YFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));

      if (!glowBlurXFxCS)
        useCompute = false;
    }

    if (initSunVolfog)
    {
      lowResLumTexA = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_lowresLumA");
      d3d_err(lowResLumTexA.getTex2D());
      lowResLumTexA->texaddr(TEXADDR_MIRROR);

      lowResLumTexB = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_lowresLumB");
      d3d_err(lowResLumTexB.getTex2D());
      lowResLumTexB->texaddr(TEXADDR_MIRROR);

      histogramTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_histogram");
      d3d_err(histogramTex.getTex2D());
      histogramTex->texaddr(TEXADDR_CLAMP);
    }

    uiBlurTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_uiBlur");
    d3d_err(uiBlurTex.getTex2D());
    uiBlurTex->texaddr(TEXADDR_CLAMP);

    uint32_t usageFlag = useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET;

    if (initPostfxGlow)
    {
      tmpTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | usageFlag, 1, "postfx_tmp");
      d3d_err(tmpTex.getTex2D());
      tmpTex->texaddr(TEXADDR_CLAMP);

      glowTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, rtarget_flags | usageFlag, 1, "postfx_glow");
      d3d_err(glowTex.getTex2D());
      glowTex->texaddr(TEXADDR_CLAMP);
    }

    if (initSunVolfog || initPostfxGlow || useAdaptation)
    {
      // low-res previous frame texture
      prevFrameLowResTex =
        dag::create_tex(NULL, lowResSize.x, lowResSize.y, rtarget_flags | TEXCF_RTARGET, 1, "postfx_prevFrameLowRes");
      d3d_err(prevFrameLowResTex.getTex2D());
      prevFrameLowResTex->texaddr(TEXADDR_CLAMP);
    }

    if (useLutTexture)
    {
      // this two textures must point to the same memory address,
      // which can be done using "alias" option in ps3_drv preallocated RT table.
      lutTex2d = dag::create_tex(NULL, LUT_TEXTURE_SIZE * LUT_TEXTURE_SIZE, LUT_TEXTURE_SIZE, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1,
        "DemonPostFx LUT 2d");
      d3d_err(lutTex2d.getTex2D());
      lutTex3d = dag::create_voltex(LUT_TEXTURE_SIZE, LUT_TEXTURE_SIZE, LUT_TEXTURE_SIZE, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1,
        "DemonPostFx LUT 3d");
      d3d_err(lutTex3d.getTex2D());
    }

    glowBlurXFx.init("demon_postfx_blur");
    glowBlurYFx.init("demon_postfx_blur");
    glowBlur2XFx.init("demon_postfx_blur");
    glowBlur2YFx.init("demon_postfx_blur");
    radialBlur1Fx.init("demon_postfx_blur");
    radialBlur2Fx.init("demon_postfx_blur");
    downsample4x.init("downsample4x");

    glowTexSzVarId = get_shader_variable_id("glow_texsz_consts");
    glowTexVarId = get_shader_variable_id("glowTex");
    glowScaleVarId = get_shader_variable_id("glowScale");
    hasPostFxGlowVarId = get_shader_variable_id("hasPostFxGlow");
    volfogTexVarId = get_shader_variable_id("volfogTex");
    volfogScaleVarId = get_shader_variable_id("volfogScale");
    volFogOnVarId = get_shader_variable_id("volFogOn", true);
    thresholdVarId = get_shader_variable_id("threshold");
    srcTexVarId = get_shader_variable_id("srcTex");
    texelOffsetXVarId = ::get_shader_variable_id("texelOffsetX", true);
    texelOffsetYVarId = ::get_shader_variable_id("texelOffsetY", true);

    weight0VarId = get_shader_variable_id("weight0");
    weight1VarId = get_shader_variable_id("weight1");
    char name[64];
    for (int i = 0; i < BLUR_SAMPLES; ++i)
    {
      snprintf(name, sizeof(name), "weight%d", i);
      weightVarId[i] = get_shader_variable_id(name);
      snprintf(name, sizeof(name), "texTm%d", i);
      texTmVarId[i] = get_shader_variable_id(name);
    }

    Color4 quad_coefs = calcQuadCoeffs(lowResSize.x, lowResSize.y);

    // init glow blur
    glowBlurXFx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    glowBlurYFx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    glowBlur2XFx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    glowBlur2YFx.getMat()->set_color4_param(texSzVarId, quad_coefs);

    // init radial blur
    radialBlur1Fx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    radialBlur2Fx.getMat()->set_color4_param(texSzVarId, quad_coefs);

    initAdaptation();
  }

  if (!updateSettings(main_blk, hdr_blk))
  {
    G_ASSERT(0 && "updateSettings() must return true in ctor!");
  }

  combineFx.init(useSimpleMode ? "demon_postfx_combine_simple" : "demon_postfx_combine");

  if (useLutTexture)
    render2lut.init("demon_postfx_render2lut");

  postFx = this;
  max_hdr_overbrightGVarId = ::get_shader_glob_var_id("max_hdr_overbright", true);
  ShaderGlobal::set_real_fast(max_hdr_overbrightGVarId, ::hdr_max_overbright);
}

void DemonPostFx::closeLenseFlare() { lensFlare.close(); }

bool DemonPostFx::getCenterWeightedAdaptation() const { return centerWeightedAdaptation; }

void DemonPostFx::setCenterWeightedAdaptation(bool value)
{
  if (centerWeightedAdaptation == value)
    return;
  useCenterWeightedAdaptation.set(value);
  centerWeightedAdaptation = value;
  initHistogramVb();
}

void DemonPostFx::initLenseFlare(const char *lense_covering_tex_name, const char *lense_radial_tex_name)
{
  if (::hdr_render_mode != HDR_MODE_FLOAT)
    return;
  lensFlare.init(lowResSize, lense_covering_tex_name, lense_radial_tex_name);
}

bool DemonPostFx::updateSettings(const DataBlock &main_blk, const DataBlock &hdr_blk)
{
  if (useSimpleMode != (::hdr_render_mode == HDR_MODE_NONE))
    return false;
  if (useAdaptation != (!globalDisableAdaptation && !useSimpleMode && hdr_blk.getBool("useAdaptation", true)))
    return false;

  setCenterWeightedAdaptation(hdr_blk.getBool("centerWeightedAdaptation", true));

  current.targetBrightnessUp = hdr_blk.getReal("targetBrightnessUp", 1.005f);
  current.targetBrightnessEffect = hdr_blk.getReal("targetBrightnessEffect", 0.5f);
  current.targetBrightness = hdr_blk.getReal("adaptationMultiplier", 0.3f);
  current.adaptTimeUp = hdr_blk.getReal("adaptationTimeHighToNorm", 5.0f);
  current.adaptTimeDown = hdr_blk.getReal("adaptationTimeNormToHigh", 0.2f);

  const DataBlock &fakeBlk = *hdr_blk.getBlockByNameEx("fake");
  current.adaptMin = fakeBlk.getReal("adaptationMin", 0.3f);
  current.adaptMax = fakeBlk.getReal("adaptationMax", 2.0f);

  current.perceptionLinear = hdr_blk.getReal("perceptionLinear", 0.85f); // 1, 1 will disable perceptual adaptation.
  current.perceptionPower = hdr_blk.getReal("perceptionPower", 0.8f);

  current.lum_min = fakeBlk.getReal("lumMin", 0.01f);
  current.lum_max = fakeBlk.getReal("lumMax", 0.99f);
  current.lum_avg = fakeBlk.getReal("lumAvg", 0.95f);
  current.lowBrightScale = fakeBlk.getReal("lowBrightScale", 0.9f);
  if (::hdr_render_format == (TEXCF_BEST | TEXCF_ABEST))
    current.lowBrightScale = 0;

  // current.useRawSkyMask=main_blk.getBool("useRawSkyMask", false);

  showAlpha = main_blk.getBool("showAlpha", false);

  // lowResSize=main_blk.getIPoint2("lowRes", IPoint2(256, 256));
  glowEdgeK = main_blk.getReal("glowEdgeK", 0.1f);
  glowFadeK = main_blk.getReal("glowFadeK", 1);

  glowAngle[0] = main_blk.getReal("glowAngleU", 45);
  glowAngle[1] = main_blk.getReal("glowAngleV", 135);

  current.glowRadius = main_blk.getReal("glowRadius", 0.05f);

  current.volfogFade = main_blk.getReal("volfogFade", 0.005f);
  current.volfogRange = main_blk.getReal("volfogRange", 7.0f);
  current.volfogMul = main_blk.getReal("volfogMul", 1.0f);
  current.volfogColor = color3(main_blk.getE3dcolor("volfogColor", E3DCOLOR(255, 255, 255)));
  current.volfogMaxAngle = DegToRad(main_blk.getReal("volfogMaxAngle", 70));
  volfogMaxAngleCos = cosf(current.volfogMaxAngle);

  screenAspect = main_blk.getReal("screenAspect", 0.75f);

  current.hdrDarkThreshold = main_blk.getReal("hdrDarkThreshold", 1.5f);
  current.hdrGlowMul = main_blk.getReal("hdrGlowMul", 1.0f);
  current.hdrGlowPower = main_blk.getReal("hdrGlowPower", 1.0f);

  current.setSunDir(dir_from_polar(DegToRad(90 - main_blk.getReal("sunAzimuth", 0)), DegToRad(main_blk.getReal("sunZenith", 45))));

  // init color matrix
  const DataBlock &cmBlk = *main_blk.getBlockByNameEx("colorMatrix");

  current.hueShift = cmBlk.getReal("hue", 0);
  current.saturationColor = color3(cmBlk.getE3dcolor("saturationColor", E3DCOLOR(255, 255, 255)));
  current.saturation = cmBlk.getReal("saturation", 100.0f);
  current.grayColor = color3(cmBlk.getE3dcolor("grayColor", E3DCOLOR(255, 255, 255)));
  current.contrastColor = color3(cmBlk.getE3dcolor("contrastColor", E3DCOLOR(255, 255, 255)));
  current.contrast = cmBlk.getReal("contrast", 100.0f);
  current.contrastPivotColor = color3(cmBlk.getE3dcolor("contrastPivotColor", E3DCOLOR(255, 255, 255)));
  current.contrastPivot = cmBlk.getReal("contrastPivot", 50.0f);
  current.brightnessColor = color3(cmBlk.getE3dcolor("brightnessColor", E3DCOLOR(255, 255, 255)));
  current.brightness = cmBlk.getReal("brightness", 100.0f);

  recalcColorMatrix();
  recalcGlow();

  return true;
}

DemonPostFx::~DemonPostFx()
{
  if (combineFx.getMat())
    combineFx.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);
  if (downsample4x.getMat())
    downsample4x.getMat()->set_texture_param(srcTexVarId, BAD_TEXTUREID);

  postFx = NULL;

  glowBlur2YFx.clear();
  glowBlur2XFx.clear();
  glowBlurYFx.clear();
  glowBlurXFx.clear();
  glowBlur2YFxCS.reset();
  glowBlur2XFxCS.reset();
  glowBlurYFxCS.reset();
  glowBlurXFxCS.reset();
  radialBlur1Fx.clear();
  radialBlur2Fx.clear();
  luminance.clear();
  combineFx.clear();
  downsample4x.clear();
  combineCallback = NULL;

  if (useLutTexture)
    render2lut.clear();

  prevFrameLowResTex.close();
  glowTex.close();
  tmpTex.close();
  uiBlurTex.close();
  histogramTex.close();
  lowResLumTexB.close();
  lowResLumTexA.close();
  closeAdaptation();

  if (useLutTexture)
  {
    lutTex2d.close();
    lutTex3d.close();
  }

  removeQueries();

  // FLARE
  closeLenseFlare();
}

#include <perfMon/dag_statDrv.h>
void DemonPostFx::applyLenseFlare(const UniqueTex &src_tex)
{
  if (!lenseFlareEnabled)
    return;
  lensFlare.apply(src_tex.getTex2D(), src_tex.getTexId());
}

void DemonPostFx::calcGlowGraphics()
{
  {
    TIME_D3D_PROFILE(glowBlurXFx);
    d3d::resource_barrier({prevFrameLowResTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    // glow
    d3d::set_render_target(tmpTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlurXFx.getMat()->set_color4_param(weight0VarId, current.hdrGlowMul * glowWeights0X);
    glowBlurXFx.getMat()->set_color4_param(weight1VarId, current.hdrGlowMul * glowWeights1X);
    glowBlurXFx.getMat()->set_color4_param(darkThreshold0VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights0X);
    glowBlurXFx.getMat()->set_color4_param(darkThreshold1VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights1X);
    glowBlurXFx.getMat()->set_texture_param(texVarId, prevFrameLowResTex.getTexId());
    glowBlurXFx.render();
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlurYFx);
    d3d::set_render_target(glowTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlurYFx.getMat()->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlurYFx.render();
    d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2XFx);
    d3d::set_render_target(tmpTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlur2XFx.getMat()->set_texture_param(texVarId, glowTex.getTexId());
    glowBlur2XFx.render();
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2YFx);
    d3d::set_render_target(glowTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlur2YFx.getMat()->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlur2YFx.render();
  }
}

void DemonPostFx::calcGlowCompute()
{
  {
    TIME_D3D_PROFILE(glowBlurXFx_cs);
    d3d::resource_barrier({prevFrameLowResTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    // glow
    d3d::set_rwtex(STAGE_CS, 0, tmpTex.getTex2D(), 0, 0);
    glowBlurXFxCS->set_color4_param(weight0VarId, current.hdrGlowMul * glowWeights0X);
    glowBlurXFxCS->set_color4_param(weight1VarId, current.hdrGlowMul * glowWeights1X);
    glowBlurXFxCS->set_color4_param(darkThreshold0VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights0X);
    glowBlurXFxCS->set_color4_param(darkThreshold1VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights1X);
    glowBlurXFxCS->set_texture_param(texVarId, prevFrameLowResTex.getTexId());
    glowBlurXFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlurXFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlurYFx_cs);
    d3d::set_rwtex(STAGE_CS, 0, glowTex.getTex2D(), 0, 0);
    glowBlurYFxCS->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlurYFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlurYFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2XFx_cs);
    d3d::set_rwtex(STAGE_CS, 0, tmpTex.getTex2D(), 0, 0);
    glowBlur2XFxCS->set_texture_param(texVarId, glowTex.getTexId());
    glowBlur2XFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlur2XFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2YFx_cs);
    d3d::set_rwtex(STAGE_CS, 0, glowTex.getTex2D(), 0, 0);
    glowBlur2YFxCS->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlur2YFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlur2YFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
}

void DemonPostFx::recalcColorMatrix()
{
  paramColorMatrix = ::make_hue_shift_color_matrix(current.hueShift);

  paramColorMatrix *= ::make_saturation_color_matrix(current.saturationColor * (current.saturation / 100), current.grayColor);

  paramColorMatrix *= ::make_contrast_color_matrix(current.contrastColor * (current.contrast / 100),
    current.contrastPivotColor * (current.contrastPivot / 100));

  paramColorMatrix *= ::make_brightness_color_matrix(current.brightnessColor * (current.brightness / 100));
}


void DemonPostFx::recalcGlow()
{
  if (useSimpleMode)
    return;
  float weights[BLUR_SAMPLES];
  for (int i = 0; i < BLUR_SAMPLES; ++i)
    weights[i] = powf(current.volfogFade, (i + 1) / real(BLUR_SAMPLES * BLUR_SAMPLES)) / BLUR_SAMPLES;

  radialBlur1Fx.getMat()->set_color4_param(weight0VarId, Color4(weights));
  radialBlur1Fx.getMat()->set_color4_param(weight1VarId, Color4(weights + 4));

  for (int i = 0; i < BLUR_SAMPLES; ++i)
    weights[i] = powf(current.volfogFade, i / real(BLUR_SAMPLES)) / BLUR_SAMPLES;

  radialBlur2Fx.getMat()->set_color4_param(weight0VarId, Color4(weights));
  radialBlur2Fx.getMat()->set_color4_param(weight1VarId, Color4(weights + 4));

  real u0 = cosf(DegToRad(-glowAngle[0]));
  real v0 = sinf(DegToRad(-glowAngle[0]));
  real u1 = cosf(DegToRad(-glowAngle[1]));
  real v1 = sinf(DegToRad(-glowAngle[1]));

  real glowWt[BLUR_SAMPLES / 2];
  real sum = 0;

  for (int i = 0; i < BLUR_SAMPLES / 2; ++i)
  {
    real t = real(i) / (BLUR_SAMPLES / 2);
    real w = (glowFadeK >= 0 ? powf(1 - t, glowFadeK) : 1 - powf(t, -glowFadeK));
    w = w * (1 - glowEdgeK) + glowEdgeK;
    glowWt[i] = w;
    sum += w;
  }

  if (!float_nonzero(sum))
    sum = 1;

  for (int i = 0; i < BLUR_SAMPLES / 2; ++i)
    glowWt[i] /= sum * 2;

  float weightsX[BLUR_SAMPLES], weightsX2[BLUR_SAMPLES];
  for (int i = 0; i < BLUR_SAMPLES; ++i)
  {
    real w = glowWt[i < BLUR_SAMPLES / 2 ? BLUR_SAMPLES / 2 - 1 - i : i - BLUR_SAMPLES / 2];

    real o1 = (i - (BLUR_SAMPLES - 1) * 0.5f) / ((BLUR_SAMPLES - 1) * 0.5f);
    Point2 o = Point2(o1 / (float)lowResSize.x, o1 / (float)lowResSize.y);
    o *= 2.f;

    real w1 = 1.0 / BLUR_SAMPLES; // use box filter for first iteration

    weightsX[i] = w1;
    weightsX2[i] = w;

    glowBlurXFx.getMat()->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u0, o.y * v0));

    glowBlurYFx.getMat()->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u1, o.y * v1));

    glowBlur2XFx.getMat()->set_color4_param(texTmVarId[i],
      Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u0, o.y * BLUR_SAMPLES * 0.5f * v0));

    glowBlur2YFx.getMat()->set_color4_param(texTmVarId[i],
      Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u1, o.y * BLUR_SAMPLES * 0.5f * v1));

    if (useCompute)
    {
      glowBlurXFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u0, o.y * v0));

      glowBlurYFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u1, o.y * v1));

      glowBlur2XFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u0, o.y * BLUR_SAMPLES * 0.5f * v0));

      glowBlur2YFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u1, o.y * BLUR_SAMPLES * 0.5f * v1));
    }
  }
  glowWeights0X = Color4(weightsX);
  glowWeights1X = Color4(weightsX + 4);
  glowBlurXFx.getMat()->set_int_param(darkThresholdVarId, 1);
  glowBlurXFx.getMat()->set_color4_param(weight0VarId, glowWeights0X * current.hdrGlowMul);
  glowBlurXFx.getMat()->set_color4_param(weight1VarId, glowWeights1X * current.hdrGlowMul);
  glowBlurYFx.getMat()->set_color4_param(weight0VarId, glowWeights0X);
  glowBlurYFx.getMat()->set_color4_param(weight1VarId, glowWeights1X);

  glowBlur2XFx.getMat()->set_color4_param(weight0VarId, Color4(weightsX2));
  glowBlur2XFx.getMat()->set_color4_param(weight1VarId, Color4(weightsX2 + 4));

  glowBlur2YFx.getMat()->set_color4_param(weight0VarId, Color4(weightsX2));
  glowBlur2YFx.getMat()->set_color4_param(weight1VarId, Color4(weightsX2 + 4));

  if (useCompute)
  {
    glowBlurXFxCS->set_int_param(darkThresholdVarId, 1);
    glowBlurXFxCS->set_color4_param(weight0VarId, glowWeights0X * current.hdrGlowMul);
    glowBlurXFxCS->set_color4_param(weight1VarId, glowWeights1X * current.hdrGlowMul);
    glowBlurYFxCS->set_color4_param(weight0VarId, glowWeights0X);
    glowBlurYFxCS->set_color4_param(weight1VarId, glowWeights1X);

    glowBlur2XFxCS->set_color4_param(weight0VarId, Color4(weightsX2));
    glowBlur2XFxCS->set_color4_param(weight1VarId, Color4(weightsX2 + 4));

    glowBlur2YFxCS->set_color4_param(weight0VarId, Color4(weightsX2));
    glowBlur2YFxCS->set_color4_param(weight1VarId, Color4(weightsX2 + 4));
  }
}


void DemonPostFx::setUseAdaptation(bool use) { useAdaptation = !globalDisableAdaptation && !useSimpleMode && use; }

void DemonPostFx::setLenseFlareEnabled(bool enabled)
{
  lenseFlareEnabled = enabled;
  lensFlare.toggleEnabled(enabled);
}

#include <debug/dag_debug3d.h>
#if defined(__CHECK_ADAPTATION__)
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
static double get_average(Texture *tex)
{
  TextureInfo tinfo;
  d3d_err(tex->getinfo(tinfo));

  void *ptr;
  int stride;
  d3d_err(tex->lockimg(&ptr, stride, 0));
  if ((tinfo.cflg & TEXFMT_MASK) == TEXFMT_R16F)
  {
    unsigned short *halfs = (unsigned short *)ptr;
    double average = 0;
    for (int j = 0; j < tinfo.h; ++j)
      for (int i = 0; i < tinfo.w; ++i, halfs++)
      {
        average += half_to_float(*halfs);
      }
    d3d_err(tex->unlockimg());
    return average / (tinfo.w * tinfo.h);
  }
  else if ((tinfo.cflg & TEXFMT_MASK) == TEXFMT_R32F)
  {
    float *floats = (float *)ptr;
    double average = 0;
    for (int j = 0; j < tinfo.h; ++j)
      for (int i = 0; i < tinfo.w; ++i, floats++)
      {
        average += *floats;
      }
    d3d_err(tex->unlockimg());
    return average / (tinfo.w * tinfo.h);
  }
  else
  {
    E3DCOLOR *pixels = (E3DCOLOR *)ptr;
    double average = 0;
    for (int j = 0; j < tinfo.h; ++j)
      for (int i = 0; i < tinfo.w; ++i, pixels++)
        average += brightness(color3(*pixels));
    d3d_err(tex->unlockimg());
    return average / (tinfo.w * tinfo.h);
  }
}
#endif

// downsample 4x4->1
void DemonPostFx::downsample(Texture *to, TEXTUREID src, int srcW, int srcH, const Point4 &uv_transform)
{
  TIME_D3D_PROFILE(downsample);

  // d3d::stretch_rect(target_tex, prevFrameLowResTex);
  // use filtered downsampling
  d3d::set_render_target(to, 0);
  d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
  Color4 target_coefs = calcQuadCoeffs0001(srcW, srcH);
  static int texelOffsetVarId = ::get_shader_variable_id("texelOffset");
  static int uvTransformVarId = ::get_shader_variable_id("uvTransform");
  if (srcTexVarId < 0)
    srcTexVarId = ::get_shader_variable_id("srcTex");
  downsample4x.getMat()->set_texture_param(srcTexVarId, src);

  float invTargX = 1.0f / srcW, invTargY = 1.0f / srcH;

  downsample4x.getMat()->set_color4_param(texelOffsetVarId,
    Color4(0.5f * invTargX + target_coefs.b, 0.5f * invTargY + target_coefs.a, invTargX * 2, invTargY * 2));

  downsample4x.getMat()->set_color4_param(uvTransformVarId, reinterpret_cast<const Color4 &>(uv_transform));

  downsample4x.render();
  downsample4x.getMat()->set_texture_param(srcTexVarId, BAD_TEXTUREID);
}

#if 0
static float getBright(int cid, int pixels, int column_pixels, int size, int threshold)
{
  if (column_pixels == 0)
    return 0.f;

  float linear = float(threshold + column_pixels - pixels) / column_pixels;
  return (linear+cid)/size;
}

static void find_min_avg_maximum(int lowThreshold, int avgThreshold,
        unsigned * __restrict histogram, int size,
        float &lowBright, float &avgBright, int &minId, int &avgId)
{
  unsigned * __restrict pixelsPtr = histogram;
  avgId = minId = size-1;
  for (int i = 0, pixels = 0; i < size; ++i, pixelsPtr++)
  {
    PREFETCH_DATA(64, pixelsPtr);
    pixels += *pixelsPtr;
    if (pixels > lowThreshold)
    {
      lowBright = getBright(i, pixels, *pixelsPtr, size, lowThreshold);
      lowThreshold = 0x7FFFFFFF;
      minId = i;
    }
    if (pixels > avgThreshold)
    {
      avgBright = getBright(i, pixels, *pixelsPtr, size, avgThreshold);
      //avgThreshold = 0x7FFFFFFF;
      avgId = i;
      break;
    }
  }
  //debug("min %d avg %d", minId, avgId);
}
#endif

float find_avg_without_outliers(unsigned *__restrict histogram, int size, float minFractionSum, float maxFractionSum,
  float currentTarget)
{
  float sumWithoutOutliers = 0, pixels = 0;
  unsigned *__restrict pixelsPtr = histogram;

  for (int i = 0; i < size; ++i, pixelsPtr++)
  {
    float localValue = *pixelsPtr;

    // remove outlier at lower end
    float sub = min(localValue, minFractionSum);
    localValue = localValue - sub;
    minFractionSum -= sub;
    maxFractionSum = max(0.f, maxFractionSum - sub);

    // remove outlier at upper end
    localValue = min(localValue, maxFractionSum);
    maxFractionSum -= localValue;

    float luminanceAtBucket = float(i) / (size - 1); // linear buckets

    sumWithoutOutliers += luminanceAtBucket * localValue;
    pixels += localValue;
  }

  return pixels < 1 ? currentTarget : sumWithoutOutliers / max(0.0001f, pixels);
}

float DemonPostFx::ExposureValues::adaptValue(float du, float dd, float prev, float next)
{
  float delta = next > prev ? du : dd;
  return (prev * (1.f - delta) + next * delta);
}
void DemonPostFx::ExposureValues::adapt(float du, float dd, const ExposureValues &next)
{
  lowBright = adaptValue(du, dd, lowBright, next.lowBright);
  avgBright = adaptValue(du, dd, avgBright, next.avgBright);
}

void DemonPostFx::update(float dt) { timePassed += dt; }

float keyValueMul = 2;
float keyValueMax = 0;

void DemonPostFx::getAdaptation()
{
  TIME_D3D_PROFILE(getAdaptation)
  if (pauseAdaptationUpdate)
    return;
  if (!useAdaptation)
  {
    float currentScale = 0.5f * (current.adaptMin + current.adaptMax);
    float newOverbright = ::hdr_max_overbright / currentScale;
    ShaderGlobal::set_int_fast(adaptationOnGVarId, 0);
    ShaderGlobal::set_real_fast(adaptation_scaleGVarId, currentScale);
    ShaderGlobal::set_real_fast(adaptation_minGVarId, 0);
    ShaderGlobal::set_real_fast(max_hdr_overbrightGVarId, ::hdr_max_overbright);
    ShaderGlobal::set_real_fast(hdr_overbrightGVarId, currentOverbright[queryTail] = newOverbright);
    return;
  }

  static unsigned char *lowresBrightness1Data;
  static int lowresBrightness1Stride;

  int max_pixels = 0;
  float useCurrentOverbright;
  int totalPixels = 0;
  if (use_queries)
  {
    if (!histogramQueries[queryHead][0])
    {
      ShaderGlobal::set_int_fast(adaptationOnGVarId, 0);
      ShaderGlobal::set_real_fast(adaptation_scaleGVarId, 1);
      ShaderGlobal::set_real_fast(adaptation_minGVarId, 0);
      ShaderGlobal::set_real_fast(hdr_overbrightGVarId, currentOverbright[queryTail] = ::hdr_max_overbright);
      return;
    }
    useCurrentOverbright = currentOverbright[queryHead] / ::hdr_max_overbright;
    useCurrentOverbright = max(useCurrentOverbright, 0.00001f);
    max_pixels = lowResSize.x * lowResSize.y;
    String hist;
    for (int queryN = 0; queryN < histogramColumns - 1; ++queryN)
    {
      int pixelCount = d3d::driver_command(DRV3D_COMMAND_GETVISIBILITYCOUNT, histogramQueries[queryHead][queryN], (void *)false, 0);
      if (pixelCount < 0)
      {
        histogramQueriesGot[queryHead] = false;
        // debug("not ready");
        return;
      }
      int darkerPixels = max_pixels - pixelCount;
      histogram[queryN] = darkerPixels - totalPixels;
      totalPixels += histogram[queryN];
    }
    histogram[histogramColumns - 1] = max_pixels - totalPixels;
    histogramQueriesGot[queryHead] = true;
    queryHead++;
    queryHead %= MAX_QUERY;
  }
  else
  {
    if (histogramVb)
    {
      if (currentHistogramTexture < histogramTexture.size())
      {
        ShaderGlobal::set_int_fast(adaptationOnGVarId, 0);
        ShaderGlobal::set_real_fast(adaptation_scaleGVarId, 1);
        ShaderGlobal::set_real_fast(adaptation_minGVarId, 0);
        ShaderGlobal::set_real_fast(hdr_overbrightGVarId, currentOverbright[queryTail] = ::hdr_max_overbright);
        return;
      }
      int lockIdx = currentHistogramTexture % histogramTexture.size();
      if (d3d::get_event_query_status(histogramTextureEvent[lockIdx].get(), (++histogramLockCounter) > 10))
      {
        histogramLockCounter = 0;
        float *data = NULL;
        int stride;
        if (histogramTexture[lockIdx]->lockimg((void **)&data, stride, 0, TEXLOCK_READ | TEXLOCK_RAWDATA))
        {
          G_VERIFY((stride & 3) == 0);
          stride /= 4;
          max_pixels = lowResSize.x * lowResSize.y;
          float maxHistValue = max_pixels * 0.99f; // we do not want to see more than 75% in each bin, fights with inf
          for (int i = 0; i < histogramColumns; ++i, data++)
          {
            histogram[i] = (int)min(maxHistValue, data[0]);
            totalPixels += histogram[i];
          }
          histogramTexture[lockIdx]->unlockimg();
          // currentHistogramTexture++;
          queryTail = lockIdx;
          useCurrentOverbright = currentOverbright[queryTail] / ::hdr_max_overbright;
          useCurrentOverbright = max(useCurrentOverbright, 0.00001f);

          histogramTextureCopied[lockIdx] = false;
        }
        else
        {
          logerr("can't lock adaptation texture!");
          histogramTextureCopied[lockIdx] = false;
          return;
        }
      }
      else
      {
        if (histogramLockCounter > 10)
          logerr("can't get event status!");
        return;
      }
    }
    else
    {
      if (!lowresBrightness2Valid)
      {
        // lock only once per life. Although it is unsafe (cache issues), it is faster, and most of the times cache is already trashed
        // since last update
        BEGIN_LAG(postfx_getHistogramTexCPU_lockimg);
        lowresBrightness1->lockimg((void **)&lowresBrightness1Data, lowresBrightness1Stride, 0, TEXLOCK_DEFAULT);
        lowresBrightness1->unlockimg();
        lowresBrightness2Valid = true;
        END_LAG(postfx_getHistogramTexCPU_lockimg);
      }

      useCurrentOverbright = currentOverbright[queryTail] / ::hdr_max_overbright;
      useCurrentOverbright = max(useCurrentOverbright, 0.00001f);
      PROFILE_START("getHistCPU");

      void *ptr;
      int stride;

      BEGIN_LAG(postfx_getHistogramTexCPU_lockimg);
      lowresBrightness1->lockimg(&ptr, stride, 0, TEXLOCK_DEFAULT);
      END_LAG(postfx_getHistogramTexCPU_lockimg);

      if (ptr)
      {
        getHistogramTexCPU<unsigned>((unsigned char *)ptr, lowResSize.x, lowResSize.y, stride, histogram, histogramColumnsBits,
          max_pixels, totalPixels);
        d3d_err(lowresBrightness1->unlockimg());
      }
    }
    PROFILE_END();
  }

  // debug("hist columns = %d(1<<%d)",
  //   histogramColumns, histogramColumnsBits);
  ExposureValues exposure;
  exposure.avgBright = find_avg_without_outliers(histogram, histogramColumns, current.lum_min * totalPixels,
    current.lum_max * totalPixels, current.targetBrightness / max(0.001f, useCurrentOverbright));
  exposure.avgBright *= useCurrentOverbright;

  if (perceptualAdaptation.get())
    exposure.avgBright = lerp(1.f, pow(exposure.avgBright, current.perceptionPower), current.perceptionLinear);

  if (framesBeforeValidHistogram > 0)
    framesBeforeValidHistogram--;
  lastHistogramAvgBright = exposure.avgBright;

  // debug("keyValue = %g exposure.avgBright = %g", keyValue, exposure.avgBright);

  float timeUp = current.adaptTimeUp;
  if (!float_nonzero(timeUp))
    timeUp = timePassed;

  float timeDown = current.adaptTimeDown;
  if (!float_nonzero(timeDown))
    timeDown = timePassed;

  float du = min(timePassed / timeUp, 1.f);
  float dd = min(timePassed / timeDown, 1.f);

  currentExposure.adapt(du, dd, exposure);
  // currentExposure = exposure;

  float lowAdaptScale = 1.0f;
  if (currentExposure.adaptationScale > 1.0)
    lowAdaptScale = safediv(1.0f, currentExposure.adaptationScale);
  float targetBrightness = current.targetBrightness;
  if (current.targetBrightnessUp > 0 && current.targetBrightnessEffect > 0)
  {
    // https://knarkowicz.wordpress.com/2016/01/09/automatic-exposure/
    // http://resources.mpi-inf.mpg.de/hdr/peffects/krawczyk05sccg.pdf
    targetBrightness =
      current.targetBrightnessUp - current.targetBrightnessEffect / (logf(exposure.avgBright + 1.f) + current.targetBrightnessEffect);
  }
  if (use_queries) ///< brightness hack, for smaller histogramm
    targetBrightness *= 0.93 + 0.07 / 255.0 * histogramColumns;
  float currentScale = targetBrightness / max(currentExposure.avgBright, 0.00001f);

  if (currentScale > current.adaptMax)
    currentScale = current.adaptMax;
  if (currentScale < current.adaptMin)
    currentScale = current.adaptMin;
  currentExposure.adaptationScale = currentScale;
  ShaderGlobal::set_real_fast(adaptation_scaleGVarId, currentScale);
  ShaderGlobal::set_real_fast(adaptation_minGVarId, 0);
  ShaderGlobal::set_int_fast(adaptationOnGVarId, 1);
  // debug("head=%d, tail=%d currentScale=%g min=%g",
  //   queryHead, queryTail, currentScale, lowBright*currentScale*currentScale/oldScale);

  float newOverbright = ::hdr_max_overbright / currentScale;
  // if (fabsf(newOverbright - useCurrentOverbright) < 0.01f)
  //   newOverbright = useCurrentOverbright;

  currentOverbright[queryTail] = ShaderGlobal::get_real_fast(hdr_overbrightGVarId);

  ShaderGlobal::set_real_fast(hdr_overbrightGVarId, newOverbright);
}

void DemonPostFx::startGetAdaptation()
{
  TIME_D3D_PROFILE(startGetAdaptation)
  if (pauseAdaptationUpdate)
    return;
  // static int queriesVarId = ::get_shader_glob_var_id("use_queries", true);
  // int columnsBits = queriesVarId>=0 ? ShaderGlobal::get_int_fast(queriesVarId) : 0;
  // use_queries = columnsBits;
  if (!use_queries)
  {
    if (histogramVb)
    {
      if (useCenterWeightedAdaptation.get() != centerWeightedAdaptation)
        setCenterWeightedAdaptation(useCenterWeightedAdaptation.get());

      int lockIdx = currentHistogramTexture % histogramTexture.size();
      if (currentHistogramTexture >= histogramTexture.size() && histogramTextureCopied[lockIdx])
        return;
      d3d::resource_barrier({lowresBrightness1.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::set_render_target();
      d3d::set_render_target(0, histogramInterlacedTexture.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 1.0, 0);
      d3d::setvsrc(0, histogramVb.getBuf(), sizeof(uint16_t) * 2);
      d3d::settex_vs(1, lowresBrightness1.getTex2D());
      histogramCalcElement->setStates(0, true);
      {
        TIME_D3D_PROFILE(draw)
#if HIST_USE_INSTANCING
        d3d::draw_instanced(PRIM_POINTLIST, 0, 1,
          centerWeightedAdaptation ? CENTER_WEIGHTED_ADAPTATION_SIZE : lowResSize.x * lowResSize.y);
#else
        d3d::draw(PRIM_POINTLIST, 0, centerWeightedAdaptation ? CENTER_WEIGHTED_ADAPTATION_SIZE : lowResSize.x * lowResSize.y);
#endif
      }
      d3d::resource_barrier({histogramInterlacedTexture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      {
        TIME_D3D_PROFILE(accum)
        d3d::set_render_target(0, histogramTexture[lockIdx].getTex2D(), 0);
        d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
        histogramAccum.render();
      }
      d3d::settex_vs(1, NULL);
      {
        TIME_D3D_PROFILE(copy)
        int stride;
        if (histogramTexture[lockIdx]->lockimg(NULL, stride, 0, TEXLOCK_READ | TEXLOCK_RAWDATA))
          histogramTexture[lockIdx]->unlockimg();
      }
      d3d::issue_event_query(histogramTextureEvent[lockIdx].get());
      histogramTextureCopied[lockIdx] = true;
      currentHistogramTexture++;
    }
    else
    {
#if _TARGET_PC
      int stride = 0;
      lowresBrightness1->lockimg(NULL, stride, 0, TEXLOCK_DEFAULT); // delayed lock
#endif
    }
    histogramColumnsBits = MAX_HISTOGRAM_COLUMNS_BITS;
    histogramColumns = 1 << histogramColumnsBits;
    return;
  }
  histogramColumnsBits = 5; // columnsBits;
  histogramColumns = 1 << histogramColumnsBits;
  if (!histogramQueriesGot[queryTail] && histogramQueries[queryTail][0])
    return;
  if (thresholdVarId < 0)
    thresholdVarId = ::get_shader_variable_id("threshold");
  d3d::set_render_target(prevFrameLowResTex.getTex2D(), 0);

  for (int queryN = 0; queryN < histogramColumns - 1; ++queryN)
  {
    d3d::driver_command(DRV3D_COMMAND_GETVISIBILITYBEGIN, &histogramQueries[queryTail][queryN], 0, 0);
    // histogramQuery.getMat()->set_real_param(thresholdVarId, (queryN+1.0)/(histogramColumns-1.0));//histogram of adapted image
    float threshold = (queryN + 1) / (histogramColumns - 1.0);
    histogramQuery.getMat()->set_real_param(thresholdVarId, threshold); // histogram of adapted image
    histogramQuery.render();
    d3d::driver_command(DRV3D_COMMAND_GETVISIBILITYEND, histogramQueries[queryTail][queryN], 0, 0);
  }
  histogramQueriesGot[queryTail] = false;
  queryTail++;
  queryTail %= MAX_QUERY;
}

void DemonPostFx::prepareSkyMask(const TMatrix &view_tm)
{
  if (useSimpleMode)
    return;
  Point3 sunDir = view_tm % current.sunDir;
  float sunLightK = sunDir.z;

  real minCos = volfogMaxAngleCos;
  if (minCos < 0.0004f)
    minCos = 0.0004f;

  TIME_D3D_PROFILE(prepareSkyMask);

  G_ASSERT(volFogCallback);
  if (sunLightK >= minCos && volFogCallback)
  {
    d3d::set_render_target(lowResLumTexB.getTex2D(), 0);
    volFogCallback->process(lowResSize.x, lowResSize.y, calcQuadCoeffs(lowResSize.x, lowResSize.y));
    skyMaskPrepared = true;
  }
}

// eye -1 for mono, 0,1 for stereo
void DemonPostFx::apply(bool vr_mode, Texture *target_tex, TEXTUREID target_id, Texture *output_tex, TEXTUREID output_id,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, Texture *depth_tex, int eye, int target_layer, const Point4 &target_uv_transform,
  const RectInt *output_viewport)
{
  G_UNREFERENCED(output_id);
#if DAGOR_DBGLEVEL == 0
  current.debugFlags = 0;
#endif
  if (::grs_draw_wire)
    d3d::setwire(0);

  SCOPE_RENDER_TARGET;

  TextureInfo info;
  target_tex->getinfo(info);
  int targtexW = info.w;
  int targtexH = info.h;
  target_tex->texfilter(TEXFILTER_SMOOTH);

  real sunLightK = 0;


  if (!useSimpleMode)
  {
#if __CHECK_ADAPTATION__ > 1
    d3d::set_render_target(target_tex, 0);
    int w = targtexW, h = targtexH;
    d3d::clearview(CLEAR_TARGET, 0, 1, 0);

    d3d::setview(0, h * 1 / 5, w, 1, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
    d3d::setview(0, h * 2 / 5, w, 2, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
    d3d::setview(0, h * 3 / 5, w, 3, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
    d3d::setview(0, h * 4 / 5, w, 7, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);

    d3d::setview(w * 1 / 5, 0, 1, h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
    d3d::setview(w * 2 / 5, 0, 2, h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
    d3d::setview(w * 3 / 5, 0, 3, h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
    d3d::setview(w * 4 / 5, 0, 7, h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1, 0);
#endif

    // capture low-res image
    if (eye <= 0)
      getAdaptation();

    d3d::resource_barrier({target_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (prevFrameLowResTex.getTex2D())
      downsample(prevFrameLowResTex.getTex2D(), target_id, targtexW, targtexH, target_uv_transform);

    // read from already downsampled tex
    if (eye < 0)
      applyLenseFlare(prevFrameLowResTex);

    if (useAdaptation && eye <= 0)
    {
      {
        TIME_D3D_PROFILE(luminance);
        d3d::resource_barrier({prevFrameLowResTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
        d3d::set_render_target(lowresBrightness1.getTex2D(), 0);
        d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
        luminance.getMat()->set_real_param(columnsVarId, use_queries ? 255 : histogramColumns - 1); // histogram of adapted image
        luminance.render();
      }
      startGetAdaptation();
    }

    // reset adaptation value
    if (useAdaptation && adaptationResetValue > 0 && framesBeforeValidHistogram == 0 && eye <= 0)
    {
      // debug("reset adaptation to: %.3f", adaptationResetValue);
      // fixme: reset whole adaptation
      currentExposure.avgBright = lastHistogramAvgBright * adaptationResetValue;
      currentExposure.adaptationScale = adaptationResetValue;
      currentExposure.lowBright = 0;
      ShaderGlobal::set_real_fast(adaptation_scaleGVarId, adaptationResetValue);
      ShaderGlobal::set_real_fast(adaptation_minGVarId, 0);
      currentOverbright[queryTail] = ::hdr_max_overbright / adaptationResetValue;
      ShaderGlobal::set_real_fast(hdr_overbrightGVarId, currentOverbright[queryTail]);
      adaptationResetValue = -1;
    }

    // apply glow color curve
    bool hasGlow = !(current.debugFlags & current.NO_GLOW);
    if (::hdr_render_mode != HDR_MODE_FLOAT && current.hdrDarkThreshold > ::hdr_max_overbright)
      hasGlow = false;

    if (current.hdrGlowMul <= 0.0)
    {
      hasGlow = false;
    }

    ShaderGlobal::set_real_fast(hasPostFxGlowVarId, hasGlow ? 1 : 0);
    if (current.hdrGlowMul * current.hdrGlowPower > 0 && hasGlow)
    {
      if (useCompute)
        calcGlowCompute();
      else
        calcGlowGraphics();
    }
    else if (glowTex.getTex2D())
    {
      TIME_D3D_PROFILE(clearGlow);
      if (useCompute)
      {
        const float zero[4] = {0, 0, 0, 0};
        d3d::clear_rwtexf(glowTex.getTex2D(), zero, 0, 0);
      }
      else
      {
        d3d::set_render_target(glowTex.getTex2D(), 0);
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      }
    }

    // radial blur (fake volumetric light from sun)
    Point3 sunDir = view_tm % current.sunDir;
    real wk, hk;

    wk = proj_tm(0, 0);
    hk = proj_tm(1, 1);

    sunLightK = sunDir.z;

    real minCos = volfogMaxAngleCos;
    if (minCos < 0.0004f)
      minCos = 0.0004f;

    // Here we disable the radial blur based volfog emulation for VR. The method looks good enough indeed
    // but in VR, the results are simply too inconsistent between the eyes, especially when there are
    // close things on the screen, like hands or cockpit.
    float volFogBrightness = current.volfogMul * rgbsum(current.volfogColor) * (vr_mode ? 0 : 1);
    G_ASSERT(volFogCallback);
    if (sunLightK >= minCos && volFogCallback && !(current.debugFlags & current.NO_VFOG) && volFogBrightness > 0.00001f)
    {
      if (!skyMaskPrepared)
        prepareSkyMask(view_tm);

      sunLightK = (sunLightK - minCos) / (1 - minCos);
      sunLightK *= current.volfogMul;

      real sunX = 0.5f + 0.5f * sunDir.x * wk / sunDir.z;
      real sunY = 0.5f - 0.5f * sunDir.y * hk / sunDir.z;

      // perform radial blur
      lowResLumTexA->texaddr(TEXADDR_BORDER);
      lowResLumTexA->texbordercolor(0);
      lowResLumTexB->texaddr(TEXADDR_BORDER);
      lowResLumTexB->texbordercolor(0);
      d3d::set_render_target(lowResLumTexA.getTex2D(), 0);

      for (int i = 0; i < BLUR_SAMPLES; ++i)
      {
        real s = powf(1 / current.volfogRange, (i + 1) / real(BLUR_SAMPLES * BLUR_SAMPLES * BLUR_SAMPLES));

        radialBlur1Fx.getMat()->set_color4_param(texTmVarId[i], Color4(s, s, sunX - sunX * s, sunY - sunY * s));
      }

      radialBlur1Fx.getMat()->set_texture_param(texVarId, lowResLumTexB.getTexId());
      {
        TIME_D3D_PROFILE(radialBlur1Fx);
        radialBlur1Fx.render();
      }

      d3d::set_render_target(lowResLumTexB.getTex2D(), 0);

      for (int i = 0; i < BLUR_SAMPLES; ++i)
      {
        real s = powf(1 / current.volfogRange, (i + 1) / real(BLUR_SAMPLES * BLUR_SAMPLES));

        radialBlur1Fx.getMat()->set_color4_param(texTmVarId[i], Color4(s, s, sunX - sunX * s, sunY - sunY * s));
      }

      radialBlur1Fx.getMat()->set_texture_param(texVarId, lowResLumTexA.getTexId());
      {
        TIME_D3D_PROFILE(radialBlur1Fx);
        radialBlur1Fx.render();
      }

      d3d::set_render_target(lowResLumTexA.getTex2D(), 0);

      for (int i = 0; i < BLUR_SAMPLES; ++i)
      {
        real s = powf(1 / current.volfogRange, i / real(BLUR_SAMPLES));

        radialBlur1Fx.getMat()->set_color4_param(texTmVarId[i], Color4(s, s, sunX - sunX * s, sunY - sunY * s));
      }

      radialBlur1Fx.getMat()->set_texture_param(texVarId, lowResLumTexB.getTexId());
      {
        TIME_D3D_PROFILE(radialBlur1Fx);
        radialBlur1Fx.render();
      }
      lowResLumTexA->texaddr(TEXADDR_CLAMP);
      lowResLumTexB->texaddr(TEXADDR_CLAMP);
      skyMaskPrepared = false;
    }
    else
      sunLightK = 0;
  }

  Color4 quad_coefs = calcQuadCoeffs(lowResSize.x, lowResSize.y);

  // set up combine params
  if (!useSimpleMode)
  {
    combineFx.getMat()->set_texture_param(glowTexVarId, glowTex.getTexId());
    combineFx.getMat()->set_texture_param(volfogTexVarId, lowResLumTexA.getTexId());
    combineFx.getMat()->set_color4_param(glowTexSzVarId, quad_coefs);

    float glowPower = current.hdrGlowPower;

    combineFx.getMat()->set_color4_param(glowScaleVarId, Color4(1, 1, 1) * (glowPower >= 1 ? 1 : glowPower));

    combineFx.getMat()->set_color4_param(volfogScaleVarId,
      Color4(current.volfogColor.r, current.volfogColor.g, current.volfogColor.b) * sunLightK);
    if (volFogOnVarId >= 0)
      combineFx.getMat()->set_int_param(volFogOnVarId, sunLightK * rgbsum(current.volfogColor) > 0.00001f ? 1 : 0);
  }

  if (colorMatrixRVarId >= 0)
  {
    TMatrix ctm = postColorMatrix * paramColorMatrix * preColorMatrix;

    Color4 colorMatrixR(ctm[0][0], ctm[1][0], ctm[2][0], ctm[3][0]);
    Color4 colorMatrixG(ctm[0][1], ctm[1][1], ctm[2][1], ctm[3][1]);
    Color4 colorMatrixB(ctm[0][2], ctm[1][2], ctm[2][2], ctm[3][2]);

    if (useLutTexture)
    {
      render2lut.getMat()->set_color4_param(colorMatrixRVarId, colorMatrixR);
      render2lut.getMat()->set_color4_param(colorMatrixGVarId, colorMatrixG);
      render2lut.getMat()->set_color4_param(colorMatrixBVarId, colorMatrixB);
    }

    combineFx.getMat()->set_color4_param(colorMatrixRVarId, colorMatrixR);
    combineFx.getMat()->set_color4_param(colorMatrixGVarId, colorMatrixG);
    combineFx.getMat()->set_color4_param(colorMatrixBVarId, colorMatrixB);
  }

  // render to LUT
  if (useLutTexture)
  {
    TIME_D3D_PROFILE(render2lut);

    d3d::set_render_target(lutTex2d.getTex2D(), 0);
    render2lut.render();

    combineFx.getMat()->set_texture_param(lutTexVarId, lutTex3d.getTexId());
  }

  // combine result to output
  if (current.debugFlags & current.NO_SCENE)
  {
    TIME_D3D_PROFILE(clearTarget);
    d3d::set_render_target(target_tex, 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  }

  if (!useSimpleMode)
  {
    if (glowTex.getTex2D())
      d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (lowResLumTexA.getTex2D())
      d3d::resource_barrier({lowResLumTexA.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  if (output_tex)
    d3d::set_render_target(output_tex, output_tex->restype() == RES3D_ARRTEX ? target_layer : 0, 0);
  else
    d3d::set_render_target();
  d3d::set_depth(depth_tex, DepthAccess::SampledRO);

  if (output_viewport)
  {
    d3d::setview(output_viewport->left, output_viewport->top, output_viewport->right - output_viewport->left,
      output_viewport->bottom - output_viewport->top, 0, 1);
    d3d::setscissor(output_viewport->left, output_viewport->top, output_viewport->right - output_viewport->left,
      output_viewport->bottom - output_viewport->top);
  }

  d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);

  combineFx.getMat()->set_texture_param(texVarId, target_id);
  combineFx.getMat()->set_color4_param(texUvTransformVarId, reinterpret_cast<const Color4 &>(target_uv_transform));
  int outW, outH;
  d3d::get_target_size(outW, outH);
  combineFx.getMat()->set_color4_param(texSzVarId, calcQuadCoeffs(outW, outH));
  if (vignette_multiplierGlobVarId >= 0)
    ShaderGlobal::set_real_fast(vignette_multiplierGlobVarId, vignetteMul);
  if (vignette_start_endGlobVarId >= 0)
    ShaderGlobal::set_color4_fast(vignette_start_endGlobVarId, Color4(vignetteStart.x, vignetteEnd.x, vignetteStart.y, vignetteEnd.y));

  ShaderGlobal::set_real(ldr_vignette_multiplierVarId, ldrVignetteMul);
  ShaderGlobal::set_real(ldr_vignette_widthVarId, ldrVignetteWidth);
  ShaderGlobal::set_real(ldr_vignette_aspectVarId, float(targtexW) / targtexH);

  {
    TIME_D3D_PROFILE(combineFx);
    combineFx.render();
  }

  combineFx.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);

  if (!useSimpleMode && useAdaptation)
  {
    // combine low-res version of final image for adaptation
    // (most likely faster than downsampling large final image)
    if (showHistogram)
    {
      drawHistogram(histogram, lowResSize.x * lowResSize.y, Point2(0.75, 0.99), Point2(0.99, 0.75));
      if (output_tex)
      {
        drawHistogram(output_tex, 1, Point2(0.75, 0.75), Point2(0.99, 0.5));
      }
      else
      {
        d3d::stretch_rect(NULL, histogramTex.getTex2D());
        drawHistogram(histogramTex.getTex2D(), 1, Point2(0.75, 0.75), Point2(0.99, 0.5));
      }
    }

#if defined(__CHECK_ADAPTATION__)
    debug("lowres average =%g\n"
          "chain0 average =%g\n"
          "chain1 average =%g\n"
          "chain2 average =%g\n"
          "chain3 average =%g\n",
      get_average(::hdr_render_mode == HDR_MODE_FAKE ? finalImageForFakeHDR : prevFrameLowResTex), get_average(downsampleTex[0]),
      get_average(downsampleTex[1]), get_average(downsampleTex[2]), get_average(downsampleTex[3]));
    debug("adaptation cur=%g, last = %g", get_average(curAdaptTex), get_average(lastAdaptTex));
#if _TARGET_PC
    save_rt_image_as_tga(target_tex, "target_tex.tga");
    save_rt_image_as_tga(prevFrameLowResTex, "lowres_target.tga");
    if (::hdr_render_mode == HDR_MODE_FAKE)
      save_rt_image_as_tga(finalImageForFakeHDR, "final_lowres.tga");
    for (int i = 0; i < NUM_DOWNSAMPLE_TEX; ++i)
      save_rt_image_as_tga(downsampleTex[i], String(128, "chain%d.tga", i));
#endif
#endif
  }

  if (::grs_draw_wire)
    d3d::setwire(::grs_draw_wire);

  timePassed = 0;
}

void DemonPostFx::delayedCombineFx(TEXTUREID textureId)
{
  // combine result to output
  ShaderGlobal::set_int_fast(renderCombinePassGVarId, 2);
  combineFx.getMat()->set_texture_param(texVarId, textureId);
  combineFx.render();
  ShaderGlobal::set_int_fast(renderCombinePassGVarId, 0);
}

void DemonPostFx::removeQueries()
{
  currentHistogramTexture = 0;
  mem_set_0(histogramTextureCopied);
#if _TARGET_PC
  queryTail = queryHead = 0;
  for (int i = 0; i < MAX_QUERY; ++i)
  {
    for (int queryN = 0; queryN < histogramColumns - 1; ++queryN)
    {
      if (histogramQueries[i][queryN])
      {
        d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &histogramQueries[i][queryN], (void *)false, 0);
        histogramQueries[i][queryN] = NULL;
      }
    }
    histogramQueriesGot[i] = false;
  }
#endif
}

void DemonPostFx::onBeforeReset3dDevice() { removeQueries(); }
