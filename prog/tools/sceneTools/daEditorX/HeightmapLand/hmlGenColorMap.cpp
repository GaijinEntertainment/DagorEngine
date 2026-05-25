// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include "hmlGenColorMap.h"
#include "landClassSlotsMgr.h"

#include <de3_hmapService.h>
#include <de3_landClassData.h>
#include <de3_assetService.h>
#include <de3_hmapDetLayerProps.h>
#include <assets/asset.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>

#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include <util/dag_stlqsort.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>
#include <coolConsole/coolConsole.h>
#include <EditorCore/ec_IEditorCore.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_parallelForInline.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <math/dag_intrin.h>
#include "blockedDetTexMap.h"
#include <debug/dag_debug.h>
#include <landClassEval/lcExprFusion.h>
#include <landClassEval/lcExprGenLayerConvert.h>
#include "hmlExprFactory.h"
#include "hmlBakeExpr.h"

// libTools/propPanel/commonWindow private header, surfaced through a
// HeightmapLand-only AddIncludes entry. Used by build_curve_sampler to
// compute cubic-polynom / hermite-spline coefficients from artist control
// points without reimplementing the math.
#include <w_curve_math.h>
#include "smooth_noise.h"
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/bitset.h>
#include <math/dag_Point2.h>
#include <stdio.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <tchar.h>
#undef ERROR
#else
#define _T(X) "" X
#endif

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

using hdpi::_pxScaled;

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

// Editor-local builtins registered via hml_lcexpr_factory on top of the gameLibs
// landClassEval defaults. See init() below.
//
// Per-octave domain transform shared by both fbm variants: rotate the sample point by
// ~36.87 degrees (cos = 0.8, sin = 0.6) and scale by 2. det = 4 = 2^2, so frequency
// doubles each octave; the rotation keeps the 64-cell LUT grid from realigning with
// itself across octaves, which is what produces visible "noise grid" artefacts.
// Matches the matrix used by gameLibs/heightmap/heightProvider/vtexElevation.cpp so a
// single named function would render the same height field on either path.
static inline void lcexpr_fbm_step(double &x, double &y)
{
  const double nx = x * 1.6 - y * 1.2;
  const double ny = x * 1.2 + y * 1.6;
  x = nx;
  y = ny;
}

// Plain value-noise FBM: sum of octaves of noise(p) at doubling frequencies / halving
// amplitudes, normalised to ~[0, 1]. fbm needs a loop, so the body lives in a helper
// that the node's expression calls -- same shape as SqrtNode/PowNode in lcExprNodes.h
// calling sqrtf/powf.
static float lcexpr_fbm_impl(float x_, float y_, float octavesF)
{
  int oct = (int)octavesF;
  if (oct < 1)
    oct = 1;
  else if (oct > 8)
    oct = 8;
  double x = x_, y = y_;
  double amp = 1.0, totalAmp = 0.0, sum = 0.0;
  for (int i = 0; i < oct; i++)
  {
    sum += amp * smooth_noise::noise_value_2d(DPoint2(x, y));
    totalAmp += amp;
    lcexpr_fbm_step(x, y);
    amp *= 0.5;
  }
  return totalAmp > 0.0 ? (float)(sum / totalAmp) : 0.f;
}

// Slope-attenuated FBM (after Inigo Quilez's "FBM with derivatives" / used by
// vtexElevation.cpp): same accumulation as plain fbm, but each octave's contribution
// is divided by 1 + |accumulated gradient|^2. Steep regions get LESS detail, which
// produces a natural eroded / weathered look on terrain.
//
// smooth_noise::noise() returns vec4f (value, dvalue/dx, dvalue/dy, _) -- we read all
// three lanes and accumulate the gradient across octaves.
static float lcexpr_eroded_fbm_impl(float x_, float y_, float octavesF)
{
  int oct = (int)octavesF;
  if (oct < 1)
    oct = 1;
  else if (oct > 16)
    oct = 16;
  double x = x_, y = y_;
  double amp = 1.0, totalAmp = 0.0, sum = 0.0;
  double accDx = 0.0, accDy = 0.0;
  for (int i = 0; i < oct; i++)
  {
    vec4f n = smooth_noise::noise(DPoint2(x, y));
    alignas(16) float lanes[4];
    v_st(lanes, n);
    accDx += lanes[1];
    accDy += lanes[2];
    const double atten = 1.0 / (1.0 + accDx * accDx + accDy * accDy);
    sum += amp * lanes[0] * atten;
    totalAmp += amp;
    lcexpr_fbm_step(x, y);
    amp *= 0.5;
  }
  return totalAmp > 0.0 ? (float)(sum / totalAmp) : 0.f;
}

// step(edge, x): s0=edge, s1=x
HML_LC_USER_BINARY(LcExprStepNode, s1 >= s0 ? 1.f : 0.f);
HML_LC_USER_BINARY(LcExprLength2Node, sqrtf(s0 *s0 + s1 * s1));
HML_LC_USER_BINARY(LcExprSmoothNoiseNode, smooth_noise::noise_value_2d(DPoint2(s0, s1)));
HML_LC_USER_TERNARY(LcExprFbmNode, lcexpr_fbm_impl(s0, s1, s2));
HML_LC_USER_TERNARY(LcExprErodedFbmNode, lcexpr_eroded_fbm_impl(s0, s1, s2));

// curve(c, x): c is a Curve-typed external var holding an integer slot index
// (stored as float in vars[c.varId]); x is a float. The curves table itself
// is owned by HmapLandPlugin::evalCurves and threaded through evaluation via
// EvalCtx::userCtx, so the eval here is a Tab<CubicCurveSampler>* lookup +
// CubicCurveSampler::sample. Out-of-range indices and a missing userCtx both
// return 0 to keep eval finite -- the editor surfaces a typedVars-out-of-sync
// warning at recompile time, this is just a runtime safety floor.
struct LcExprCurveNode : lcexpr::EvalNode
{
  uint32_t n0; // curve var (resolves to vars[curveVarId] = float-encoded index)
  uint32_t n1; // x argument
  float eval(const lcexpr::EvalCtx &ctx) const override
  {
    const float idxF = ctx.at(n0)->eval(ctx);
    const float x = ctx.at(n1)->eval(ctx);
    if (!ctx.userCtx)
      return 0.f;
    const Tab<CubicCurveSampler> *curves = static_cast<const Tab<CubicCurveSampler> *>(ctx.userCtx);
    const int idx = (int)idxF;
    if (idx < 0 || idx >= curves->size())
      return 0.f;
    return (*curves)[idx].sample(x);
  }
};

// LcExprContext / g_lcExpr / lcexpr_compute_var_mask_bitset / compile_expression
// moved to hmlBakeExpr.h / .cpp. init() defined below because it registers the
// LcExpr*Node types declared in this TU.
void LcExprContext::init()
{
  if (inited)
    return;
  inited = true;
  parseMap = lcexpr::make_default_func_parse_map();
  emitMap = lcexpr::make_default_node_emit_map();
  numFusionRules = lcexpr::make_default_fusion_rules(fusionRules, 16);

  hml_lcexpr_factory::register_binary(parseMap, emitMap, "step", &lcexpr::emitBinaryFn<LcExprStepNode>);
  hml_lcexpr_factory::register_binary(parseMap, emitMap, "length2", &lcexpr::emitBinaryFn<LcExprLength2Node>);
  hml_lcexpr_factory::register_binary(parseMap, emitMap, "noise", &lcexpr::emitBinaryFn<LcExprSmoothNoiseNode>);
  hml_lcexpr_factory::register_ternary(parseMap, emitMap, "fbm", &lcexpr::emitTernaryFn<LcExprFbmNode>);
  hml_lcexpr_factory::register_ternary(parseMap, emitMap, "eroded_fbm", &lcexpr::emitTernaryFn<LcExprErodedFbmNode>);
  // Typed registration: curve(c, x) requires its first argument to be Curve-
  // typed; passing a Float (or anything else) is a parse-time error. The
  // eval-time lookup uses EvalCtx::userCtx -> Tab<CubicCurveSampler>*.
  hml_lcexpr_factory::register_typed_binary(parseMap, emitMap, "curve", hml_lcexpr_factory::TYPE_CURVE, lcexpr::TYPE_FLOAT,
    lcexpr::TYPE_FLOAT, &lcexpr::emitBinaryFn<LcExprCurveNode>);
  smooth_noise::init();
}

// hmlPlugin.h declares commonExprVarMask as eastl::bitset<256> because it cannot pull
// in landClassEval headers (matching the Tab<uint8_t> aliasing of lcexpr::Arena there).
// Pin the literal to the lib's hard varId cap so the two stay in sync.
static_assert(256 == lcexpr::MAX_VAR_ID, "commonExprVarMask in hmlPlugin.h must mirror lcexpr::MAX_VAR_ID; update both together");

// Layer paramName is user-entered text. For mask_<name> to parse, the combined string
// must be a valid identifier. The `mask_` prefix starts with a letter, so we just need
// the suffix to be non-empty and contain only [A-Za-z0-9_].
static bool lcexpr_is_ident_suffix(const char *s)
{
  if (!s || !*s)
    return false;
  for (const char *p = s; *p; p++)
  {
    char c = *p;
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    if (!ok)
      return false;
  }
  return true;
}

const int PID_STATIC_OFFSET = 10000;


class ScriptParamInt : public HmapLandPlugin::ScriptParam
{
public:
  int value;
  int paramPid;

  ScriptParamInt(const char *name, int val) : ScriptParam(name), value(val), paramPid(-1) {}

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {
    paramPid = pid++;
    panel.createEditInt(paramPid, paramName, value);
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid != paramPid)
      return;
    value = panel.getInt(pid);
  }

  void save(DataBlock &blk) override { blk.setInt(paramName, value); }

  void load(const DataBlock &blk) override { value = blk.getInt(paramName, value); }
};


class ScriptParamReal : public HmapLandPlugin::ScriptParam
{
public:
  real value;
  int paramPid;

  ScriptParamReal(const char *name, real val) : ScriptParam(name), value(val), paramPid(-1) {}

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {
    paramPid = pid++;
    panel.createEditFloat(paramPid, paramName, value);
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid != paramPid)
      return;
    value = panel.getFloat(pid);
  }

  void save(DataBlock &blk) override { blk.setReal(paramName, value); }

  void load(const DataBlock &blk) override { value = blk.getReal(paramName, value); }
};


class ScriptParamColor : public HmapLandPlugin::ScriptParam
{
public:
  E3DCOLOR value;
  int paramPid;

  ScriptParamColor(const char *name, E3DCOLOR val) : ScriptParam(name), value(val), paramPid(-1) {}

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {
    paramPid = pid++;
    panel.createColorBox(paramPid, paramName, value);
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid != paramPid)
      return;
    value = panel.getColor(pid);
  }

  void save(DataBlock &blk) override { blk.setE3dcolor(paramName, value); }

  void load(const DataBlock &blk) override { value = blk.getE3dcolor(paramName, value); }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ScriptParamMask : public HmapLandPlugin::ScriptParam
{
public:
  int imageIndex, bitsPerPixel;

  int buttonPid, editButtonPid;
  bool detailed;
  int divisor;


  ScriptParamMask(const char *name, int bpp, bool det, int div = 1) :
    ScriptParam(name), buttonPid(-1), imageIndex(-1), bitsPerPixel(bpp), detailed(det), divisor(div)
  {
    imageIndex = HmapLandPlugin::self->getScriptImage(paramName, divisor, bpp); // preload image
  }

  void *get_interface(int id) override { return id == 'Mask' ? this : NULL; }

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {
    bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this;
    buttonPid = pid++;
    editButtonPid = pid++;
    panel.createIndent();
    panel.createSeparator(0);
    panel.createStatic(pid + PID_STATIC_OFFSET, String(64, "%s, %d bpp", (char *)paramName, bitsPerPixel));
    panel.setBool(pid + PID_STATIC_OFFSET, true);
    panel.createButton(buttonPid, "Import from...", false);
    panel.createButton(editButtonPid, isEditImage ? "Done" : "Edit", true, false);
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override {}

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid == buttonPid)
    {
      //== import mask here
    }
    else if (pid == editButtonPid)
    {
      ScriptParamMask *curEd = (ScriptParamMask *)HmapLandPlugin::self->getEditedScriptImage();
      if (curEd)
        curEd->finishEdit(panel);

      HmapLandPlugin::self->editScriptImage(this);
      bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this;
      panel.setCaption(pid, isEditImage ? "Done" : "Edit");
      HmapLandPlugin::self->setShowBlueWhiteMask();

      if (!isEditImage)
        HmapLandPlugin::self->setSelectMode();
    }
  }

  void save(DataBlock &main_blk) override {}

  void load(const DataBlock &main_blk) override {}

  virtual void finishEdit(PropPanel::ContainerPropertyControl &panel) { panel.setCaption(editButtonPid, "Edit"); }

  float sampleMask1(const LandColorGenData &lcg) const;
  float sampleMask8(const LandColorGenData &lcg) const;
  float sampleMask1Pixel(const LandColorGenData &lcg) const;
  float sampleMask8Pixel(const LandColorGenData &lcg) const;
  void setMask1(const LandColorGenData &lcg, bool c) const;
  void setMask8(const LandColorGenData &lcg, char c) const;

  static inline ScriptParamMask *cast(ScriptParam *es)
  {
    if (es && es->get_interface('Mask'))
      return static_cast<ScriptParamMask *>(es);
    return NULL;
  }
};


class ScriptParamImage : public ScriptParamMask
{
public:
  SimpleString imageName;
  int mappingType;
  Point2 tile, offset;
  bool clampU, clampV, flipU, flipV, swapUV, showMask;
  int detailType;
  IHmapBrushImage::Channel channel;

  int mappingTypePid, tilePid, offsetPid, clampUPid, clampVPid, showMaskPid, flipUPid, flipVPid, swapUVPid, detailTypePid,
    channelsComboPid;


  enum
  {
    MAPPING_HMAP,
    MAPPING_HMAP_PERCENT,
    MAPPING_WORLD,
    MAPPING_VERT_U,
    MAPPING_VERT_V,
  };

  struct DefParams
  {
    SimpleString imageName;
    int mappingType, detailType;
    Point2 tile, offset;
    bool clampU, clampV, flipU, flipV, swapUV;

    DefParams() :
      mappingType(MAPPING_HMAP),
      tile(100, 100),
      offset(0, 0),
      clampU(false),
      clampV(false),
      flipU(false),
      flipV(false),
      swapUV(false),
      detailType(0)
    {}
  };


  ScriptParamImage(const char *name, const DefParams &def) :
    ScriptParamMask(name, 32, false, 1),
    imageName(def.imageName),
    mappingType(def.mappingType),
    tile(def.tile),
    offset(def.offset),
    clampU(def.clampU),
    clampV(def.clampV),
    flipU(def.flipU),
    flipV(def.flipV),
    swapUV(def.swapUV),
    detailType(def.detailType),
    channel(IHmapBrushImage::CHANNEL_RGB)
  {}

  void *get_interface(int id) override { return (id == 'Mask' || id == 'Img') ? this : NULL; }

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {

    PropPanel::ContainerPropertyControl *op = panel.createGroup(pid++, paramName);

    bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this;
    buttonPid = pid++;
    editButtonPid = pid++;
    op->createButton(buttonPid, imageName);
    op->createButton(editButtonPid, isEditImage ? "Done" : "Edit", true, false);

    Tab<String> channels(tmpmem);
    channels.resize(IHmapBrushImage::CHANNELS_COUNT);
    channels[IHmapBrushImage::CHANNEL_RGB] = "RGB";
    channels[IHmapBrushImage::CHANNEL_R] = "Red";
    channels[IHmapBrushImage::CHANNEL_G] = "Green";
    channels[IHmapBrushImage::CHANNEL_B] = "Blue";
    channels[IHmapBrushImage::CHANNEL_A] = "Alpha";

    channelsComboPid = pid++;
    op->createCombo(channelsComboPid, "Edit channel:", channels, channel, !isEditImage);

    showMaskPid = pid++;
    op->createCheckBox(showMaskPid, "Show blue-white mask", showMask);

    detailTypePid = pid++;
    op->createEditInt(detailTypePid, "Detail type", detailType);

    mappingTypePid = pid++;
    PropPanel::ContainerPropertyControl *rg = op->createRadioGroup(mappingTypePid, "Mapping type", mappingType);
    rg->createRadio(pid++, "match heightmap");
    rg->createRadio(pid++, "heightmap percent");
    rg->createRadio(pid++, "world units");
    rg->createRadio(pid++, "vertical U");
    rg->createRadio(pid++, "vertical V");
    op->setInt(mappingTypePid, mappingTypePid + 1 + mappingType);

    tilePid = pid++;
    offsetPid = pid++;
    op->createPoint2(tilePid, "Tile", tile);
    op->createPoint2(offsetPid, "Offset", offset);

    op->createCheckBox(clampUPid = pid++, "Clamp U", clampU);
    op->createCheckBox(clampVPid = pid++, "Clamp V", clampV);

    op->createCheckBox(swapUVPid = pid++, "Swap UV", swapUV);

    op->createCheckBox(flipUPid = pid++, "Flip U", flipU);
    op->createCheckBox(flipVPid = pid++, "Flip V", flipV);

    op->setBoolValue(true);
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid == detailTypePid)
      detailType = panel.getInt(detailTypePid);
    else if (pid == mappingTypePid)
      mappingType = panel.getInt(mappingTypePid) - mappingTypePid - 1;
    else if (pid == tilePid)
      tile = panel.getPoint2(tilePid);
    else if (pid == offsetPid)
      offset = panel.getPoint2(offsetPid);
    else if (pid == clampUPid)
      clampU = panel.getBool(pid);
    else if (pid == clampVPid)
      clampV = panel.getBool(pid);
    else if (pid == flipUPid)
      flipU = panel.getBool(pid);
    else if (pid == flipVPid)
      flipV = panel.getBool(pid);
    else if (pid == swapUVPid)
      swapUV = panel.getBool(pid);
    else if (pid == channelsComboPid)
      channel = (IHmapBrushImage::Channel)panel.getInt(pid);
    else if (pid == showMaskPid)
      showMask = panel.getBool(pid);
  }

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid == buttonPid)
    {
      const char *name = HmapLandPlugin::self->pickScriptImage(imageName, 32);
      if (name)
      {
        imageName = name;
        panel.setCaption(pid, (char *)imageName);
      }
    }
    else if (pid == editButtonPid)
    {
      ScriptParamMask *curEd = (ScriptParamMask *)HmapLandPlugin::self->getEditedScriptImage();
      if (curEd)
        curEd->finishEdit(panel);

      HmapLandPlugin::self->editScriptImage(this);
      bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this;
      panel.setCaption(pid, isEditImage ? "Done" : "Edit");
      panel.setEnabledById(channelsComboPid, !isEditImage);
    }
  }

  void save(DataBlock &main_blk) override
  {
    DataBlock &blk = *main_blk.addBlock(paramName);

    blk.setStr("name", imageName);
    blk.setInt("detailType", detailType);
    blk.setInt("mappingType", mappingType);
    blk.setPoint2("tile", tile);
    blk.setPoint2("offset", offset);
    blk.setBool("clampU", clampU);
    blk.setBool("clampV", clampV);
    blk.setBool("flipU", flipU);
    blk.setBool("flipV", flipV);
    blk.setBool("swapUV", swapUV);
    blk.setInt("channel", channel);
    blk.setBool("showMask", showMask);
  }

  void load(const DataBlock &main_blk) override
  {
    const DataBlock &blk = *main_blk.getBlockByNameEx(paramName);
    const char *name = blk.getStr("name", NULL);
    if (name)
      imageName = name;
    detailType = blk.getInt("detailType", detailType);
    mappingType = blk.getInt("mappingType", mappingType);
    tile = blk.getPoint2("tile", tile);
    offset = blk.getPoint2("offset", offset);
    clampU = blk.getBool("clampU", clampU);
    clampV = blk.getBool("clampV", clampV);
    flipU = blk.getBool("flipU", flipU);
    flipV = blk.getBool("flipV", flipV);
    swapUV = blk.getBool("swapUV", swapUV);
    channel = (IHmapBrushImage::Channel)blk.getInt("channel", channel);
    showMask = blk.getBool("showMask", true);
  }

  void finishEdit(PropPanel::ContainerPropertyControl &panel) override
  {
    panel.setCaption(editButtonPid, "Edit");
    panel.setEnabledById(channelsComboPid, true);
  }

  void flipAndSwapUV(real &u, real &v) const
  {
    if (swapUV)
    {
      real a = u;
      u = v;
      v = a;
    }

    if (flipU)
      u = 1 - u;
    if (flipV)
      v = 1 - v;
  }

  void calcMapping(const LandColorGenData &lcg, real x, real z, Point2 &p) const;

  E3DCOLOR sampleImage(const LandColorGenData &lcg) const;
  float sampleImage1(const LandColorGenData &lcg) const;
  float sampleImage8(const LandColorGenData &lcg) const;
  void setImage(const LandColorGenData &lcg, E3DCOLOR) const;

  E3DCOLOR sampleImagePixel(const LandColorGenData &lcg) const;
  E3DCOLOR sampleImagePixelTrueAlpha(const LandColorGenData &lcg) const;
  void paintImage(const LandColorGenData &lcg, E3DCOLOR color) const;

  E3DCOLOR sampleImageAt(real u, real v)
  {
    flipAndSwapUV(u, v);

    E3DCOLOR c = HmapLandPlugin::self->sampleScriptImageUV(imageIndex, u, v, clampU, clampV);
    c.a = detailType;
    return c;
  }
  bool saveImage();
};


class ScriptParamLandClass : public HmapLandPlugin::ScriptParam, public IAssetUpdateNotify
{
public:
  SimpleString asset;
  SmallTab<short, MidmemAlloc> perLayerAssetIdx;
  PtrTab<ScriptParamImage> images;
  int detailIdx;
  landclass::AssetData *assetData;

  int buttonPid;

  struct DefParams
  {
    SimpleString asset;
    SmallTab<short, TmpmemAlloc> layerIdx;
    int detailIdx;

    DefParams() : detailIdx(-1) {}
  };


  ScriptParamLandClass(const char *name, const DefParams &def) : ScriptParam(name), asset(def.asset), buttonPid(-1), images(midmem)
  {
    perLayerAssetIdx = def.layerIdx;
    detailIdx = def.detailIdx;
    assetData = NULL;
    registerAsset();
    LandClassSlotsManager::subscribeLandClassUpdateNotify(this);
  }
  ~ScriptParamLandClass() override
  {
    unregisterAsset();
    LandClassSlotsManager::unsubscribeLandClassUpdateNotify(this);
  }

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {
    buttonPid = pid;
    panel.createIndent();
    panel.createSeparator(0);
    panel.createStatic(pid + PID_STATIC_OFFSET, paramName);
    panel.setBool(pid + PID_STATIC_OFFSET, true);
    panel.createButton(buttonPid, ::dd_get_fname(asset));
    // panel.createButton(buttonPid+1, "X");
    pid += 2;
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override {}

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid == buttonPid)
    {
      const char *lc_asset = DAEDITOR3.selectAssetX(asset, "Select landclass", "land");

      if (lc_asset)
      {
        asset = lc_asset;
        panel.setCaption(pid, lc_asset);
        registerAsset();

        HmapLandPlugin::self->getLandClassMgr().regenerateObjects(HmapLandPlugin::self->getlandClassMap());
      }
    }
    else if (pid == buttonPid + 1)
    {
      // asset=NULL;
      // panel.setCaption(pid-1, (char*)asset);
      // unregisterAsset();
    }
  }

  void save(DataBlock &main_blk) override
  {
    DataBlock &blk = *main_blk.addBlock(paramName);

    blk.setStr("asset", asset);
  }

  void load(const DataBlock &main_blk) override
  {
    const DataBlock &blk = *main_blk.getBlockByNameEx(paramName);
    const char *name = blk.getStr("asset", NULL);
    if (name)
      asset = name;
    registerAsset();
  }

  void registerAsset()
  {
    if (!asset.length())
      return unregisterAsset();

    for (int i = 0; i < perLayerAssetIdx.size(); i++)
      if (perLayerAssetIdx[i] >= 0)
        HmapLandPlugin::self->getLandClassMgr().setLandClass(i, perLayerAssetIdx[i], asset);

    regetAsset();
    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    if (det_layer_id >= 0 && detailIdx >= 0 && assetData)
      HmapLandPlugin::self->setDetailTexSlot(detailIdx, asset);
    images.clear();
    if (assetData && assetData->colorImages)
    {
      images.resize(assetData->colorImages->images.size());
      for (int i = 0; i < images.size(); i++)
      {
        landclass::ColorImagesData::Image &img = assetData->colorImages->images[i];
        ScriptParamImage::DefParams def;
        def.imageName = img.fname;
        def.mappingType = img.mappingType;
        def.tile = img.tile;
        def.offset = img.offset;
        def.clampU = img.clampU;
        def.clampV = img.clampV;
        def.flipU = img.flipU;
        def.flipV = img.flipV;
        def.swapUV = img.swapUV;

        images[i] = new ScriptParamImage(String(64, "%s_%s", (char *)paramName, (char *)img.name), def);
        HmapLandPlugin::self->getScriptImage(images[i]->imageName, 1, 32);
      }
    }
  }

  void unregisterAsset()
  {
    LandClassSlotsManager::releaseAsset(assetData);
    assetData = NULL;

    for (int i = 0; i < perLayerAssetIdx.size(); i++)
      if (perLayerAssetIdx[i] >= 0)
        HmapLandPlugin::self->getLandClassMgr().setLandClass(i, perLayerAssetIdx[i], NULL);

    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    if (det_layer_id >= 0 && detailIdx >= 0)
      HmapLandPlugin::self->setDetailTexSlot(detailIdx, NULL);
    images.clear();
  }

  void regetAsset()
  {
    landclass::AssetData *a = assetData;
    assetData = LandClassSlotsManager::getAsset(asset);
    LandClassSlotsManager::releaseAsset(a);
  }

  // IAssetUpdateNotify interface
  void onLandClassAssetChanged(landclass::AssetData *data) override
  {
    if (assetData == data)
    {
      unregisterAsset();
      registerAsset();
      HmapLandPlugin::self->getLandClassMgr().regenerateObjects(HmapLandPlugin::self->getlandClassMap());
    }
  }
  void onLandClassAssetTexturesChanged(landclass::AssetData *data) override {}
  void onSplineClassAssetChanged(splineclass::AssetData *data) override {}
};


class ScriptParamDetailTex : public HmapLandPlugin::ScriptParam
{
public:
  enum
  {
    IPID_LANDCLASS,
    IPID__NUM
  };
  struct DefParams
  {
    int detailSlot;
    // Point2 offset;
    SimpleString blkName;

    DefParams() : detailSlot(-1), blkName("") {} //, tile(1.0), offset(0.f, 0.f)
  };
  DefParams p;
  int basePid;

  ScriptParamDetailTex(const char *name, const DefParams &def) : ScriptParam(name), p(def), basePid(-1) { registerDetTex(); }
  ~ScriptParamDetailTex() override { unregisterDetTex(); }

  static bool prepareName(String &out, const char *in)
  {
    if (!in || !in[0])
    {
      out = NULL;
      return false;
    }

    out = DagorAsset::fpath2asset(in);
    DagorAsset *a = DAEDITOR3.getAssetByName(out, DAEDITOR3.getAssetTypeId("land"));

    if (a)
      return ::dd_file_exist(a->getTargetFilePath());

    DAEDITOR3.conError("cant find det land asset: %s", out);
    return ::dd_file_exist(in);
  }

  void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) override
  {
    String btn_name;
    PropPanel::ContainerPropertyControl *op = panel.createGroup(pid++, String(64, "%s: slot %d", (char *)paramName, p.detailSlot));

    basePid = pid;
    pid += IPID__NUM;

    // detail tex
    if (!prepareName(btn_name, p.blkName))
      btn_name = "---";
    op->createButton(basePid + IPID_LANDCLASS, btn_name);
    op->setBoolValue(true);
  }

  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override {}

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid >= basePid + IPID_LANDCLASS && pid < basePid + IPID__NUM)
    {
      const char *asset;

      switch (pid - basePid)
      {
        case IPID_LANDCLASS: asset = p.blkName; break;
        default: G_ASSERT(0 && "unknown tex!");
      }
      asset = DAEDITOR3.selectAssetX(asset, "Select landclass", "land");

      if (!asset)
        return;

      String btn_name;
      switch (pid - basePid)
      {
        case IPID_LANDCLASS:
          p.blkName = asset;
          if (!prepareName(btn_name, asset))
            btn_name = "---";
          panel.setCaption(pid, btn_name.str());
          break;
      }
      registerDetTex();
    }
  }

  void save(DataBlock &main_blk) override
  {
    DataBlock &blk = *main_blk.addBlock(paramName);
    blk.setStr("asset", p.blkName);
  }

  void load(const DataBlock &main_blk) override
  {
    const DataBlock &blk = *main_blk.getBlockByNameEx(paramName);
    const char *asset = blk.getStr("asset", NULL);

    if (asset)
      p.blkName = asset;
    registerDetTex();
  }

  void registerDetTex()
  {
    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    if (det_layer_id >= 0 && p.detailSlot >= 0)
      HmapLandPlugin::self->setDetailTexSlot(p.detailSlot, p.blkName);
  }
  void unregisterDetTex()
  {
    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    if (det_layer_id >= 0 && p.detailSlot >= 0)
      HmapLandPlugin::self->setDetailTexSlot(p.detailSlot, NULL);
  }
};


// Deterministic color from a layer name -- used as the default tint in the
// "Show landclass weights (color)" overlay so a fresh BLK already has visually
// distinct per-layer colors. FNV-1a -> rgb, then boost the brightest channel
// to 255 (and scale the others) so dim hashes still map to vivid colors.
static E3DCOLOR defaultLayerColor(const char *name)
{
  uint32_t h = 2166136261u;
  for (const char *p = name ? name : ""; *p; ++p)
    h = (h ^ (uint8_t)*p) * 16777619u;
  uint32_t r = (h >> 0) & 0xFF;
  uint32_t g = (h >> 8) & 0xFF;
  uint32_t b = (h >> 16) & 0xFF;
  uint32_t m = max(r, max(g, b));
  if (m == 0)
    return E3DCOLOR(128, 128, 128, 255);
  r = (r * 255) / m;
  g = (g * 255) / m;
  b = (b * 255) / m;
  return E3DCOLOR(r, g, b, 255);
}


class PostScriptParamLandLayer : public HmapLandPlugin::ScriptParam, public IAssetUpdateNotify
{
public:
  enum
  {
    PID_LAYER_CAPTION,
    PID_LAYER_ENABLED,
    PID_EDIT_PROPS,
    PID_SEL_LC1,
    PID_SEL_LC2,
    PID_SEL_MASK,
    PID_EDIT_MASK,
    PID_SEL_GRASS_MASK,
    PID_EXPR_SUMMARY,
    PID_LAYER_COLOR,
    PID_ELC_EDIT_MASK_BASE,
    PID_ELC_EDIT_MASK_LAST = PID_ELC_EDIT_MASK_BASE + 10,
    PID__COUNT
  };
  struct WtRange
  {
    enum
    {
      WMT_one,
      WMT_asIs,
      WMT_smoothStep,
      WMT_zero
    };
    float v0, dv;
    int conv;
  };

  int slotIdx, pidBase, landIdx;
  int detIdx;
  bool enabled;
  int wtMode; // 0=multiply, 1=sum, 2=max(mask, terrain product)
  bool badDetTex;

  enum
  {
    AREA_main,
    AREA_det,
    AREA_both
  };
  int areaSelect;

  // landclass 1
  struct LandClassRec
  {
    SimpleString asset;
    landclass::AssetData *assetData;
  } lc1, lc2;

  // mask
  SimpleString maskName;
  int imageIndex, imageBpp;
  int maskConv;
  bool detRectMappedMask;

  union
  {
    struct
    {
      WtRange ht, ang, curv;
    };
    WtRange ht_ang_curv[3];
  };
  bool writeDetTex = true, writeLand1 = true, writeLand2 = false, writeImportance = false;
  float writeDetTexThres = 0, writeLand1Thres = 0, writeLand2Thres = 0;
  bool editableLandClass;

  // Tint used by the "Show landclass weights (color)" terrain overlay. Default is
  // a deterministic hash of paramName so a fresh BLK already shows distinct colors.
  E3DCOLOR layerColor;

  // Expression evaluator
  SimpleString exprText;
  lcexpr::Arena exprArena;
  uint32_t exprRoot = 0;
  eastl::bitset<lcexpr::MAX_VAR_ID> exprVarMask;
  bool useExpr = false;
  bool exprValid = false;
  // Last parse/compile error from compileExpr(); empty on success. Read by the Apply
  // handler to surface the error in a message box without re-parsing.
  SimpleString lastErr;
  // Post-parse nv for this layer's bytecode -- references varIds in [0, exprNv).
  // [post_common_nv, exprNv) is this layer's private user-var range and must be
  // zeroed per pixel before evaluating the layer (so an unassigned `var x` reads 0
  // rather than a stale value left by another layer's eval at the same pixel).
  // Block-scoped temp regs (from `{...}` syntax) live at slots [exprNv, exprNv +
  // MAX_TEMP_REGS); the bake loop always reserves MAX_TEMP_REGS slots beyond the
  // per-expression named range rather than tracking each expression's actual peak --
  // simpler bookkeeping, and 16 extra floats per pixel is below the noise floor.
  uint16_t exprNv = 0;

  // shrink_arena: reclaim unused arena capacity at the end. Set to false on the UI
  // slider-drag rebuild path (rebuildExprFromOldFormat) where compileExpr runs many
  // times per second and the realloc cost dominates.
  //
  // vm/nv (and vt) are this layer's PRIVATE copy of the post-common symbol table.
  // The walker builds a fresh copy for each layer, so any var/let this layer
  // declares stays local and is invisible to peers (mirrors C function-local
  // semantics). To share a value across layers, declare it in the common expression
  // instead.
  void compileExpr(lcexpr::NameMap &vm, uint16_t &nv, lcexpr::VarTypeTable &vt, bool shrink_arena = true)
  {
    exprArena.clear();
    exprValid = false;
    exprRoot = 0;
    exprVarMask.reset();
    lastErr = "";
    exprNv = nv;

    // PARSE_DEFAULT (no flags): a layer's TOP-LEVEL `var`/`let` declarations
    // are local to that layer's eval -- nothing reads them past the bake of
    // this pixel, so DCE may freely eliminate dead writes. Common's parse
    // keeps PARSE_EXPORT_TOP_LEVEL_VARS so common-declared vars survive for
    // subsequent layer reads against the shared varMap.
    //
    // vm/nv/vt are this layer's PRIVATE post-common copy: any var/let parsed
    // here mutates them but the caller discards the copy after the layer is
    // done, so peers see the same starting symbol table.
    eastl::string err;
    if (!compile_expression(exprArena, exprText.str(), lcexpr::PARSE_DEFAULT, vm, nv, vt, exprRoot, exprVarMask, err))
    {
      lastErr = err.c_str();
      DAEDITOR3.conError("layer '%s' expression %s (expr='%s')", paramName.str(), err.c_str(), exprText.str());
      exprArena.clear();
      updateExprDisplay();
      return;
    }
    exprNv = nv;
    exprValid = true;
    if (shrink_arena)
      exprArena.shrink_to_fit();
    updateExprDisplay();
  }

  eastl::string buildExprFromOldFormat() const
  {
    return lcexpr::genLayerToExpr({maskConv, ht.conv, ang.conv, curv.conv, ht.v0, ht.dv, ang.v0, ang.dv, curv.v0, curv.dv, wtMode});
  }

  void rebuildExprFromOldFormat()
  {
    if (useExpr)
      return;
    exprText = buildExprFromOldFormat().c_str();
    // Per-keystroke (slider-drag) path: shrink_to_fit costs more than the recompile, so
    // skip it. compileExpr() itself updates the display on every exit.
    HmapLandPlugin::self->recompileGenLayerExpressions(/*shrink_arenas*/ false);
  }

  // Shared core of PID_SEL_MASK handling used by the PropsDlg button AND the outer panel
  // button (both previously had their own copy). Returns true if the user picked a name
  // (i.e. didn't cancel). Callers are responsible for syncing their own panel widgets.
  // maskConv is updated to track whether a mask is selected even in expression mode,
  // because the surrounding UI (PID_EDIT_MASK, PID_MASK_REFSYS, the SEL_MASK enabled
  // state) keys off maskConv. At runtime the expression decides whether mask is read,
  // not maskConv, so toggling maskConv here does not affect evaluation.
  bool pickMaskAndApplyState()
  {
    const char *name = HmapLandPlugin::self->pickScriptImage(maskName, 8);
    if (!name)
      return false;
    maskName = name;
    if (*name && maskConv == WtRange::WMT_one)
      maskConv = WtRange::WMT_asIs;
    else if (!*name && maskConv != WtRange::WMT_one)
      maskConv = WtRange::WMT_one;
    imageIndex = HmapLandPlugin::self->getScriptImage(maskName, 1, -1);
    imageBpp = HmapLandPlugin::self->getScriptImageBpp(imageIndex);
    rebuildExprFromOldFormat();
    return true;
  }

  void updateExprDisplay()
  {
    const char *text = exprText.empty() ? "1" : exprText.str();
    if (dlg)
    {
      if (dlg->dlg->getPanel()->getById(PropsDlg::PID_EXPR_DISPLAY))
        dlg->dlg->getPanel()->setText(PropsDlg::PID_EXPR_DISPLAY, text);
    }
    // pidBase is assigned in fillParams(); compileExpr() / rebuildExprFromOldFormat() can
    // fire before that from load() / addGenLayer(). pidBase + PID_EXPR_SUMMARY with
    // pidBase == -1 would alias some unrelated control id on the open panel and overwrite
    // its caption, so skip the outer-panel update until the layer has been placed.
    if (pidBase >= 0)
    {
      if (PropPanel::ContainerPropertyControl *pp = HmapLandPlugin::self->getPropPanel())
      {
        if (pp->getById(pidBase + PID_EXPR_SUMMARY))
          pp->setCaption(pidBase + PID_EXPR_SUMMARY, text);
      }
    }
  }

  struct PropsDlg : public PropPanel::ControlEventHandler
  {
    enum
    {
      PID_LAYER_CAPTION,
      PID_LAYER_WEIGHTCALC,
      PID_LAYER_OUTPUT,
      PID_LAYER_SUMWT,
      PID_WTR_AREA,
      PID_WTR_MASK_CONV,
      PID_MASK_REFSYS,
      PID_SEL_MASK,
      PID_WTR_HT_V0,
      PID_WTR_HT_DV,
      PID_WTR_HT_CONV,
      PID_WTR_ANG_V0,
      PID_WTR_ANG_DV,
      PID_WTR_ANG_CONV,
      PID_WTR_CURV_V0,
      PID_WTR_CURV_DV,
      PID_WTR_CURV_CONV,
      PID_WRITE_LC1,
      PID_WRITE_LC1_THRES,
      PID_WRITE_LC2,
      PID_WRITE_LC2_THRES,
      PID_WRITE_DET,
      PID_WRITE_DET_THRES,
      PID_WRITE_IM,
      PID_EXPR_DISPLAY,
      PID_EXPR_EDIT,
      PID_EXPR_APPLY,
      PID_CONVERT_TO_EXPR,
      PID_EXPR_HELP,
      PID_LAYER_ELC_ADD,
      PID_BASE,
    };
    enum
    {
      PID_ELC_GRP,
      PID_ELC_PRESET,
      PID_ELC_SEL_MASK,
      PID_ELC_OUT_COLORMAP,
      PID_ELC_OUT_SPLATTINGMAP,
      PID_ELC_PARAM_BASE,
      PID_ELC_PARAM_LAST = PID_ELC_PARAM_BASE + 10,
      PID_ELC__CNT
    };

  public:
    PropsDlg(PostScriptParamLandLayer &_gl) : gl(_gl), windowPosition(0, 0), windowSize(0, 0)
    {
      dlg = EDITORCORE->createDialog(_pxScaled(290), _pxScaled(460),
        String(0, gl.paramName.empty() ? "Layer #%d" : "Layer #%d: %s", gl.slotIdx + 1, gl.paramName));
      dlg->setCloseHandler(this);
      dlg->showButtonPanel(false);
    }
    ~PropsDlg() override
    {
      EDITORCORE->deleteDialog(dlg);
      dlg = NULL;
    }

    bool isVisible() const { return dlg->isVisible(); }
    void show()
    {
      if (windowSize.x > 0 && windowSize.y > 0)
      {
        dlg->setWindowPosition(windowPosition);
        dlg->setWindowSize(windowSize);
      }

      dlg->show();
      PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
      panel.setEventHandler(this);

      Tab<String> wtConv(tmpmem);
      wtConv.push_back() = "1";
      wtConv.push_back() = "As is";
      wtConv.push_back() = "Smooth step";
      wtConv.push_back() = "0";
      Tab<String> areaConv(tmpmem);
      areaConv.push_back() = "Main only";
      areaConv.push_back() = "Detailed only";
      areaConv.push_back() = "Main and detailed";

      panel.createStatic(-1, "Apply to land region");
      panel.createCombo(PID_WTR_AREA, "", areaConv, gl.areaSelect, true, false);
      panel.setEnabledById(PID_WTR_AREA, HmapLandPlugin::self->hasDetaledRect());

      PropPanel::ContainerPropertyControl &pCalc = *panel.createGroup(PID_LAYER_WEIGHTCALC, "Weight formula");
      PropPanel::ContainerPropertyControl &pOut = *panel.createGroup(PID_LAYER_OUTPUT, "Output and thresholds");

      // Expression display (always shown)
      if (gl.useExpr)
      {
        pCalc.createEditBox(PID_EXPR_EDIT, "Expression:", gl.exprText, true);
        pCalc.createButton(PID_EXPR_APPLY, "Apply expression");
        if (!gl.exprValid)
          pCalc.createStatic(-1, gl.slotIdx == 0 ? "** PARSE ERROR -- layer 0 falls back to weight=1 (full coverage) **"
                                                 : "** PARSE ERROR -- layer evaluates as 0 **");
      }
      else
      {
        pCalc.createStatic(PID_EXPR_DISPLAY, gl.exprText.empty() ? "1" : gl.exprText.str());
        pCalc.createButton(PID_CONVERT_TO_EXPR, "Convert to expression");
      }
      pCalc.createButton(PID_EXPR_HELP, "Print expression help to console");
      pCalc.createSeparator(0);

      if (!gl.useExpr)
      {
        Tab<String> wtModeConv(tmpmem);
        wtModeConv.push_back() = "Multiply";
        wtModeConv.push_back() = "Sum";
        wtModeConv.push_back() = "Max(mask, terrain)";
        pCalc.createStatic(-1, "Weight combination mode");
        pCalc.createCombo(PID_LAYER_SUMWT, "", wtModeConv, gl.wtMode, true, false);
        pCalc.createSeparator(0);

        pCalc.createStatic(-1, "Mask weight conv");
        pCalc.createCombo(PID_WTR_MASK_CONV, "", wtConv, gl.maskConv, true, false);
        pCalc.createCheckBox(PID_MASK_REFSYS, "Mapped to detailed area", gl.detRectMappedMask);
        pCalc.setEnabledById(PID_MASK_REFSYS, HmapLandPlugin::self->hasDetaledRect() && gl.maskConv);
        pCalc.createButton(PID_SEL_MASK, gl.maskName.empty() ? "-- no mask --" : gl.maskName, gl.maskConv);
        pCalc.createSeparator(0);

        pCalc.createStatic(-1, "Height weight conv");
        pCalc.createCombo(PID_WTR_HT_CONV, "", wtConv, gl.ht.conv, true, false);
        pCalc.createEditFloat(PID_WTR_HT_V0, "base", gl.ht.v0, 2, gl.ht.conv);
        pCalc.createEditFloat(PID_WTR_HT_DV, "delta", gl.ht.dv, 2, gl.ht.conv, false);

        pCalc.createStatic(-1, "Angle weight conv");
        pCalc.createCombo(PID_WTR_ANG_CONV, "", wtConv, gl.ang.conv, true, false);
        pCalc.createEditFloat(PID_WTR_ANG_V0, "base", gl.ang.v0, 2, gl.ang.conv);
        pCalc.createEditFloat(PID_WTR_ANG_DV, "delta", gl.ang.dv, 2, gl.ang.conv, false);

        pCalc.createStatic(-1, "Curv. weight conv");
        pCalc.createCombo(PID_WTR_CURV_CONV, "", wtConv, gl.curv.conv, true, false);
        pCalc.createEditFloat(PID_WTR_CURV_V0, "base", gl.curv.v0, 2, gl.curv.conv);
        pCalc.createEditFloat(PID_WTR_CURV_DV, "delta", gl.curv.dv, 2, gl.curv.conv, false);
      }

      pOut.createCheckBox(PID_WRITE_DET, "Write weight to DetTex", gl.writeDetTex);
      pOut.createEditFloat(PID_WRITE_DET_THRES, "DetTex weight threshold", gl.writeDetTexThres, 2, gl.writeDetTex);

      pOut.createCheckBox(PID_WRITE_IM, "Write weight to importance mask", gl.writeImportance);

      pOut.createCheckBox(PID_WRITE_LC1, "Write weight to Land#1", gl.writeLand1);
      pOut.createEditFloat(PID_WRITE_LC1_THRES, "Land#1 weight threshold", gl.writeLand1Thres, 2, gl.writeLand1);

      pOut.createCheckBox(PID_WRITE_LC2, "Write weight to Land#2", gl.writeLand2);
      pOut.createEditFloat(PID_WRITE_LC2_THRES, "Land#2 weight threshold", gl.writeLand2Thres, 2, gl.writeLand2);

      if (gl.editableLandClass)
      {
        dag::Span<HmapDetLayerProps> dlp = HmapLandPlugin::hmlService->getDetLayerClassList();
        Tab<String> clsList(tmpmem);
        for (int i = 0; i < dlp.size(); i++)
          clsList.push_back() = dlp[i].name;

        pCalc.createSeparator(0);
        for (int i = 0; i < gl.elcLayers.blockCount(); i++)
        {
          const DataBlock &b = *gl.elcLayers.getBlock(i);
          int base = PID_BASE + i * PID_ELC__CNT;
          int idx = HmapLandPlugin::hmlService->resolveDetLayerClassName(b.getStr("lName", ""));

          PropPanel::ContainerPropertyControl &pElc =
            *panel.createExtGroup(base + PID_ELC_GRP, String(0, "Land detail layer #%d", i + 1));
          panel.setInt(base + PID_ELC_GRP, (1 << PropPanel::EXT_BUTTON_INSERT) | (1 << PropPanel::EXT_BUTTON_REMOVE) |
                                             (i > 0 ? (1 << PropPanel::EXT_BUTTON_UP) : 0) |
                                             (i + 1 < gl.elcLayers.blockCount() ? (1 << PropPanel::EXT_BUTTON_DOWN) : 0));

          pElc.createStatic(-1, "Detail class");
          pElc.createCombo(base + PID_ELC_PRESET, "", clsList, b.getStr("lName", ""), true, false);
          if (idx >= 0)
          {
            if (dlp[idx].needsMask)
              pElc.createButton(base + PID_ELC_SEL_MASK, *b.getStr("lMask", "") ? b.getStr("lMask") : "-- no mask --");
            pElc.createCheckBox(base + PID_ELC_OUT_COLORMAP, "Output colormap", b.getBool("lOutCmap", true));
            if (dlp[idx].canOutSplatting)
              pElc.createCheckBox(base + PID_ELC_OUT_SPLATTINGMAP, "Output splatting mask", b.getBool("lOutSmap", true));
            for (int j = 0; j < dlp[idx].param.size() && PID_ELC_PARAM_BASE + j <= PID_ELC_PARAM_LAST; j++)
              switch (dlp[idx].param[j].type)
              {
                case HmapDetLayerProps::Param::PT_int:
                  pElc.createEditInt(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].name,
                    b.getInt(dlp[idx].param[j].name, dlp[idx].param[j].defValI[0]));
                  pElc.setMinMaxStep(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].pmin, dlp[idx].param[j].pmax, 1);
                  break;
                case HmapDetLayerProps::Param::PT_bool:
                  pElc.createCheckBox(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].name,
                    b.getBool(dlp[idx].param[j].name, dlp[idx].param[j].defValI[0]));
                  break;
                case HmapDetLayerProps::Param::PT_float:
                  pElc.createEditFloat(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].name,
                    b.getReal(dlp[idx].param[j].name, dlp[idx].param[j].defValF[0]));
                  pElc.setMinMaxStep(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].pmin, dlp[idx].param[j].pmax, 0.1);
                  break;
                case HmapDetLayerProps::Param::PT_float2:
                  pElc.createPoint2(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].name,
                    b.getPoint2(dlp[idx].param[j].name, Point2(dlp[idx].param[j].defValF[0], dlp[idx].param[j].defValF[1])));
                  pElc.setMinMaxStep(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].pmin, dlp[idx].param[j].pmax, 0.1);
                  break;
                case HmapDetLayerProps::Param::PT_tex:
                  pElc.createButton(base + PID_ELC_PARAM_BASE + j,
                    String(0, "%s: %s", dlp[idx].param[j].name,
                      *b.getStr(dlp[idx].param[j].name, "") ? b.getStr(dlp[idx].param[j].name) : "-- no tex --"));
                  break;
                case HmapDetLayerProps::Param::PT_color:
                  pElc.createSimpleColor(base + PID_ELC_PARAM_BASE + j, dlp[idx].param[j].name,
                    b.getE3dcolor(dlp[idx].param[j].name, 0xFFFFFFFF));
                  break;
              }
          }
        }
        panel.createButton(PID_LAYER_ELC_ADD, "Add layer", dlp.size() > 0);
      }
      dlg->getPanel()->loadState(panelState);
      dlg->setScrollPos(panelState.getInt("pOffset", 0));
    }
    void hide()
    {
      if (dlg->getPanel()->getChildCount())
      {
        panelState.reset();
        dlg->getPanel()->saveState(panelState);
        panelState.setInt("pOffset", dlg->getScrollPos());
        dlg->getPanel()->clear();
      }
      dlg->hide();
    }
    void refillPanel()
    {
      if (dlg->isVisible())
      {
        panelState.reset();
        dlg->getPanel()->saveState(panelState);
        panelState.setInt("pOffset", dlg->getScrollPos());

        dlg->getPanel()->clear();
        show();
      }
    }
    void updateTitle()
    {
      dlg->setCaption(String(0, gl.paramName.empty() ? "Layer #%d" : "Layer #%d: %s", gl.slotIdx + 1, gl.paramName));
    }

    void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override
    {
      if (pid == PID_LAYER_SUMWT)
        gl.wtMode = panel->getInt(pid);
      else if (pid == PID_MASK_REFSYS)
        gl.detRectMappedMask = panel->getBool(pid);
      else if (pid == PID_WTR_AREA)
      {
        gl.areaSelect = panel->getInt(pid);
      }
      else if (pid == PID_WTR_MASK_CONV)
      {
        gl.maskConv = panel->getInt(pid);
        panel->setEnabledById(PID_MASK_REFSYS, HmapLandPlugin::self->hasDetaledRect() && gl.maskConv);
        panel->setEnabledById(PID_SEL_MASK, gl.maskConv);
        if (PropPanel::ContainerPropertyControl *pp = HmapLandPlugin::self->getPropPanel())
        {
          pp->setEnabledById(gl.pidBase + gl.PID_SEL_MASK, true);
          pp->setEnabledById(gl.pidBase + gl.PID_EDIT_MASK, gl.maskConv);
        }
      }
      else if (pid == PID_WTR_HT_CONV)
      {
        gl.ht.conv = panel->getInt(pid);
        panel->setEnabledById(PID_WTR_HT_V0, gl.ht.conv);
        panel->setEnabledById(PID_WTR_HT_DV, gl.ht.conv);
      }
      else if (pid == PID_WTR_HT_V0)
        gl.ht.v0 = panel->getFloat(pid);
      else if (pid == PID_WTR_HT_DV)
        gl.ht.dv = panel->getFloat(pid);
      else if (pid == PID_WTR_ANG_CONV)
      {
        gl.ang.conv = panel->getInt(pid);
        panel->setEnabledById(PID_WTR_ANG_V0, gl.ang.conv);
        panel->setEnabledById(PID_WTR_ANG_DV, gl.ang.conv);
      }
      else if (pid == PID_WTR_ANG_V0)
        gl.ang.v0 = panel->getFloat(pid);
      else if (pid == PID_WTR_ANG_DV)
        gl.ang.dv = panel->getFloat(pid);
      else if (pid == PID_WTR_CURV_CONV)
      {
        gl.curv.conv = panel->getInt(pid);
        panel->setEnabledById(PID_WTR_CURV_V0, gl.curv.conv);
        panel->setEnabledById(PID_WTR_CURV_DV, gl.curv.conv);
      }
      else if (pid == PID_WTR_CURV_V0)
        gl.curv.v0 = panel->getFloat(pid);
      else if (pid == PID_WTR_CURV_DV)
        gl.curv.dv = panel->getFloat(pid);

      // Recompile expression from old-format params when they change
      if (!gl.useExpr && pid >= PID_LAYER_SUMWT && pid <= PID_WTR_CURV_CONV)
        gl.rebuildExprFromOldFormat();

      if (pid == PID_WRITE_LC1)
      {
        gl.writeLand1 = panel->getBool(pid);
        panel->setEnabledById(PID_WRITE_LC1_THRES, gl.writeLand1);
      }
      else if (pid == PID_WRITE_LC1_THRES)
        gl.writeLand1Thres = panel->getFloat(pid);
      else if (pid == PID_WRITE_LC2)
      {
        gl.writeLand2 = panel->getBool(pid);
        panel->setEnabledById(PID_WRITE_LC2_THRES, gl.writeLand2);
      }
      else if (pid == PID_WRITE_LC2_THRES)
        gl.writeLand2Thres = panel->getFloat(pid);
      else if (pid == PID_WRITE_DET)
      {
        gl.writeDetTex = panel->getBool(pid);
        panel->setEnabledById(PID_WRITE_DET_THRES, gl.writeDetTex);
      }
      else if (pid == PID_WRITE_DET_THRES)
        gl.writeDetTexThres = panel->getFloat(pid);
      else if (pid == PID_WRITE_IM)
        gl.writeImportance = panel->getBool(pid);
      else if (pid >= PID_BASE && pid < PID_BASE + PID_ELC__CNT * gl.elcLayers.blockCount())
      {
        dag::Span<HmapDetLayerProps> dlp = HmapLandPlugin::hmlService->getDetLayerClassList();
        int lidx = (pid - PID_BASE) / PID_ELC__CNT;
        int lpid = (pid - PID_BASE) % PID_ELC__CNT;
        DataBlock &b = *gl.elcLayers.getBlock(lidx);

        if (lpid == PID_ELC_PRESET)
        {
          if (strcmp(b.getStr("lName", ""), dlp[panel->getInt(pid)].name) != 0)
          {
            b.setStr("lName", dlp[panel->getInt(pid)].name);
            panel->setPostEvent(1);
            return;
          }
        }
        else if (lpid == PID_ELC_OUT_COLORMAP)
          b.setBool("lOutCmap", panel->getBool(pid));
        else if (lpid == PID_ELC_OUT_SPLATTINGMAP)
          b.setBool("lOutSmap", panel->getBool(pid));
        else
        {
          int idx = HmapLandPlugin::hmlService->resolveDetLayerClassName(b.getStr("lName", ""));
          if (idx < 0)
            return;
          int pidx = lpid - PID_ELC_PARAM_BASE;
          switch (dlp[idx].param[pidx].type)
          {
            case HmapDetLayerProps::Param::PT_int: b.setInt(dlp[idx].param[pidx].name, panel->getInt(pid)); break;
            case HmapDetLayerProps::Param::PT_bool: b.setBool(dlp[idx].param[pidx].name, panel->getBool(pid)); break;
            case HmapDetLayerProps::Param::PT_float: b.setReal(dlp[idx].param[pidx].name, panel->getFloat(pid)); break;
            case HmapDetLayerProps::Param::PT_float2: b.setPoint2(dlp[idx].param[pidx].name, panel->getPoint2(pid)); break;
            case HmapDetLayerProps::Param::PT_color: b.setE3dcolor(dlp[idx].param[pidx].name, panel->getColor(pid)); break;
          }
        }
        if (gl.drdHandle)
        {
          HmapLandPlugin::hmlService->updateDetailRenderData(gl.drdHandle, gl.elcLayers);
          HmapLandPlugin::hmlService->invalidateClipmap(false);
        }
      }
    }
    void onClick(int pid, PropPanel::ContainerPropertyControl *panel) override
    {
      if (pid == PID_EXPR_HELP)
      {
        static const char *help = "=== Landclass Expression Language ===\n"
                                  "Variables: height, angle, curvature, mask, world_x, world_y,\n"
                                  "           mask_<layer_name>  (any enabled peer layer's mask, up to 32 layers)\n"
                                  "Declarations: var <name> = <expr> | let <name> = <expr>\n"
                                  "  -- introduces a fresh user-mutable variable. LOCAL to the current\n"
                                  "     expression: visible inside the same layer (or inside the common\n"
                                  "     expression) but NOT to other layers. To share a value across\n"
                                  "     layers, declare it in the common expression -- common's var/let\n"
                                  "     names are visible to every layer.\n"
                                  "Statements: <expr>, <expr>   or   <expr>; <expr>\n"
                                  "  -- both `,` and `;` are sequence operators (left then right, return right).\n"
                                  "Assignment: <user_var> = <expr>\n"
                                  "  -- only on user-declared (var/let) names; external vars are read-only.\n"
                                  "Operators: + - * / < > <= >= == != && || !\n"
                                  "Functions:\n"
                                  "  max(a, b)  min(a, b)  clamp(x, lo, hi)  saturate(x)\n"
                                  "  ramp(val, from, to)        -- linear ramp, saturated\n"
                                  "  smooth_ramp(val, from, to) -- smoothstep ramp, saturated\n"
                                  "  smoothstep(x)              -- 3x^2 - 2x^3\n"
                                  "  pow(base, exp)  sqrt(x)  abs(x)  lerp(a, b, t)  frac(x)\n"
                                  "  select(bool_cond, a, b)    -- first arg must be bool\n"
                                  "  step(edge, x)              -- 1 if x >= edge else 0\n"
                                  "  length2(x, y)              -- sqrt(x*x + y*y)\n"
                                  "  noise(x, y)                -- 64-cell tiled LUT noise, [0,1]\n"
                                  "  fbm(x, y, octaves)         -- fractal sum of noise, [0,1]\n"
                                  "  eroded_fbm(x, y, octaves)  -- fbm with slope attenuation (eroded look), [0,1]\n"
                                  "Examples:\n"
                                  "  mask\n"
                                  "  max(mask, ramp(height,16,26) * smooth_ramp(angle,2,22))\n"
                                  "  mask * smooth_ramp(angle, 17, 12)\n"
                                  "  mask + ramp(height, 1, -2)\n"
                                  "  1 - pow(0.65, max(15 - height, 0))\n"
                                  "  ramp(length2(world_x - 500, world_y - 300), 50, 80)\n"
                                  "  fbm(world_x * 0.01, world_y * 0.01, 4)\n"
                                  "  var t = fbm(world_x, world_y, 7); t * t   (cache fbm: one call, used twice)";
        DAEDITOR3.conNote(help);
        return;
      }
      if (pid == PID_CONVERT_TO_EXPR)
      {
        // One-way migration: once a layer becomes an expression there is no revert path.
        // The confirmation exists to stop accidental single-click conversions; users who
        // need the old behavior must re-enter the values in a fresh layer.
        int r = wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Convert to expression",
          "Switch this layer to expression mode?\n\n"
          "This is a one-way migration: you cannot switch back to the simple ht/ang/curv UI "
          "without recreating the layer.");
        if (r != wingw::MB_ID_YES)
          return;
        gl.useExpr = true;
        gl.exprText = gl.buildExprFromOldFormat().c_str();
        HmapLandPlugin::self->recompileGenLayerExpressions();
        hide();
        // Layout structure changes between simple and expression modes; drop any saved
        // widget state so stale pOffset / collapsed-group state does not carry over.
        panelState.reset();
        show();
        HmapLandPlugin::self->refillPanel();
        return;
      }
      if (pid == PID_EXPR_APPLY)
      {
        // Per-layer scope: this layer's var/let are local to its own bytecode and
        // cannot affect peers. So the only failure mode is "this layer's expression
        // itself fails to parse/compile". On failure restore prev text + recompile +
        // surface gl.lastErr in a message box; the edit widget keeps the user's
        // failing text so they can fix the typo in place.
        SimpleString newText = panel->getText(PID_EXPR_EDIT);
        SimpleString prev = gl.exprText;
        gl.exprText = newText;
        HmapLandPlugin::self->recompileGenLayerExpressions();
        if (!gl.exprValid)
        {
          SimpleString errMsg = gl.lastErr;
          gl.exprText = prev;
          HmapLandPlugin::self->recompileGenLayerExpressions();
          wingw::message_box(wingw::MBS_EXCL, "Expression error", "%s", errMsg.empty() ? "compile failed" : errMsg.str());
          return;
        }
        hide();
        show();
        HmapLandPlugin::self->refillPanel();
        return;
      }
      if (pid == -PropPanel::DIALOG_ID_CLOSE)
      {
        windowPosition = dlg->getWindowPosition();
        windowSize = dlg->getWindowSize();

        if (PropPanel::ContainerPropertyControl *pp = HmapLandPlugin::self->getPropPanel())
          gl.onPPBtnPressed(pid, *pp);
        if (dlg->getPanel()->getChildCount())
        {
          panelState.reset();
          dlg->getPanel()->saveState(panelState);
          panelState.setInt("pOffset", dlg->getScrollPos());
          dlg->getPanel()->clear();
        }
      }
      else if (pid == PID_SEL_MASK)
      {
        if (HmapLandPlugin::self->getEditedScriptImage() == &gl)
        {
          gl.finishEdit(*panel);
          HmapLandPlugin::self->editScriptImage(&gl);
          HmapLandPlugin::self->setShowBlueWhiteMask();
        }
        if (gl.pickMaskAndApplyState())
        {
          const char *caption = !gl.maskName.empty() ? gl.maskName.str() : "-- no mask --";
          panel->setInt(PID_WTR_MASK_CONV, gl.maskConv);
          panel->setEnabledById(PID_MASK_REFSYS, HmapLandPlugin::self->hasDetaledRect() && gl.maskConv);
          panel->setEnabledById(PID_SEL_MASK, gl.maskConv);
          panel->setCaption(pid, caption);
          if (PropPanel::ContainerPropertyControl *pp = HmapLandPlugin::self->getPropPanel())
          {
            pp->setCaption(gl.pidBase + gl.PID_SEL_MASK, caption);
            pp->setEnabledById(gl.pidBase + gl.PID_SEL_MASK, true);
            pp->setEnabledById(gl.pidBase + gl.PID_EDIT_MASK, gl.maskConv);
          }
        }
      }
      else if (pid == PID_LAYER_ELC_ADD)
      {
        DataBlock &b = *gl.elcLayers.addNewBlock("elc_layer");
        HmapDetLayerProps &lp = HmapLandPlugin::hmlService->getDetLayerClassList()[0];
        b.setStr("lName", lp.name);
        for (int i = 0; i < lp.param.size(); i++)
          switch (lp.param[i].type)
          {
            case HmapDetLayerProps::Param::PT_int: b.setInt(lp.param[i].name, lp.param[i].defValI[0]); break;
            case HmapDetLayerProps::Param::PT_float: b.setReal(lp.param[i].name, lp.param[i].defValF[0]); break;
            case HmapDetLayerProps::Param::PT_bool: b.setBool(lp.param[i].name, lp.param[i].defValI[0]); break;
            case HmapDetLayerProps::Param::PT_float2:
              b.setPoint2(lp.param[i].name, Point2(lp.param[i].defValF[0], lp.param[i].defValF[1]));
              break;
            case HmapDetLayerProps::Param::PT_tex: b.setStr(lp.param[i].name, ""); break;
            case HmapDetLayerProps::Param::PT_color: b.setE3dcolor(lp.param[i].name, e3dcolor(Color4(lp.param[i].defValF))); break;
          }
        if (lp.needsMask)
          b.setStr("lMask", "");
        panel->setPostEvent(1);
        gl.reinitDrdHandle();
        gl.updateElcMasks();
      }
      else if (((pid - PID_BASE) % PID_ELC__CNT) == PID_ELC_GRP)
      {
        dag::Span<HmapDetLayerProps> dlp = HmapLandPlugin::hmlService->getDetLayerClassList();
        DataBlock bc;
        int lidx = (pid - PID_BASE) / PID_ELC__CNT;

        switch (panel->getInt(pid))
        {
          case PropPanel::EXT_BUTTON_REMOVE:
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                  "Do you really want to delete land detail layer #%d (%s)?", lidx + 1,
                  gl.elcLayers.getBlock(lidx)->getStr("lName")) != wingw::MB_ID_YES)
              return;
            gl.elcLayers.removeBlock(lidx);
            break;

          case PropPanel::EXT_BUTTON_INSERT:
            for (int i = 0; i < gl.elcLayers.blockCount(); i++)
            {
              if (i == lidx)
              {
                DataBlock &b = *bc.addNewBlock("elc_layer");
                int i;
                b.setStr("lName", dlp[0].name);
                for (int i = 0; i < dlp[0].param.size(); i++)
                  switch (dlp[0].param[i].type)
                  {
                    case HmapDetLayerProps::Param::PT_int: b.setInt(dlp[0].param[i].name, dlp[0].param[i].defValI[0]); break;
                    case HmapDetLayerProps::Param::PT_bool: b.setBool(dlp[0].param[i].name, dlp[0].param[i].defValI[0]); break;
                    case HmapDetLayerProps::Param::PT_float2:
                      b.setPoint2(dlp[0].param[i].name, Point2(dlp[0].param[i].defValF[0], dlp[0].param[i].defValF[1]));
                      break;
                    case HmapDetLayerProps::Param::PT_float: b.setReal(dlp[0].param[i].name, dlp[0].param[i].defValF[0]); break;
                    case HmapDetLayerProps::Param::PT_tex: b.setStr(dlp[0].param[i].name, ""); break;
                    case HmapDetLayerProps::Param::PT_color:
                      b.setE3dcolor(dlp[0].param[i].name, e3dcolor(Color4(dlp[0].param[i].defValF)));
                      break;
                  }
                if (dlp[0].needsMask)
                  b.setStr("lMask", "");
              }
              bc.addNewBlock(gl.elcLayers.getBlock(i));
            }
            gl.elcLayers = bc;
            break;

          case PropPanel::EXT_BUTTON_UP:
            for (int i = 0; i < gl.elcLayers.blockCount(); i++)
            {
              if (i == lidx - 1)
                bc.addNewBlock(gl.elcLayers.getBlock(i + 1));
              else if (i == lidx)
                bc.addNewBlock(gl.elcLayers.getBlock(i - 1));
              else
                bc.addNewBlock(gl.elcLayers.getBlock(i));
            }
            gl.elcLayers = bc;
            break;

          case PropPanel::EXT_BUTTON_DOWN:
            for (int i = 0; i < gl.elcLayers.blockCount(); i++)
            {
              if (i == lidx)
                bc.addNewBlock(gl.elcLayers.getBlock(i + 1));
              else if (i == lidx + 1)
                bc.addNewBlock(gl.elcLayers.getBlock(i - 1));
              else
                bc.addNewBlock(gl.elcLayers.getBlock(i));
            }
            gl.elcLayers = bc;
            break;
        }
        panel->setPostEvent(1);
        gl.reinitDrdHandle();
        gl.updateElcMasks();
      }
      else if (pid >= PID_BASE && pid < PID_BASE + PID_ELC__CNT * gl.elcLayers.blockCount())
      {
        dag::Span<HmapDetLayerProps> dlp = HmapLandPlugin::hmlService->getDetLayerClassList();
        int lidx = (pid - PID_BASE) / PID_ELC__CNT;
        int lpid = (pid - PID_BASE) % PID_ELC__CNT;
        DataBlock &b = *gl.elcLayers.getBlock(lidx);
        int idx = HmapLandPlugin::hmlService->resolveDetLayerClassName(b.getStr("lName", ""));

        if (lpid == PID_ELC_SEL_MASK)
        {
          if (HmapLandPlugin::self->getEditedScriptImage())
          {
            HmapLandPlugin::self->editScriptImage(NULL);
            HmapLandPlugin::self->setShowBlueWhiteMask();
            HmapLandPlugin::self->refillPanel();
            panel->setPostEvent(1);
          }
          const char *name = HmapLandPlugin::self->pickScriptImage(b.getStr("lMask"), 8);
          if (name)
          {
            b.setStr("lMask", name);
            panel->setCaption(pid, *name ? name : "-- no mask --");
            gl.updateElcMasks();
            if (gl.drdHandle)
            {
              HmapLandPlugin::hmlService->updateDetailRenderData(gl.drdHandle, gl.elcLayers);
              HmapLandPlugin::hmlService->invalidateClipmap(false);
            }
          }
        }
        else if (idx >= 0 && lpid >= PID_ELC_PARAM_BASE)
        {
          int pidx = lpid - PID_ELC_PARAM_BASE;
          if (dlp[idx].param[pidx].type == HmapDetLayerProps::Param::PT_tex)
            if (const char *tex = DAEDITOR3.selectAssetX(b.getStr(dlp[idx].param[pidx].name, ""), "Select texture", "tex"))
            {
              b.setStr(dlp[idx].param[pidx].name, tex);
              panel->setCaption(pid, String(0, "%s: %s", dlp[idx].param[pidx].name, tex));
              HmapLandPlugin::hmlService->updateDetailRenderData(gl.drdHandle, gl.elcLayers);
              HmapLandPlugin::hmlService->invalidateClipmap(false);
            }
        }
      }
    }
    void onDoubleClick(int pid, PropPanel::ContainerPropertyControl *panel) override {}
    void onPostEvent(int pid, PropPanel::ContainerPropertyControl *panel) override
    {
      if (pid == 1)
        refillPanel();
    }

    PostScriptParamLandLayer &gl;
    PropPanel::DialogWindow *dlg;
    IPoint2 windowPosition;
    IPoint2 windowSize;
    DataBlock panelState;
  };
  PropsDlg *dlg;
  DataBlock elcLayers;
  NameMap elcMaskList;
  int elcMaskCurIdx, elcMaskImageIndex, elcMaskImageBpp;

  void *drdHandle;


  PostScriptParamLandLayer(int slot_idx, int land_idx, const char *nm) :
    ScriptParam(nm), slotIdx(slot_idx), pidBase(-1), imageIndex(-1), imageBpp(-1), enabled(true), landIdx(land_idx), maskConv(0)
  {
    memset(&lc1, 0, sizeof(lc1));
    memset(&lc2, 0, sizeof(lc2));
    memset(&ht, 0, sizeof(ht));
    memset(&ang, 0, sizeof(ang));
    memset(&curv, 0, sizeof(curv));
    wtMode = 0;
    badDetTex = false;
    detIdx = slotIdx;
    detRectMappedMask = false;
    editableLandClass = false;
    drdHandle = NULL;
    elcMaskCurIdx = elcMaskImageIndex = elcMaskImageBpp = -1;
    areaSelect = 0;
    // Defaults mirror load() / save() asymmetry: writeDetTex and writeLand1 are common
    // (save only writes them when cleared), writeLand2 and writeImportance are rare.
    writeDetTex = writeLand1 = true;
    writeLand2 = writeImportance = false;
    writeDetTexThres = writeLand1Thres = writeLand2Thres = 1e-3;
    layerColor = defaultLayerColor(nm);
    LandClassSlotsManager::subscribeLandClassUpdateNotify(this);
    dlg = new PropsDlg(*this);
  }
  ~PostScriptParamLandLayer() override
  {
    HmapLandPlugin::hmlService->destroyDetailRenderData(drdHandle);
    unregisterAsset(lc1);
    unregisterAsset(lc2);
    LandClassSlotsManager::unsubscribeLandClassUpdateNotify(this);
    del_it(dlg);
  }
  void *get_interface(int id) override { return id == 'Layr' ? this : NULL; }

  void fillParams(PropPanel::ContainerPropertyControl &_panel, int &pid) override
  {
    pidBase = pid;
    PropPanel::ContainerPropertyControl &panel = *_panel.createExtGroup(pidBase + PID_LAYER_CAPTION,
      String(0, paramName.empty() ? "Layer #%d" : "Layer #%d: %s", slotIdx + 1, paramName));
    _panel.setInt(pidBase + PID_LAYER_CAPTION, (1 << PropPanel::EXT_BUTTON_INSERT) | (1 << PropPanel::EXT_BUTTON_REMOVE) |
                                                 (1 << PropPanel::EXT_BUTTON_RENAME) | (1 << PropPanel::EXT_BUTTON_UP) |
                                                 (1 << PropPanel::EXT_BUTTON_DOWN));

    panel.createCheckBox(pidBase + PID_LAYER_ENABLED, "Layer enabled", enabled);
    panel.createButton(pidBase + PID_EDIT_PROPS, "Props...", true, false);
    panel.createStatic(pidBase + PID_EXPR_SUMMARY, exprText.empty() ? "1" : exprText.str());
    panel.createColorBox(pidBase + PID_LAYER_COLOR, "Color (weights overlay)", layerColor);
    panel.createButton(pidBase + PID_SEL_LC1, lc1.asset.empty() ? "-- no land#1 --" : ::dd_get_fname(lc1.asset));
    panel.createButton(pidBase + PID_SEL_LC2, lc2.asset.empty() ? "-- no land#2 --" : ::dd_get_fname(lc2.asset));

    // mask
    bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this && elcMaskCurIdx < 0;
    panel.createSeparator(0);
    panel.createButton(pidBase + PID_SEL_MASK, maskName.empty() ? "-- no mask --" : maskName);
    panel.createButton(pidBase + PID_EDIT_MASK, isEditImage ? "Done" : "Edit", !maskName.empty(), false);
    // panel.setEnabledById(pidBase+PID_SEL_MASK, maskConv);
    panel.setEnabledById(pidBase + PID_EDIT_MASK, maskConv);

    // grass mask
    /*int landclassId = HmapLandPlugin::self->getLandclassIndex(lc1.asset);
    String grassMaskName("-- grass mask --");
    if (landclassId != -1)
      grassMaskName = HmapLandPlugin::self->landClassInfo[landclassId].grassMaskName;
    panel.createButton(pidBase + PID_SEL_GRASS_MASK, grassMaskName.str());*/

    // editable land class masks
    elcMaskList.reset();
    if (editableLandClass)
    {
      dag::Span<HmapDetLayerProps> dlp = HmapLandPlugin::hmlService->getDetLayerClassList();
      for (int i = 0; i < elcLayers.blockCount(); i++)
      {
        const DataBlock &b = *elcLayers.getBlock(i);
        int idx = HmapLandPlugin::hmlService->resolveDetLayerClassName(b.getStr("lName", ""));

        if (idx >= 0 && dlp[idx].needsMask && *b.getStr("lMask", ""))
          elcMaskList.addNameId(b.getStr("lMask"));
      }

      if (elcMaskList.nameCount())
      {
        panel.createSeparator(0);
        for (int i = 0; i < elcMaskList.nameCount() && PID_ELC_EDIT_MASK_BASE + i <= PID_ELC_EDIT_MASK_LAST; i++)
        {
          bool edit = HmapLandPlugin::self->getEditedScriptImage() == this && elcMaskCurIdx == i;
          panel.createButton(pidBase + PID_ELC_EDIT_MASK_BASE + i,
            String(0, edit ? "%s   - Done!" : "elcMask:    %s", elcMaskList.getName(i)));
        }
      }
    }

    pid += PID__COUNT;
    prepareDrdHandle();
  }
  void updateElcMasks()
  {
    NameMap list;
    bool missing = false;
    if (editableLandClass)
    {
      dag::Span<HmapDetLayerProps> dlp = HmapLandPlugin::hmlService->getDetLayerClassList();
      for (int i = 0; i < elcLayers.blockCount(); i++)
      {
        const DataBlock &b = *elcLayers.getBlock(i);
        int idx = HmapLandPlugin::hmlService->resolveDetLayerClassName(b.getStr("lName", ""));

        if (idx >= 0 && dlp[idx].needsMask && *b.getStr("lMask", ""))
        {
          if (elcMaskList.getNameId(b.getStr("lMask")) == -1)
          {
            missing = true;
            break;
          }
          else
            list.addNameId(b.getStr("lMask"));
        }
      }
    }
    if (missing || list.nameCount() != elcMaskList.nameCount())
    {
      HmapLandPlugin::hmlService->destroyDetailRenderData(drdHandle);
      drdHandle = NULL;
      HmapLandPlugin::self->refillPanel();
    }
  }
  void prepareDrdHandle() {}
  void reinitDrdHandle() {}
  void updateDrdTex() {}


  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid == pidBase + PID_LAYER_ENABLED)
    {
      enabled = panel.getBool(pid);
      // Toggling enabled changes which layers contribute var/let declarations to the
      // shared symbol table, so peer expressions need to be recompiled against the
      // updated set (and own expression compile state synced).
      HmapLandPlugin::self->recompileGenLayerExpressions();
    }
    else if (pid == pidBase + PID_LAYER_COLOR)
    {
      layerColor = panel.getColor(pid);
      if (HmapLandPlugin::self->isShowingLandClassColors())
        HmapLandPlugin::self->refreshLandClassColorsTex();
    }
  }

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (pid == pidBase + PID_SEL_LC1)
    {
      const char *lc_asset = DAEDITOR3.selectAssetX(lc1.asset, "Select primary landclass", "land");

      if (lc_asset)
      {
        lc1.asset = lc_asset;
        panel.setCaption(pid, lc_asset);
        // Changing lc1 re-shuffles the plugin-wide lex order, so recompute and
        // re-register every layer rather than just this one.
        HmapLandPlugin::self->rebuildLandSlots();
        updateEditableLandClassFlag();

        HmapLandPlugin::self->getLandClassMgr().regenerateObjects(HmapLandPlugin::self->getlandClassMap());
        HmapLandPlugin::hmlService->invalidateClipmap(true);
      }
    }
    else if (pid == pidBase + PID_SEL_LC2)
    {
      const char *lc_asset = DAEDITOR3.selectAssetX(lc2.asset, "Select secondary landclass", "land");

      if (lc_asset)
      {
        lc2.asset = lc_asset;
        panel.setCaption(pid, lc_asset);
        // Changing lc2 can also shift the lex order across all layers.
        HmapLandPlugin::self->rebuildLandSlots();

        HmapLandPlugin::self->getLandClassMgr().regenerateObjects(HmapLandPlugin::self->getlandClassMap());
        HmapLandPlugin::hmlService->invalidateClipmap(true);
      }
    }
    else if (pid == pidBase + PID_EDIT_PROPS)
    {
      if (dlg->isVisible())
      {
        panel.setCaption(pid, "Props...");
        dlg->hide();
      }
      else
      {
        panel.setCaption(pid, "-hide-");
        dlg->show();
      }
    }
    else if (pid == -PropPanel::DIALOG_ID_CLOSE)
    {
      panel.setCaption(pidBase + PID_EDIT_PROPS, "Props...");
    }
    else if (pid == pidBase + PID_SEL_MASK)
    {
      if (HmapLandPlugin::self->getEditedScriptImage() == this)
      {
        finishEdit(panel);
        HmapLandPlugin::self->editScriptImage(this);
        HmapLandPlugin::self->setShowBlueWhiteMask();
      }
      if (pickMaskAndApplyState())
      {
        const char *caption = !maskName.empty() ? maskName.str() : "-- no mask --";
        panel.setCaption(pid, caption);
        panel.setEnabledById(pidBase + PID_EDIT_MASK, maskConv);
        if (dlg && dlg->dlg)
        {
          dlg->dlg->getPanel()->setCaption(dlg->PID_SEL_MASK, caption);
          dlg->dlg->getPanel()->setInt(dlg->PID_WTR_MASK_CONV, maskConv);
          dlg->dlg->getPanel()->setEnabledById(dlg->PID_MASK_REFSYS, HmapLandPlugin::self->hasDetaledRect() && maskConv);
          dlg->dlg->getPanel()->setEnabledById(dlg->PID_SEL_MASK, maskConv);
        }
      }
    }
    else if (pid == pidBase + PID_EDIT_MASK)
    {
      ScriptParam *curEd = HmapLandPlugin::self->getEditedScriptImage();
      if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(curEd))
        gl->finishEdit(panel);
      else if (ScriptParamMask *sm = ScriptParamMask::cast(curEd))
        sm->finishEdit(panel);

      if (imageIndex < 0)
        return;

      HmapLandPlugin::self->editScriptImage(this);
      bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this;
      panel.setCaption(pid, isEditImage ? "Done" : "Edit");
      HmapLandPlugin::self->setShowBlueWhiteMask();

      if (!isEditImage)
        HmapLandPlugin::self->setSelectMode();
    }
    else if (pid == pidBase + PID_SEL_GRASS_MASK)
    {
      /*String fname = wingw::file_open_dlg(NULL, "Select grass mask",
        "tex (*.tga)|*.tga|All files (*.*)|*.*", "tga");

      if (!fname.length())
        return;

      String grassMaskName = ::get_file_name_wo_ext(fname) + "*";
      panel.setCaption(pid, grassMaskName.isEmpty() ? "-- grass mask --" : grassMaskName);

      //replace grass_mask for all landclasses with given name (lc1.asset)
      if (lc1.asset != NULL)
      {
        int landclassId = HmapLandPlugin::self->getLandclassIndex(lc1.asset);
        if (landclassId != -1)
        {
          HmapLandPlugin::self->replaceGrassMask(HmapLandPlugin::self->landClasses[landclassId].colormapId, grassMaskName.str());
          HmapLandPlugin::self->hmlService->invalidateClipmap(true);
        }
      }*/
    }
    else if (pid == pidBase + PID_LAYER_CAPTION)
    {
      switch (panel.getInt(pid))
      {
        case PropPanel::EXT_BUTTON_REMOVE:
          if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                "Do you really want to delete generation layer #%d %s?", slotIdx + 1, paramName) == wingw::MB_ID_YES)
            if (HmapLandPlugin::self->delGenLayer(this))
            {
              if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                    "Generation layers changed.\nRegenerate all?\n(generation may be started manually with Ctrl-G later)") ==
                  wingw::MB_ID_YES)
                HmapLandPlugin::self->refillPanel(true);
              else
                HmapLandPlugin::self->refillPanel();
            }
          break;

        case PropPanel::EXT_BUTTON_INSERT:
        {
          PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(125), "Insert generation layer");
          dialog->setInitialFocus(PropPanel::DIALOG_ID_NONE);
          PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
          panel->createEditBox(0, "Enter generation layer name:");
          panel->setFocusById(0);

          int ret = dialog->showDialog();
          if (ret == PropPanel::DIALOG_ID_OK)
          {
            SimpleString newName = panel->getText(0);
            if (!HmapLandPlugin::isValidGenLayerName(newName))
              wingw::message_box(wingw::MBS_EXCL, "Invalid layer name",
                "Layer name '%s' contains characters that aren't allowed.\n\n"
                "Use only letters, digits, and underscore so the mask_<name> binding can be referenced from expressions.",
                newName.str());
            else if (HmapLandPlugin::self->isGenLayerNameInUse(newName))
              wingw::message_box(wingw::MBS_EXCL, "Duplicate name",
                "A generation layer named '%s' already exists. Pick a unique name.", newName.str());
            // mask_<paramName> enters the same shared varMap as typed vars, so a
            // layer name whose mask_<name> collides with a typed-var slot would
            // silently alias that slot at the next recompile. Reject up front so
            // the artist hits a clear error rather than a bake-time aliasing bug.
            else if (SimpleString conflict;
                     HmapLandPlugin::self->collidesWithTypedVar(String(0, "mask_%s", newName.str()).str(), conflict))
              wingw::message_box(wingw::MBS_EXCL, "Name collides with typed variable",
                "Layer name '%s' would create binding 'mask_%s' which collides with typed variable '%s'. "
                "Pick a different layer name or rename the typed variable first.",
                newName.str(), newName.str(), conflict.str());
            // register_layer_mask_names inserts mask_<paramName> into the shared
            // varMap before commonExprText / layer exprText is parsed; a `var` /
            // `let` declaring the same name in any of those texts would then make
            // the next recompile fail. Same up-front rejection as the typed-var
            // Add path.
            else if (SimpleString site; HmapLandPlugin::self->findVarLetDeclSite(String(0, "mask_%s", newName.str()).str(), site))
              wingw::message_box(wingw::MBS_EXCL, "Name already declared",
                "Layer name '%s' would create binding 'mask_%s' which is already declared as 'var' or 'let' in %s. "
                "Pick a different layer name or remove the declaration first.",
                newName.str(), newName.str(), site.str());
            else
            {
              HmapLandPlugin::self->addGenLayer(newName, slotIdx);
              if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                    "Generation layers changed.\nRegenerate all?\n(generation may be started manually with Ctrl-G later)") ==
                  wingw::MB_ID_YES)
                HmapLandPlugin::self->refillPanel(true);
              else
                HmapLandPlugin::self->refillPanel();
            }
          }
          DAGORED2->deleteDialog(dialog);
        }
        break;

        case PropPanel::EXT_BUTTON_UP:
          if (HmapLandPlugin::self->moveGenLayer(this, true))
          {
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                  "Generation layers changed.\nRegenerate all?\n(generation may be started manually with Ctrl-G later)") ==
                wingw::MB_ID_YES)
              HmapLandPlugin::self->refillPanel(true);
            else
              HmapLandPlugin::self->refillPanel();
          }
          break;

        case PropPanel::EXT_BUTTON_DOWN:
          if (HmapLandPlugin::self->moveGenLayer(this, false))
          {
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                  "Generation layers changed.\nRegenerate all?\n(generation may be started manually with Ctrl-G later)") ==
                wingw::MB_ID_YES)
              HmapLandPlugin::self->refillPanel(true);
            else
              HmapLandPlugin::self->refillPanel();
          }
          break;

        case PropPanel::EXT_BUTTON_RENAME:
        {
          PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(125), "Rename generation layer");
          dialog->setInitialFocus(PropPanel::DIALOG_ID_NONE);
          PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
          panel->createEditBox(0, "Change generation layer name:", paramName);
          panel->setFocusById(0);

          int ret = dialog->showDialog();
          if (ret == PropPanel::DIALOG_ID_OK)
          {
            SimpleString newName = panel->getText(0);
            // Reject names that aren't valid identifiers (mask_<name> would not parse,
            // making the layer's mask silently unreferenceable from expressions).
            // Reject duplicates: with two layers named identically, mask_<name>
            // resolves to the first match and silently aliases later expressions to
            // the wrong layer. `this` is excluded so renaming to the same name is a
            // no-op rather than a self-conflict.
            if (!HmapLandPlugin::isValidGenLayerName(newName))
              wingw::message_box(wingw::MBS_EXCL, "Invalid layer name",
                "Layer name '%s' contains characters that aren't allowed.\n\n"
                "Use only letters, digits, and underscore so the mask_<name> binding can be referenced from expressions.",
                newName.str());
            else if (HmapLandPlugin::self->isGenLayerNameInUse(newName, this))
              wingw::message_box(wingw::MBS_EXCL, "Duplicate name",
                "A generation layer named '%s' already exists. Pick a unique name.", newName.str());
            // Same mask_<name> collision check as Insert: rename must not move a
            // layer onto a typed-var's parser-visible slot.
            else if (SimpleString conflict;
                     HmapLandPlugin::self->collidesWithTypedVar(String(0, "mask_%s", newName.str()).str(), conflict))
              wingw::message_box(wingw::MBS_EXCL, "Name collides with typed variable",
                "Layer name '%s' would create binding 'mask_%s' which collides with typed variable '%s'. "
                "Pick a different layer name or rename the typed variable first.",
                newName.str(), newName.str(), conflict.str());
            // Same var/let collision check as Insert: rename must not move a
            // layer onto a name declared in commonExprText / any layer exprText.
            else if (SimpleString site; HmapLandPlugin::self->findVarLetDeclSite(String(0, "mask_%s", newName.str()).str(), site))
              wingw::message_box(wingw::MBS_EXCL, "Name already declared",
                "Layer name '%s' would create binding 'mask_%s' which is already declared as 'var' or 'let' in %s. "
                "Pick a different layer name or remove the declaration first.",
                newName.str(), newName.str(), site.str());
            else
            {
              paramName = newName;
              // Renaming shifts mask_<name> bindings -- recompile so peer expressions
              // pick up the new varMap.
              HmapLandPlugin::self->recompileGenLayerExpressions();
              HmapLandPlugin::self->refillPanel();
            }
          }
          DAGORED2->deleteDialog(dialog);
        }
        break;
      }
    }
    else if (pid >= pidBase + PID_ELC_EDIT_MASK_BASE && pid <= pidBase + PID_ELC_EDIT_MASK_LAST)
    {
      int idx = pid - (pidBase + PID_ELC_EDIT_MASK_BASE);
      ScriptParam *curEd = HmapLandPlugin::self->getEditedScriptImage();
      if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(curEd))
        gl->finishEdit(panel);
      else if (ScriptParamMask *sm = ScriptParamMask::cast(curEd))
        sm->finishEdit(panel);

      HmapLandPlugin::self->editScriptImage(this, idx);
      bool isEditImage = HmapLandPlugin::self->getEditedScriptImage() == this;
      panel.setCaption(pid, String(0, isEditImage ? "%s    - Done!" : "elcMask:    %s", elcMaskList.getName(idx)));
      HmapLandPlugin::self->setShowBlueWhiteMask();

      if (!isEditImage)
        HmapLandPlugin::self->setSelectMode();
    }
  }
  bool onPPChangeEx(int pid, PropPanel::ContainerPropertyControl &p) override
  {
    if (pid < pidBase || pid >= pidBase + PID__COUNT)
      return false;
    onPPChange(pid, p);
    ;
    return true;
  }
  bool onPPBtnPressedEx(int pid, PropPanel::ContainerPropertyControl &p) override
  {
    if (pid < pidBase || pid >= pidBase + PID__COUNT)
      return false;
    onPPBtnPressed(pid, p);
    return true;
  }

  virtual void finishEdit(PropPanel::ContainerPropertyControl &panel)
  {
    if (elcMaskCurIdx < 0)
      panel.setCaption(pidBase + PID_EDIT_MASK, "Edit");
    else
    {
      panel.setCaption(pidBase + PID_ELC_EDIT_MASK_BASE + elcMaskCurIdx,
        String(0, "elcMask:    %s", elcMaskList.getName(elcMaskCurIdx)));
      elcMaskCurIdx = elcMaskImageIndex = elcMaskImageBpp = -1;
    }
  }

  void save(DataBlock &blk) override
  {
    blk.setStr("name", paramName);
    if (!enabled)
      blk.setBool("enabled", enabled);
    if (wtMode > 0)
      blk.setInt("wtMode", wtMode);
    if (!lc1.asset.empty())
      blk.setStr("lc1", lc1.asset);
    if (!lc2.asset.empty())
      blk.setStr("lc2", lc2.asset);
    // lcIdx is no longer persisted: slot is derived from lex-sorted lc1/lc2 names
    // in HmapLandPlugin::rebuildLandSlots().

    if (areaSelect)
      blk.setInt("area_select", areaSelect);

    if (layerColor != defaultLayerColor(paramName))
      blk.setE3dcolor("color", layerColor);

    // The expression string is authoritative when useExpr is set, but we keep writing the
    // legacy data-driven fields too (until legacy support is removed). That way older
    // binaries and tools that only understand the legacy format can still load the file,
    // and a user who clears `wt` in the .blk recovers the original layer configuration
    // instead of a factory-reset zeroed layer.
    if (useExpr)
      blk.setStr("wt", exprText);

    if (maskConv)
      blk.setInt("mask_conv", maskConv);
    if (!maskName.empty())
      blk.setStr("mask", maskName);
    if (detRectMappedMask)
      blk.setBool("maskDRM", detRectMappedMask);

    if (ht.conv)
      blk.setInt("ht_conv", ht.conv);
    if (ht.v0)
      blk.setReal("ht_v0", ht.v0);
    if (ht.dv)
      blk.setReal("ht_dv", ht.dv);

    if (ang.conv)
      blk.setInt("ang_conv", ang.conv);
    if (ang.v0)
      blk.setReal("ang_v0", ang.v0);
    if (ang.dv)
      blk.setReal("ang_dv", ang.dv);

    if (curv.conv)
      blk.setInt("curv_conv", curv.conv);
    if (curv.v0)
      blk.setReal("curv_v0", curv.v0);
    if (curv.dv)
      blk.setReal("curv_dv", curv.dv);

    if (!writeDetTex)
      blk.setBool("writeDetTex", writeDetTex);
    if (!writeLand1)
      blk.setBool("writeLand1", writeLand1);
    if (writeLand2)
      blk.setBool("writeLand2", writeLand2);
    if (writeImportance)
      blk.setBool("writeImportance", writeImportance);
    if (writeDetTexThres)
      blk.setReal("writeDetTexThres", writeDetTexThres);
    if (writeLand1Thres)
      blk.setReal("writeLand1Thres", writeLand1Thres);
    if (writeLand2Thres)
      blk.setReal("writeLand2Thres", writeLand2Thres);

    int nid = elcLayers.getNameId("elc_layer");
    for (int i = 0; i < elcLayers.blockCount(); i++)
      if (elcLayers.getBlock(i)->getBlockNameId() == nid)
        blk.addNewBlock(elcLayers.getBlock(i));
  }

  void load(const DataBlock &blk) override
  {
    paramName = blk.getStr("name", "");
    enabled = blk.getBool("enabled", true);
    layerColor = blk.getE3dcolor("color", defaultLayerColor(paramName));
    wtMode = blk.getInt("wtMode", blk.getBool("sumWt", false) ? 1 : 0);
    lc1.asset = blk.getStr("lc1", NULL);
    lc2.asset = blk.getStr("lc2", NULL);
    landIdx = blk.getInt("lcIdx", -1);

    areaSelect = blk.getInt("area_select", 0);

    maskConv = blk.getInt("mask_conv", 0);
    maskName = blk.getStr("mask", NULL);
    detRectMappedMask = blk.getBool("maskDRM", false);

    ht.conv = blk.getInt("ht_conv", 0);
    ht.v0 = blk.getReal("ht_v0", 0);
    ht.dv = blk.getReal("ht_dv", 0);
    ang.conv = blk.getInt("ang_conv", 0);
    ang.v0 = blk.getReal("ang_v0", 0);
    ang.dv = blk.getReal("ang_dv", 0);
    curv.conv = blk.getInt("curv_conv", 0);
    curv.v0 = blk.getReal("curv_v0", 0);
    curv.dv = blk.getReal("curv_dv", 0);
    writeDetTex = blk.getBool("writeDetTex", true);
    writeLand1 = blk.getBool("writeLand1", true);
    writeLand2 = blk.getBool("writeLand2", false);
    writeImportance = blk.getBool("writeImportance", false);
    writeDetTexThres = blk.getReal("writeDetTexThres", 0);
    writeLand1Thres = blk.getReal("writeLand1Thres", 0);
    writeLand2Thres = blk.getReal("writeLand2Thres", 0);

    imageIndex = HmapLandPlugin::self->getScriptImage(maskName, 1, -1);
    imageBpp = HmapLandPlugin::self->getScriptImageBpp(imageIndex);

    elcLayers.reset();
    int nid = blk.getNameId("elc_layer");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
        elcLayers.addNewBlock(blk.getBlock(i));

    dlg->updateTitle();

    // Expression evaluator
    const char *wtStr = blk.getStr("wt", "");
    if (wtStr[0])
    {
      useExpr = true;
      exprText = wtStr;
    }
    else
    {
      useExpr = false;
      exprText = buildExprFromOldFormat().c_str();
    }
    // No compile here: peer layers may not be loaded yet, so mask_<other> references
    // would spuriously fail. loadGenLayers() triggers a full recompile once every
    // layer is in place; exprValid stays false until then.
  }

  void registerAssets()
  {
    registerAsset(lc1);
    registerAsset(lc2);
    updateEditableLandClassFlag();
  }

  void changeSlotIdx(int idx)
  {
    unregisterAsset(lc1);
    unregisterAsset(lc2);
    detIdx = slotIdx = idx;
    registerAsset(lc1);
    registerAsset(lc2);
    dlg->updateTitle();
    updateEditableLandClassFlag();
  }

  void swapSlotIdx(PostScriptParamLandLayer &other)
  {
    unregisterAsset(lc1);
    unregisterAsset(lc2);
    other.unregisterAsset(other.lc1);
    other.unregisterAsset(other.lc2);
    int tmp = slotIdx;
    slotIdx = other.slotIdx;
    other.slotIdx = tmp;
    registerAsset(lc1);
    registerAsset(lc2);
    other.registerAsset(other.lc1);
    other.registerAsset(other.lc2);
    dlg->updateTitle();
    other.dlg->updateTitle();
    updateEditableLandClassFlag();
    other.updateEditableLandClassFlag();
  }


  void registerAsset(LandClassRec &lc)
  {
    if (lc.asset.empty())
      return unregisterAsset(lc);

    void *handle = HmapLandPlugin::self->getLayersHandle();
    int lc_li = HmapLandPlugin::hmlService->getBitLayerIndexByName(handle, &lc == &lc1 ? "land" : "adds");
    int lc_base = lc_li < 0 ? 0 : (1 << HmapLandPlugin::hmlService->getBitLayersList(handle)[lc_li].bitCount) - 1;
    if (lc_li >= 0 && landIdx >= 0)
      HmapLandPlugin::self->getLandClassMgr().setLandClass(lc_li, lc_base - landIdx, lc.asset);

    if (&lc == &lc1)
    {
      int phys_li = HmapLandPlugin::hmlService->getBitLayerIndexByName(handle, "phys");
      int phys_li_base = phys_li < 0 ? 0 : (1 << HmapLandPlugin::hmlService->getBitLayersList(handle)[phys_li].bitCount) - 1;
      if (phys_li >= 0 && landIdx >= 0)
        HmapLandPlugin::self->getLandClassMgr().setLandClass(phys_li, phys_li_base - landIdx, lc.asset);
    }

    regetAsset(lc);
    if (&lc == &lc2)
      return;

    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    if (det_layer_id >= 0 && slotIdx >= 0 && lc.assetData)
      HmapLandPlugin::self->setDetailTexSlot(detIdx = slotIdx, lc.asset);
    else
      detIdx = -1;
    badDetTex = det_layer_id < 0; //    badDetTex = det_layer_id < 0 || !lc.assetData->detTex;
    if (badDetTex && writeDetTex)
      DAEDITOR3.conError("bad asset <%s> to write detTex weight in layer %d", lc.asset, slotIdx);
  }

  void unregisterAsset(LandClassRec &lc)
  {
    if (!HmapLandPlugin::self)
      return;

    LandClassSlotsManager::releaseAsset(lc.assetData);
    lc.assetData = NULL;

    void *handle = HmapLandPlugin::self->getLayersHandle();
    int lc_li = HmapLandPlugin::hmlService->getBitLayerIndexByName(handle, &lc == &lc1 ? "land" : "adds");
    int lc_base = lc_li < 0 ? 0 : (1 << HmapLandPlugin::hmlService->getBitLayersList(handle)[lc_li].bitCount) - 1;
    if (lc_li >= 0 && landIdx >= 0)
      HmapLandPlugin::self->getLandClassMgr().setLandClass(lc_li, lc_base - landIdx, NULL);

    if (&lc == &lc1)
    {
      int phys_li = HmapLandPlugin::hmlService->getBitLayerIndexByName(handle, "phys");
      int phys_li_base = phys_li < 0 ? 0 : (1 << HmapLandPlugin::hmlService->getBitLayersList(handle)[phys_li].bitCount) - 1;
      if (phys_li >= 0 && landIdx >= 0)
        HmapLandPlugin::self->getLandClassMgr().setLandClass(phys_li, phys_li_base - landIdx, NULL);
    }

    if (&lc == &lc2)
      return;

    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    if (det_layer_id >= 0 && slotIdx >= 0)
      HmapLandPlugin::self->setDetailTexSlot(detIdx = slotIdx, NULL);
    else
      detIdx = -1;
    badDetTex = true;
  }

  void regetAsset(LandClassRec &lc)
  {
    landclass::AssetData *a = lc.assetData;
    lc.assetData = LandClassSlotsManager::getAsset(lc.asset);
    LandClassSlotsManager::releaseAsset(a);
  }

  // IAssetUpdateNotify interface
  void onLandClassAssetChanged(landclass::AssetData *data) override
  {
    bool changed = lc1.assetData == data || lc2.assetData == data;
    if (lc1.assetData == data)
    {
      unregisterAsset(lc1);
      registerAsset(lc1);
      updateEditableLandClassFlag();
    }
    if (lc2.assetData == data)
    {
      unregisterAsset(lc2);
      registerAsset(lc2);
    }
    if (changed)
      HmapLandPlugin::self->getLandClassMgr().regenerateObjects(HmapLandPlugin::self->getlandClassMap());
  }
  void onLandClassAssetTexturesChanged(landclass::AssetData *data) override {}
  void onSplineClassAssetChanged(splineclass::AssetData *data) override {}


  inline float getMask(float fx, float fy) const;
  inline void setMask(float fx, float fy, int c);
  inline float getMaskDirect(float fx, float fy) const;
  inline void setMaskDirect(float fx, float fy, int c);
  inline float getMaskDirectEx(float fx, float fy) const;
  inline void setMaskDirectEx(float fx, float fy, int c);
  inline float calcWeight(const LandColorGenData &lcg, float height, float angDiff, float curvature) const;

  static inline PostScriptParamLandLayer *cast(ScriptParam *es)
  {
    if (es && es->get_interface('Layr'))
      return static_cast<PostScriptParamLandLayer *>(es);
    return NULL;
  }
  void updateEditableLandClassFlag()
  {
    bool new_elc = lc1.assetData && lc1.assetData->editableMasks;
    if (new_elc != editableLandClass)
    {
      editableLandClass = new_elc;
      HmapLandPlugin::hmlService->destroyDetailRenderData(drdHandle);
      drdHandle = NULL;
      dlg->refillPanel();
    }
    else
      updateElcMasks();
  }
};

// Register the six base variables (height/angle/.../world_y) into vm/nv. Always
// emits exactly LcExprContext::VAR_COUNT bindings starting at varId 0. Mirrors
// the FLOAT type into varTypes so the parser sees them as ordinary floats.
static void register_base_vars(lcexpr::NameMap &vm, uint16_t &nv, lcexpr::VarTypeTable &vt)
{
  lcexpr::register_var_typed(vm, nv, vt, "height", lcexpr::TYPE_FLOAT);
  lcexpr::register_var_typed(vm, nv, vt, "angle", lcexpr::TYPE_FLOAT);
  lcexpr::register_var_typed(vm, nv, vt, "curvature", lcexpr::TYPE_FLOAT);
  lcexpr::register_var_typed(vm, nv, vt, "mask", lcexpr::TYPE_FLOAT);
  lcexpr::register_var_typed(vm, nv, vt, "world_x", lcexpr::TYPE_FLOAT);
  lcexpr::register_var_typed(vm, nv, vt, "world_y", lcexpr::TYPE_FLOAT);
  G_ASSERT(nv == LcExprContext::VAR_COUNT);
}

// Build the runtime CubicCurveSampler from a TypedVar's control points. Mirrors
// the daFx editor's serialization path (ScriptHelpers::TunedCubicCurveParam::
// saveData) but writes directly into the sampler instead of going through a
// BinDumpSaveCB roundtrip. PropPanel's CubicPolynomCB / CubicPSplineCB live in
// libTools/propPanel/commonWindow/w_curve_math.h -- private to libTools, but
// HeightmapLand's jamfile adds that path so we can use them here without
// duplicating the cubic-spline solver.
static void build_curve_sampler(CubicCurveSampler &out, const TypedVar &v)
{
  // Wipe any prior spline storage; the entry may be reused on recompile. Route
  // through CubicCurveSampler::mem_free when the engine has installed a custom
  // allocator (e.g. memory tracker, restricted heap) so we don't mismatch the
  // alloc side. Mirrors the assignment / load paths in dag_curveParams.h.
  if (out.splineCoef)
  {
    if (CubicCurveSampler::mem_free)
      CubicCurveSampler::mem_free(out.splineCoef);
    else
      delete[] out.splineCoef;
    out.splineCoef = nullptr;
  }
  out.type = 1;
  mem_set_0(out.coef);

  if (v.curvePtCnt < 1)
  {
    out.setCoef(0.f, 0.f, 0.f, 0.f);
    return;
  }

  PropPanel::CubicPolynomCB polyCb;
  PropPanel::CubicPSplineCB splineCb;
  PropPanel::ICurveControlCallback *cb = (v.curveType == 0) ? static_cast<PropPanel::ICurveControlCallback *>(&polyCb)
                                                            : static_cast<PropPanel::ICurveControlCallback *>(&splineCb);
  for (int i = 0; i < v.curvePtCnt; i++)
    cb->addNewControlPoint(v.curvePos[i]);

  Tab<Point2> segc(tmpmem);
  cb->getCoefs(segc); // 4 entries per segment, polynomial coefficients in y

  // Cubic polynom mode produces exactly 4 entries (one segment over [0,1]).
  // Hermite-spline mode produces 4*(ptCnt-1) entries (one segment per gap).
  // Anything else (degenerate input, unsupported mode) collapses to a constant.
  if (segc.size() == 4)
  {
    out.setCoef(segc[0].y, segc[1].y, segc[2].y, segc[3].y);
    return;
  }

  if ((segc.size() & 3) == 0 && segc.size() / 4 >= 2)
  {
    // Multi-segment hermite spline. Match CubicCurveSampler::load's layout:
    //   type      = N segments
    //   coef[i]   = inner boundary X for i in [0, N-2]
    //               (the X positions of control points 1..N-1)
    //   splineCoef[4i + j] = polynomial coef j for segment i
    const int n = (int)segc.size() / 4;
    out.type = n;
    // Same allocator routing as the wipe above: prefer the configured allocator
    // when present so the matching free path picks it up.
    out.splineCoef =
      CubicCurveSampler::mem_allocator ? (float *)CubicCurveSampler::mem_allocator(sizeof(float) * 4 * n) : new float[4 * n];
    for (int i = 0; i + 1 < n; i++)
      out.coef[i] = v.curvePos[i + 1].x;
    for (int i = 0; i < (int)segc.size(); i++)
      out.splineCoef[i] = segc[i].y;
    return;
  }

  // Degenerate fallback: too few points for a real fit. Emit a constant equal
  // to the first y so the expression still has a finite value, and warn so the
  // artist notices.
  const float fallback = v.curvePos[0].y;
  out.setCoef(fallback, 0.f, 0.f, 0.f);
  DAEDITOR3.conWarning("typed-var curve '%s': degenerate control-point set, using constant %g", v.name.str(), fallback);
}

// True for any character that can appear inside a landClassEval identifier.
// Mirrors lcexpr_is_ident_suffix's allowed set: [A-Za-z0-9_].
static inline bool is_lc_ident_char(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Returns true if `name` appears in `haystack` as a STANDALONE identifier --
// preceded by start-of-string or a non-ident char and followed by end-of-string
// or a non-ident char. Avoids false positives like "myVar" matching "myVar2".
static bool name_appears_as_ident(const char *haystack, const char *name)
{
  if (!haystack || !name || !*name)
    return false;
  const size_t nameLen = strlen(name);
  for (const char *p = haystack;;)
  {
    const char *hit = strstr(p, name);
    if (!hit)
      return false;
    const bool leftOk = (hit == haystack) || !is_lc_ident_char(hit[-1]);
    const bool rightOk = !is_lc_ident_char(hit[nameLen]);
    if (leftOk && rightOk)
      return true;
    p = hit + 1;
  }
}

// True if `haystack` contains a `var <name>` or `let <name>` declaration as a
// standalone keyword followed (across whitespace) by `<name>` as a standalone
// identifier. lcexpr's parser rejects such declarations when the name is
// already in the varMap, so a typed var sharing this name would make the next
// recompile fail with "name is already defined". The Add dialog uses this to
// reject the name up front instead of mutating the panel and then surfacing
// the error during recompile. The lcexpr language has no comment syntax, so
// scanning the raw text without a tokenizer is sufficient.
//
// Index-based traversal with explicit bounds checks: every haystack[k] lookup
// (including the lookahead at i+3 and ident-boundary peeks at i+kwLen+nameLen)
// is gated on `k < hayLen` first. The trailing NUL itself is a valid byte to
// read but we never go beyond it.
static bool name_declared_as_var_or_let(const char *haystack, const char *name)
{
  if (!haystack || !name || !*name)
    return false;
  const size_t hayLen = strlen(haystack);
  const size_t nameLen = strlen(name);
  // "var" + 1 separator + nameLen is the minimum viable match, so anything
  // shorter cannot contain a declaration of `name`.
  if (hayLen < 3 + 1 + nameLen)
    return false;
  for (size_t i = 0; i + 3 <= hayLen;)
  {
    const bool isVar = haystack[i] == 'v' && haystack[i + 1] == 'a' && haystack[i + 2] == 'r';
    const bool isLet = haystack[i] == 'l' && haystack[i + 1] == 'e' && haystack[i + 2] == 't';
    if (!isVar && !isLet)
    {
      ++i;
      continue;
    }
    const bool leftOk = (i == 0) || !is_lc_ident_char(haystack[i - 1]);
    // i + 3 == hayLen lands on the NUL terminator, which is always present and
    // never an ident char. Beyond that we cannot have a separator + name.
    const bool rightOk = (i + 3 == hayLen) || !is_lc_ident_char(haystack[i + 3]);
    if (!leftOk || !rightOk)
    {
      ++i;
      continue;
    }
    size_t j = i + 3;
    while (j < hayLen && (haystack[j] == ' ' || haystack[j] == '\t' || haystack[j] == '\n' || haystack[j] == '\r'))
      ++j;
    if (j + nameLen <= hayLen && strncmp(haystack + j, name, nameLen) == 0 &&
        (j + nameLen == hayLen || !is_lc_ident_char(haystack[j + nameLen])))
      return true;
    i += 3;
  }
  return false;
}

// Register every TypedVar from the plugin's typedVars list into the shared parse
// table (vm/nv) and pre-populate the per-typedVar runtime binding (typedVarRuntime).
// Curve typedVars also append a CubicCurveSampler into evalCurves and store the
// slot index in runtime.curveIdx. Range typedVars register two slots: <name>_lo
// and <name>_hi. Mask typedVars register a single FLOAT slot whose per-pixel
// value is the per-pixel sample of the asset resolved via getScriptImage.
//
// Calls register_var_typed so the parser sees the right type at every reference.
// Mismatches between artist intent and registered slots (e.g. a Curve var that
// failed to register because of varId budget exhaustion) leave runtime.primaryVarId
// as 0xFFFFu; the bake loop then skips the per-pixel write for that entry.
static void register_typed_vars(const Tab<TypedVar> &typedVars, Tab<TypedVarRuntime> &runtime, Tab<CubicCurveSampler> &evalCurves,
  lcexpr::NameMap &vm, uint16_t &nv, lcexpr::VarTypeTable &vt)
{
  runtime.clear();
  runtime.resize(typedVars.size());
  evalCurves.clear();

  for (int i = 0; i < typedVars.size(); i++)
  {
    const TypedVar &v = typedVars[i];
    TypedVarRuntime &r = runtime[i];

    if (v.kind == TypedVarKind::Range)
    {
      // Two physical slots <name>_lo, <name>_hi; each is FLOAT-typed. Preflight
      // the budget so we never half-register: register_var_typed only rolls back
      // its own overflow, so registering _lo when exactly one slot remains lets
      // _lo consume that slot and then _hi overflow, leaving <name>_lo in the
      // varMap with no runtime binding. Both names are guaranteed new at this
      // point (Add rejected collisions with builtins / mask_<layer> / sibling
      // typed vars), so two fresh slots are needed.
      if (nv + 2 > lcexpr::MAX_VAR_ID)
      {
        DAEDITOR3.conError("typed-var '%s' (range): out of varId budget; skipping. Reduce typedVars count or layer mask names.",
          v.name.str());
        continue;
      }
      String loName(0, "%s_lo", v.name.str());
      String hiName(0, "%s_hi", v.name.str());
      r.primaryVarId = lcexpr::register_var_typed(vm, nv, vt, loName, lcexpr::TYPE_FLOAT);
      r.secondaryVarId = lcexpr::register_var_typed(vm, nv, vt, hiName, lcexpr::TYPE_FLOAT);
      G_ASSERT(r.primaryVarId != lcexpr::VAR_ID_OVERFLOW && r.secondaryVarId != lcexpr::VAR_ID_OVERFLOW);
      continue;
    }

    const lcexpr::ExprType t = (v.kind == TypedVarKind::Curve) ? hml_lcexpr_factory::TYPE_CURVE : lcexpr::TYPE_FLOAT;
    const uint16_t vid = lcexpr::register_var_typed(vm, nv, vt, v.name.str(), t);
    if (vid == lcexpr::VAR_ID_OVERFLOW)
    {
      DAEDITOR3.conError("typed-var '%s': out of varId budget; skipping.", v.name.str());
      continue;
    }
    r.primaryVarId = vid;

    if (v.kind == TypedVarKind::Curve)
    {
      r.curveIdx = (int)evalCurves.size();
      evalCurves.push_back();
      build_curve_sampler(evalCurves.back(), v);
    }
    else if (v.kind == TypedVarKind::Mask)
    {
      // Resolve the asset name to a script-image index now so the bake loop
      // can sample without per-pixel name lookups. -1 leaves the per-pixel
      // sample at 0 (logged once below) so a missing/typo'd asset doesn't
      // crash the bake.
      if (v.maskAsset.empty())
      {
        DAEDITOR3.conWarning("typed-var '%s' (mask): no asset name set; samples will read 0", v.name.str());
      }
      else
      {
        // getScriptImage(name, 1, -1) lazily allocates a slot for any non-empty
        // name and returns >= 0 even when the asset cannot be located on disk;
        // it does NOT signal "not found" via a negative return. The actual
        // resolution status is read back through getScriptImageBpp:
        //   bpp == 0 -> loadImage failed AND bitMaskImgMgr couldn't find props
        //               (asset name does not resolve to anything on disk)
        //   bpp 1 or 8 -> supported by the bake's sample dispatch
        //   other bpp -> image exists but uses a format the dispatch can't
        //                sample (sampleMask8UV's internal bpp guard returns
        //                a constant 1.0, silently making the mask always-on)
        // In every error case we reset maskImageIdx to -1 so the bake's
        // `tr.maskImageIdx >= 0` gate falls through to the 0.0 default,
        // matching the missing-asset defensive floor.
        r.maskImageIdx = HmapLandPlugin::self->getScriptImage(v.maskAsset.str(), 1, -1);
        r.maskImageBpp = (r.maskImageIdx >= 0) ? HmapLandPlugin::self->getScriptImageBpp(r.maskImageIdx) : 0;
        if (r.maskImageBpp == 0)
        {
          DAEDITOR3.conError("typed-var '%s' (mask): asset '%s' not found; samples will read 0", v.name.str(), v.maskAsset.str());
          r.maskImageIdx = -1;
        }
        else if (r.maskImageBpp != 1 && r.maskImageBpp != 8)
        {
          DAEDITOR3.conError("typed-var '%s' (mask): asset '%s' is %dbpp, only 1bpp and 8bpp masks are supported; "
                             "samples will read 0",
            v.name.str(), v.maskAsset.str(), r.maskImageBpp);
          r.maskImageIdx = -1;
          r.maskImageBpp = 0;
        }
      }
    }
  }
}

// Reserve a slot at VAR_COUNT + layerIdx for every gen layer (used for own-mask
// access in the bake loop) and register mask_<name> in the varMap for up to
// MAX_LAYER_VARS *enabled* layers whose paramName is a valid identifier. The
// budget is counted across enabled+nameable layers, NOT by layer index, so a
// run of disabled layers near the front of the list does not eat into the
// peer-visible name budget for enabled layers further down. Disabled layers
// and overflow-budget layers still get a slot but no peer-visible name (a
// disabled layer is inert, so peer/common expressions cannot reference its
// mask -- parse fails loudly instead of silently reading 0 at bake time, since
// the bake's pre-sampler also skips disabled layers). Caller must have already
// populated vm/nv via register_base_vars.
//
// vt parallels vm/nv: each successfully registered slot gets a TYPE_FLOAT entry
// so layer-mask reads return TYPE_FLOAT during type-check. Slots reserved without
// a peer-visible name (anonymous / overflow / disabled) are still padded to
// keep vt indices aligned with nv.
// Sum of varId slots typed-vars will consume next (Range = 2 for _lo / _hi,
// other kinds = 1). Used to subtract headroom from the layer-mask cap so a
// project with many layers cannot starve register_typed_vars of slots.
static int compute_typed_var_slot_budget(const Tab<TypedVar> &typedVars)
{
  int n = 0;
  for (int i = 0; i < typedVars.size(); i++)
    n += (typedVars[i].kind == TypedVarKind::Range) ? 2 : 1;
  return n;
}

static void register_layer_mask_names(const PtrTab<HmapLandPlugin::ScriptParam> &genLayers, lcexpr::NameMap &vm, uint16_t &nv,
  lcexpr::VarTypeTable &vt, int reserved_for_typed_vars)
{
  const int numLayers = (int)genLayers.size();
  int namedCount = 0;    // enabled + valid-ident layers we've registered so far
  int overflowCount = 0; // enabled + valid-ident layers we couldn't register due to MAX_LAYER_VARS cap

  // Hard cap on per-layer slot reservation: each layer occupies one varId at
  // VAR_COUNT + l. Once VAR_COUNT + l reaches MAX_VAR_ID we cannot allocate a
  // new slot without overflowing the parser's uint16_t varId space AND blowing
  // past the bake's exprVars (RelocatableFixedVector<float, MAX_VAR_ID> with
  // canOverflow=false: resize() silently clamps to MAX_VAR_ID, so per-pixel
  // writes at VAR_COUNT + l for the excess would land out of bounds and crash
  // the bake on a project with 250+ layers). Stop reserving past this point;
  // the bake also iterates only up to lcNumLayerVars which is set from this
  // same cap.
  //
  // reserved_for_typed_vars is subtracted from the cap so register_typed_vars,
  // which runs immediately after this, has guaranteed slot headroom even on a
  // project with enough layers to otherwise saturate MAX_VAR_ID. Without this
  // subtract, a high-layer-count project would silently lose every artist-
  // declared typed var (Add/UI accepts them; recompile drops them as overflow).
  const int rawMaxLayerSlots = (int)lcexpr::MAX_VAR_ID - LcExprContext::VAR_COUNT;
  const int maxLayerSlots = max(0, rawMaxLayerSlots - reserved_for_typed_vars);
  const int reservableLayers = (numLayers < maxLayerSlots) ? numLayers : maxLayerSlots;

  for (int l = 0; l < reservableLayers; l++)
  {
    const uint16_t slot = (uint16_t)(LcExprContext::VAR_COUNT + l);
    PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[l]);
    const char *nm = gl ? gl->paramName.str() : nullptr;
    const bool nameable = gl && gl->enabled && lcexpr_is_ident_suffix(nm);

    if (nameable && namedCount < LcExprContext::MAX_LAYER_VARS)
    {
      G_ASSERT(nv == slot);
      String fullName(0, "mask_%s", nm);
      const uint16_t before = nv;
      lcexpr::register_var(vm, nv, fullName);
      // If register_var didn't bump nv, the name was already in the map (duplicate
      // paramName among enabled layers, or a collision with a base var). The new
      // slot for layer l exists but is unreachable by name; warn so the user knows
      // why mask_<name> resolves to a different layer. Add/rename UI rejects new
      // duplicates, so this fires only on legacy / hand-edited BLK files.
      if (nv == before)
        DAEDITOR3.conWarning("layer[%d] '%s': duplicate name -- mask_%s aliases an earlier slot", l, nm, nm);
      ++namedCount;
    }
    else if (nameable)
    {
      // Hit the named-mask budget; this layer's slot exists but no peer can refer
      // to it by name.
      ++overflowCount;
    }
    // Always reserve slot VAR_COUNT+l so the bake loop can store every layer's
    // mask at exprVars[VAR_COUNT + l] uniformly, regardless of name registration.
    nv = slot + 1;
    if ((int)nv > vt.size())
    {
      int oldSize = vt.size();
      vt.resize(nv);
      for (int i = oldSize; i < (int)vt.size(); i++)
        vt[i] = lcexpr::TYPE_FLOAT;
    }
  }

  if (overflowCount > 0)
    logerr("too many enabled gen layers with named masks (cap %d, %d overflow): mask_<name> not registered for the excess "
           "(own-mask access still works)",
      LcExprContext::MAX_LAYER_VARS, overflowCount);
  if (numLayers > reservableLayers)
    logerr("layer count %d exceeds the lcexpr varId budget (%d, with %d slot(s) reserved for typed land variables). Layers beyond "
           "%d have no parser-visible mask binding and the bake skips their per-layer slot writes. Reduce layer count or typed-var "
           "count to silence this.",
      numLayers, maxLayerSlots, reserved_for_typed_vars, reservableLayers);
  // MAX_VAR_ID is the lcexpr lib's hard cap on varIds (uint16_t indexed); user-declared
  // var/let slots will start at nv, so leave headroom.
  if ((int)nv >= (int)lcexpr::MAX_VAR_ID)
    logerr("genLayers slot reservation (%d) reached varId cap %d: user var/let declarations will fail to parse", (int)nv,
      (int)lcexpr::MAX_VAR_ID);
}

// Compile the optional common expression into `arena` (run once per pixel before any
// layer's expression). Empty text leaves outValid=false (with empty outLastErr) so
// the bake skips it; non-empty text that fails to parse/compile sets outLastErr so
// the Apply path can surface the message in a dialog.
//
// vm/nv/vt are pass-by-reference: PARSE_EXPORT_TOP_LEVEL_VARS extends the shared
// symbol table with common's top-level var/let so subsequent layer parses see
// them. On any failure we save+restore so a broken common expression's partial
// mutations don't leak into later layer parses.
static void compile_common_into(lcexpr::Arena &arena, const char *text, uint32_t &outRoot,
  eastl::bitset<lcexpr::MAX_VAR_ID> &outVarMask, bool &outValid, SimpleString &outLastErr, lcexpr::NameMap &vm, uint16_t &nv,
  lcexpr::VarTypeTable &vt)
{
  outValid = false;
  outRoot = 0;
  outVarMask.reset();
  outLastErr = "";
  arena.clear();
  if (!text || !*text)
    return;
  lcexpr::NameMap savedVm(vm);
  const uint16_t savedNv = nv;
  const int savedVtSize = vt.size();
  eastl::string err;
  if (!compile_expression(arena, text, lcexpr::PARSE_EXPORT_TOP_LEVEL_VARS, vm, nv, vt, outRoot, outVarMask, err))
  {
    outLastErr = err.c_str();
    DAEDITOR3.conError("Common expression %s (expr='%s')", err.c_str(), text);
    vm = savedVm;
    nv = savedNv;
    vt.resize(savedVtSize);
    return;
  }
  outValid = true;
}

// Debug-only expression compile. Mirrors compile_common_into but uses PARSE_DEFAULT
// and takes vm/nv/vt by value: the debug expression is read-only -- its top-level
// var/let are local to its own eval (never visible to layers, never persisted),
// so the caller's copy is discarded after this returns and no save/restore is
// needed. vm/nv on entry are the post-common shared table (so the debug expr sees
// mask_<layer> and common's var/let just like a regular layer expr does).
static void compile_debug_into(lcexpr::Arena &arena, const char *text, uint32_t &outRoot,
  eastl::bitset<lcexpr::MAX_VAR_ID> &outVarMask, bool &outValid, SimpleString &outLastErr, uint16_t &outNv, lcexpr::NameMap vm,
  uint16_t nv, lcexpr::VarTypeTable vt)
{
  outValid = false;
  outRoot = 0;
  outVarMask.reset();
  outLastErr = "";
  outNv = nv;
  arena.clear();
  if (!text || !*text)
    return;
  eastl::string err;
  if (!compile_expression(arena, text, lcexpr::PARSE_DEFAULT, vm, nv, vt, outRoot, outVarMask, err))
  {
    outLastErr = err.c_str();
    DAEDITOR3.conError("Debug expression %s (expr='%s')", err.c_str(), text);
    return;
  }
  outNv = nv;
  outValid = true;
}

// Per-layer scope model:
//
//   visible to layers = base vars + mask_<enabled_name> + common's var/let.
//
// The walker builds that shared table once, then compiles every layer against a
// FRESH PRIVATE COPY of it. A layer's own var/let extends only its private copy
// and dies with the iteration -- peers see the same starting state regardless of
// what came before, and changing one layer's expression cannot break another.
// (To share a value across layers, declare it in the common expression.)
//
// Each layer stashes its post-parse nv in gl->exprNv: bytecode references varIds
// in [0, exprNv), and the bake loop zeroes [commonExprNv, exprNv) per pixel before
// evaluating each layer so an unassigned `var x` reads as 0 rather than a stale
// value from another layer.
//
// Final commonExprNv / lcMaxNv / lcNumLayerVars are stashed on the plugin for
// generateLandColors to size exprVars[] and pass the right bound to evalFinite.
//
// Required after any change that shifts varIds (add/move/del/rename/enable-toggle/
// Apply, plus the per-keystroke slider-drag path with shrink_arenas=false).
void HmapLandPlugin::recompileGenLayerExpressions(bool shrink_arenas)
{
  g_lcExpr.init();

  lcexpr::NameMap baseVm;
  uint16_t baseNv = 0;
  // Per-varId type table parallel to baseVm/baseNv. Built fresh on every recompile.
  // Stored on the plugin so the bake loop doesn't have to recompute it; the bake
  // doesn't actually consult the table (types are parse-time only) but downstream
  // lookups want the same shape.
  lcexpr::VarTypeTable baseVt;
  register_base_vars(baseVm, baseNv, baseVt);
  // Pre-reserve typed-var slot headroom so register_layer_mask_names cannot
  // saturate MAX_VAR_ID on a layer-heavy project before typed vars get a turn.
  const int typedVarSlotBudget = compute_typed_var_slot_budget(typedVars);
  register_layer_mask_names(genLayers, baseVm, baseNv, baseVt, typedVarSlotBudget);
  // Artist-defined typed variables come after layer masks so their slot indices
  // are stable across layers (each layer sees them as external vars). This also
  // rebuilds evalCurves and typedVarRuntime in lockstep with vm/nv/vt.
  register_typed_vars(typedVars, typedVarRuntime, evalCurves, baseVm, baseNv, baseVt);

  // Common expression extends the shared table with its var/let. From here on
  // (baseVm, baseNv, baseVt) is what every layer sees as its starting symbol table.
  compile_common_into(commonExprArena, commonExprText.str(), commonExprRoot, commonExprVarMask, commonExprValid, commonExprLastErr,
    baseVm, baseNv, baseVt);
  commonExprNv = baseNv;

  uint16_t maxNv = baseNv;
  for (int i = 0; i < genLayers.size(); i++)
  {
    PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[i]);
    if (!gl)
      continue;
    // Fresh private copy: any var/let this layer declares stays in layerVm/layerNv/
    // layerVt and is invisible to every peer (whether enabled or not).
    lcexpr::NameMap layerVm = baseVm;
    uint16_t layerNv = baseNv;
    lcexpr::VarTypeTable layerVt = baseVt;
    gl->compileExpr(layerVm, layerNv, layerVt, shrink_arenas);
    if (gl->enabled && gl->exprValid && gl->exprNv > maxNv)
      maxNv = gl->exprNv;
  }

  // Debug expression compiles against a fresh post-common copy of the symbol
  // table. Visible to it: base vars + mask_<layer> + common's var/let. Its
  // exprNv may exceed the per-pixel exprVars[] slot count if the debug
  // expression declares many local vars; track it in maxNv so the buffer is
  // sized accordingly even when no enabled gen layer needs as many slots.
  {
    lcexpr::NameMap debugVm = baseVm;
    uint16_t debugNv = baseNv;
    lcexpr::VarTypeTable debugVt = baseVt;
    compile_debug_into(debugExprArena, debugExprText.str(), debugExprRoot, debugExprVarMask, debugExprValid, debugExprLastErr,
      debugExprNv, debugVm, debugNv, debugVt);
    if (debugExprValid && debugExprNv > maxNv)
      maxNv = debugExprNv;
  }

  // Reserve MAX_TEMP_REGS slots beyond the widest named-var range for `{ block }`
  // scoped temp regs. Done unconditionally rather than tracking each expression's
  // actual peak: the buffer is per-pixel scratch and 16 floats (64 bytes) is below
  // the noise floor, so the simpler bookkeeping wins.
  lcMaxNv = maxNv + lcexpr::MAX_TEMP_REGS;
  // Capped to the same budget register_layer_mask_names enforces: only that
  // many slots actually get reserved for per-layer masks. The bake iterates
  // up to this count when filling per-layer mask slots and when running the
  // per-layer eval pass, so a project with more layers than the budget allows
  // simply leaves the excess inert (with a logerr from the registration path)
  // rather than overrunning exprVars. The typed-var slot budget is subtracted
  // here too so the bake never writes per-layer mask values into slots that
  // register_typed_vars has claimed for typed-var lo/hi/curve/value reads.
  lcNumLayerVars = min((int)genLayers.size(), max(0, (int)lcexpr::MAX_VAR_ID - LcExprContext::VAR_COUNT - typedVarSlotBudget));

  // Recompile invalidates any compiled bytecode the debug overlay refers to
  // (a layer's exprArena or debugExprArena), so refresh it whenever the user's
  // selected mode targets one. Gating on debugViewMode rather than the cached
  // showDebugWeightOverlay flag lets a transient !targetValid (e.g. a bad
  // debug-expression Apply) recover on the next successful recompile without
  // forcing the user to re-pick from the combobox. Reads lcNumLayerVars set
  // just above, so this must run last.
  if (debugViewMode == DV_DEBUG_EXPR || debugViewMode == DV_LAYER)
    updateDebugWeightTex();
}

// Common-expression accessors. The setter normalises by stripping `\r` and `\n` from
// the input so the BLK store remains a single-line value; UI display reflows via
// getCommonExprDisplayText() below.
void HmapLandPlugin::setCommonExprText(const char *txt)
{
  eastl::string normalized;
  if (txt)
    for (const char *p = txt; *p; p++)
      if (*p != '\n' && *p != '\r')
        normalized.push_back(*p);
  commonExprText = normalized.c_str();
  recompileGenLayerExpressions();
}

eastl::string HmapLandPlugin::getCommonExprDisplayText() const
{
  eastl::string out;
  for (const char *p = commonExprText.str(); p && *p; p++)
  {
    out.push_back(*p);
    if (*p == ';')
      out.push_back('\n');
  }
  return out;
}

// Debug-expression accessors. Same single-line normalisation as commonExprText:
// the BLK store is one line; UI display reflows by inserting `\n` after each `;`.
void HmapLandPlugin::setDebugExprText(const char *txt)
{
  eastl::string normalized;
  if (txt)
    for (const char *p = txt; *p; p++)
      if (*p != '\n' && *p != '\r')
        normalized.push_back(*p);
  debugExprText = normalized.c_str();
  recompileGenLayerExpressions();
}

eastl::string HmapLandPlugin::getDebugExprDisplayText() const
{
  eastl::string out;
  for (const char *p = debugExprText.str(); p && *p; p++)
  {
    out.push_back(*p);
    if (*p == ';')
      out.push_back('\n');
  }
  return out;
}

// Swap genLayers[lo] <-> genLayers[hi] (lo < hi) and update both layers' slotIdx.
// The caller is responsible for the post-step rebuildLandSlots() and
// recompileGenLayerExpressions() so a chain of swaps (e.g. addGenLayer's bubble)
// triggers them once at the end rather than per swap.
static void swap_gen_layers_unsafe(PtrTab<HmapLandPlugin::ScriptParam> &genLayers, int lo, int hi)
{
  Ptr<HmapLandPlugin::ScriptParam> tmp = genLayers[lo];
  genLayers[lo] = genLayers[hi];
  genLayers[hi] = tmp;
  static_cast<PostScriptParamLandLayer *>(genLayers[lo].get())
    ->swapSlotIdx(*static_cast<PostScriptParamLandLayer *>(genLayers[hi].get()));
}

bool HmapLandPlugin::loadGenLayers(const DataBlock &blk)
{
  int nid = blk.getNameId("layer");
  genLayers.clear();

  // Pass 1: load every layer and decide whether this .blk is in the legacy
  // format. New-format saves never emit lcIdx:i=, so any post-load
  // landIdx >= 0 on any layer means we are reading an older file that
  // still needs the donor-search migration.
  bool isLegacy = false;
  for (int i = 0, idx = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      PostScriptParamLandLayer *l = new PostScriptParamLandLayer(idx, 0, "");
      l->load(*blk.getBlock(i));
      if (l->landIdx >= 0)
        isLegacy = true;
      genLayers.push_back(l);
      idx++;
    }

  // Pass 2 (legacy-only): a layer that has lcIdx:i= but no lc1:t= (e.g. old
  // noobjects / importance slots that were only identified by index) adopts
  // the lc1 of any sibling layer sharing the same legacy lcIdx. This is
  // O(N^2) but N is small (<= ~20) and only runs on first open of an old
  // save; once the file is re-written in the new format (which never emits
  // lcIdx) isLegacy stays false and the whole pass is skipped.
  if (isLegacy)
    for (int i = 0; i < genLayers.size(); i++)
    {
      auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
      if (!gl->lc1.asset.empty() || gl->landIdx < 0)
        continue;
      for (int j = 0; j < genLayers.size(); j++)
      {
        if (i == j)
          continue;
        auto *donor = static_cast<PostScriptParamLandLayer *>(genLayers[j].get());
        if (donor->landIdx == gl->landIdx && !donor->lc1.asset.empty())
        {
          gl->lc1.asset = donor->lc1.asset;
          DAEDITOR3.conNote("loadGenLayers: layer[%d] adopted lc1=\"%s\" from layer[%d] via legacy lcIdx=%d", i,
            donor->lc1.asset.str(), j, gl->landIdx);
          break;
        }
      }
    }

  // Common expression: stored as a single line in the .blk; line breaks are not
  // part of the persisted form. We don't reflow here -- getCommonExprDisplayText()
  // does that on the way to the UI.
  commonExprText = blk.getStr("commonExpr", "");
  // Debug expression: same single-line format. Unlike commonExpr it is never
  // referenced from the bake, so a missing key just leaves the overlay empty.
  debugExprText = blk.getStr("debugExpr", "");

  // Pass 2.5: deduplicate paramNames. The add/rename UI rejects new duplicates,
  // but legacy BLK files (pre-mask_<name>) and hand-edited files can still feed
  // pairs like ("field", "field") into the rebuild. register_var() keeps only the
  // first binding, so without this pass mask_field would silently alias both
  // layers to the first slot. Auto-suffix later duplicates with _2/_3/... and log
  // a conError so the user notices the file was repaired on load.
  for (int i = 0; i < genLayers.size(); i++)
  {
    auto *gi = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    if (!gi || gi->paramName.empty())
      continue;
    bool collides = false;
    for (int j = 0; j < i && !collides; j++)
    {
      auto *gj = static_cast<PostScriptParamLandLayer *>(genLayers[j].get());
      if (gj && strcmp(gi->paramName.str(), gj->paramName.str()) == 0)
        collides = true;
    }
    if (!collides)
      continue;
    String candidate;
    for (int n = 2;; n++)
    {
      candidate.printf(0, "%s_%d", gi->paramName.str(), n);
      bool taken = false;
      for (int k = 0; k < genLayers.size() && !taken; k++)
      {
        if (k == i)
          continue;
        auto *gk = static_cast<PostScriptParamLandLayer *>(genLayers[k].get());
        if (gk && strcmp(gk->paramName.str(), candidate.str()) == 0)
          taken = true;
      }
      if (!taken)
        break;
    }
    DAEDITOR3.conError("loadGenLayers: layer[%d] paramName \"%s\" duplicates an earlier layer -- renamed to \"%s\". Re-save to "
                       "make this permanent.",
      i, gi->paramName.str(), candidate.str());
    gi->paramName = candidate.str();
  }

  // Pass 3 + 4: lex-sort unique lc1 / lc2 names (independently), assign each
  // layer its derived landIdx / landIdx2, then register every layer's asset
  // at its new slot pair.
  rebuildLandSlots();

  // All layers are now present; per-layer load() skipped its own compile, so do the
  // authoritative recompile here. Resolves any cross-layer mask_<name> / user-var refs.
  recompileGenLayerExpressions();
  return true;
}

void HmapLandPlugin::rebuildLandSlots()
{
  // Reentrancy note: called from PostScriptParamLandLayer::onPPBtnPressed UI
  // callbacks (lc1/lc2 picker), but none of register/unregister/reget below
  // synchronously re-enter that callback. setLandClass / getLandClassData
  // mutate LandClassSlotsManager / LandClassAssetMgr state only;
  // onLandClassAssetChanged notifications fire from asset-file-watcher paths
  // (onAssetChanged / onAssetRemoved), not from these register calls.
  // Even if they did fire, onLandClassAssetChanged reads lc1/lc2 assetData
  // but does not mutate lc1.asset / lc2.asset names, so the sort stream
  // below stays valid.
  // (name, owning-layer-index, which-slot) triple used by the union lex sort
  // below. `which == 0` means the name came from the layer's lc1 field;
  // `which == 1` means lc2. The dedup pass uses `which` to route the
  // assigned slot into one of two per-layer arrays so landIdx derivation
  // (prefer lc1, fall back to lc2) stays O(1) per layer.
  // name borrows into the owning layer's lc1.asset / lc2.asset SimpleString
  // buffers -- no lc1/lc2 mutation may happen between the entries.push_back
  // loop below and the end of the sort/scan.
  struct LcEntry
  {
    const char *name;
    int layerIdx;
    uint8_t which;
  };

  // Step 1: unregister every layer's current (possibly stale) slot so old
  // entries don't linger in the LandClassSlotsManager after the slots shift.
  for (int i = 0; i < genLayers.size(); i++)
  {
    auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    gl->unregisterAsset(gl->lc1);
    gl->unregisterAsset(gl->lc2);
  }

  // Step 2: gather (name, layerIdx, which) triples for every non-empty lc1
  // and lc2 across the layer set. lc1 and lc2 feed the SAME sort stream --
  // same union-of-names semantics as before -- but carrying the owning layer
  // index on every entry means the dedup pass below can back-propagate each
  // slot to its layer(s) without a second linear search.
  const int nLayers = genLayers.size();
  Tab<LcEntry> entries(tmpmem);
  entries.reserve(nLayers * 2);
  for (int i = 0; i < nLayers; i++)
  {
    auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    if (!gl->lc1.asset.empty())
      entries.push_back({gl->lc1.asset.str(), i, uint8_t(0)});
    if (!gl->lc2.asset.empty())
      entries.push_back({gl->lc2.asset.str(), i, uint8_t(1)});
  }

  // Step 3: lex-ascending sort via stlsort (introsort). O(N log N).
  stlsort::sort(entries.begin(), entries.end(), [](const LcEntry &a, const LcEntry &b) { return strcmp(a.name, b.name) < 0; });

  // Step 4: one pass over the sorted stream. Each unique name gets the next
  // ascending slot; the slot is written back into per-layer arrays keyed by
  // which == 0 (lc1) vs which == 1 (lc2). Total cost O(N log N) for the
  // sort + O(N) for this scan, replacing the previous per-layer linear
  // strcmp search over the full names list (O(N^2) overall).
  SmallTab<int, TmpmemAlloc> lc1Slot, lc2Slot;
  clear_and_resize(lc1Slot, nLayers);
  clear_and_resize(lc2Slot, nLayers);
  for (int i = 0; i < nLayers; i++)
  {
    lc1Slot[i] = -1;
    lc2Slot[i] = -1;
  }
  const char *prev = nullptr;
  int uniqN = 0;
  for (const LcEntry &e : entries)
  {
    if (!prev || strcmp(prev, e.name) != 0)
    {
      ++uniqN;
      prev = e.name;
    }
    (e.which == 0 ? lc1Slot : lc2Slot)[e.layerIdx] = uniqN - 1;
  }

  // Step 5: placeholder slots for nameless layers. A layer with neither lc1
  // nor lc2 (e.g. "noobjects") still writes a distinct marker to the land
  // bit-sublayer when writeLand1/writeLand2 is set, so it needs a unique
  // slot that does not collide with any named layer. We hand out the next
  // ascending slots after the lex-sorted ones -- this mirrors the legacy
  // find_value_idx(usedIdx, false) fallback that kept every layer's landIdx
  // distinct regardless of whether it had an asset.
  int lc1_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "land");
  int bits = lc1_li < 0 ? 0 : hmlService->getBitLayersList(getLayersHandle())[lc1_li].bitCount;
  // cap matches the validation in generateLandColors: landIdx < lc1_base.
  // Slot 0 is reserved (write value lc1_base - 0 = lc1_base is the "layer X"
  // marker for slot 0, write value 0 means "no layer here").
  int cap = (bits > 0) ? ((1 << bits) - 1) : 0;
  int totalSlots = uniqN;
  for (int i = 0; i < nLayers; i++)
    if (lc1Slot[i] < 0 && lc2Slot[i] < 0)
    {
      if (cap <= 0 || totalSlots >= cap)
        break;
      lc1Slot[i] = totalSlots++;
    }

  // Step 6: transitional slot cap. Until landClsMap is blockified, the
  // "land" bit-sublayer holds lc1_base slots and setLandClass wraps on
  // overflow. Log the overflow loudly and drop the tail assets.
  if (cap > 0 && totalSlots > cap)
  {
    DAEDITOR3.conError("genLayers: %d lc1/lc2/nameless slots exceed land bit-layer capacity (%d slots); extras dropped", totalSlots,
      cap);
    for (int i = 0; i < nLayers; i++)
    {
      if (lc1Slot[i] >= cap)
        lc1Slot[i] = -1;
      if (lc2Slot[i] >= cap)
        lc2Slot[i] = -1;
    }
  }

  // Step 7: derive each layer's landIdx. Preserve the original rule: lc1
  // wins if the layer has one, otherwise fall back to lc2's slot. That is
  // the same priority the previous linear search used when picking its key.
  for (int i = 0; i < nLayers; i++)
  {
    auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    gl->landIdx = lc1Slot[i] >= 0 ? lc1Slot[i] : lc2Slot[i];
  }

  // Step 7.5: loud-fail on lc2 asset collisions. Two layers that share lc1
  // (or more generally end up with the same landIdx) register their lc2
  // assets into the same "adds" sublayer slot (lc_base - landIdx); the
  // second registration overwrites the first. Currently no shipping level
  // has lc2 at all, so this is a guard for a future regression -- warn
  // instead of silently dropping.
  for (int i = 0; i < nLayers; i++)
  {
    auto *gi = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    if (gi->landIdx < 0 || gi->lc2.asset.empty())
      continue;
    for (int j = i + 1; j < nLayers; j++)
    {
      auto *gj = static_cast<PostScriptParamLandLayer *>(genLayers[j].get());
      if (gj->landIdx != gi->landIdx || gj->lc2.asset.empty())
        continue;
      if (strcmp(gi->lc2.asset, gj->lc2.asset) != 0)
        DAEDITOR3.conError("genLayers[%d] and [%d] share landIdx=%d but have distinct lc2 "
                           "(\"%s\" vs \"%s\"); the \"adds\" slot keeps only the last-registered asset. "
                           "Align lc2 or use a distinct lc1.",
          i, j, gi->landIdx, gi->lc2.asset.str(), gj->lc2.asset.str());
    }
  }

  // Step 8: register every layer at its fresh landIdx.
  for (int i = 0; i < nLayers; i++)
  {
    auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    gl->registerAssets();
  }
}
bool HmapLandPlugin::saveGenLayers(DataBlock &blk)
{
  if (!commonExprText.empty())
    blk.setStr("commonExpr", commonExprText);
  if (!debugExprText.empty())
    blk.setStr("debugExpr", debugExprText);
  for (int i = 0; i < genLayers.size(); i++)
    genLayers[i]->save(*blk.addNewBlock("layer"));
  return true;
}
bool HmapLandPlugin::isDetTexSlotWritten(int slot_idx) const
{
  // Mirror of the blendDetTex() predicate in generateLandColors (see the
  // "gl.writeDetTex && !gl.badDetTex" gate at the bottom of this file):
  // any layer that targets this slot and actually writes detTex weights
  // counts. registerAsset() publishes the slot name regardless of the
  // writeDetTex flag, so the exporter needs this filter to avoid flagging
  // a legitimate land-only layer as "final weights all 0".
  for (int i = 0; i < genLayers.size(); i++)
  {
    auto *gl = PostScriptParamLandLayer::cast(genLayers[i].get());
    if (gl && gl->detIdx == slot_idx && gl->writeDetTex && !gl->badDetTex)
      return true;
  }
  return false;
}
void HmapLandPlugin::prepareEditableLandClasses()
{
  for (int i = 0; i < genLayers.size(); i++)
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[i]))
      gl->prepareDrdHandle();
}
void HmapLandPlugin::addGenLayer(const char *name, int insert_before)
{
  // A brand-new layer has no lc1/lc2 yet; rebuildLandSlots assigns a
  // placeholder slot based on the layer's final position in genLayers. It (and
  // the lcexpr recompile) run once below, after any insert_before reorder.
  PostScriptParamLandLayer *l = new PostScriptParamLandLayer(genLayers.size(), -1, name);
  l->exprText = l->buildExprFromOldFormat().c_str();
  genLayers.push_back(l);
  // writeDetTex / writeLand1 now default true in the constructor (aligned with load()),
  // so no need to special-case the first layer here.

  // Bubble the new layer up to insert_before via the unsafe swap helper so we don't
  // pay rebuildLandSlots+recompile per swap. Both run once at the end of addGenLayer.
  if (insert_before != -1)
    for (int i = genLayers.size() - 1; i > insert_before; i--)
      swap_gen_layers_unsafe(genLayers, i - 1, i);
  rebuildLandSlots();
  recompileGenLayerExpressions();
}
void HmapLandPlugin::rebuildEvalCurveForRow(int row)
{
  if (row < 0 || row >= typedVars.size() || row >= typedVarRuntime.size())
    return;
  const TypedVar &v = typedVars[row];
  if (v.kind != TypedVarKind::Curve)
    return;
  const TypedVarRuntime &rt = typedVarRuntime[row];
  if (rt.curveIdx < 0 || rt.curveIdx >= evalCurves.size())
    return;
  build_curve_sampler(evalCurves[rt.curveIdx], v);
}

// Scan commonExprText and every layer's exprText for `var <name>` / `let <name>`
// declarations matching `name`. On a hit, fill `outSite` with a human-readable
// origin and return true. Disabled / non-useExpr layers are scanned too: their
// exprText is still on disk and would resurface the collision the next time the
// user toggles the layer on. The Add dialog uses this so a typed var that would
// collide with a text-declared var/let is rejected before the panel mutates.
bool HmapLandPlugin::findVarLetDeclSite(const char *name, SimpleString &outSite) const
{
  outSite = "";
  if (!name || !*name)
    return false;
  if (name_declared_as_var_or_let(commonExprText.str(), name))
  {
    outSite = "common expression";
    return true;
  }
  for (int i = 0; i < genLayers.size(); i++)
  {
    auto *gl = PostScriptParamLandLayer::cast(genLayers[i].get());
    if (!gl)
      continue;
    if (name_declared_as_var_or_let(gl->exprText.str(), name))
    {
      String s(0, "layer '%s' expression", gl->paramName.str());
      outSite = s.str();
      return true;
    }
  }
  return false;
}

bool HmapLandPlugin::isReservedVarName(const char *name) const
{
  if (!name || !*name)
    return false;
  // Six fixed builtins -- registered first by register_base_vars and reserved
  // for the bake's per-pixel writes. Aliasing any of them with a typed var
  // would silently shadow the runtime value.
  static const char *kBuiltins[] = {"height", "angle", "curvature", "mask", "world_x", "world_y"};
  for (const char *b : kBuiltins)
    if (strcmp(name, b) == 0)
      return true;
  // Each non-empty, identifier-valid layer paramName claims mask_<paramName>.
  // We don't filter by `enabled` -- a disabled layer keeps its slot reservation
  // and toggling it on later would resurface the collision.
  for (int i = 0; i < genLayers.size(); i++)
  {
    auto *gl = PostScriptParamLandLayer::cast(genLayers[i].get());
    if (!gl || gl->paramName.empty() || !lcexpr_is_ident_suffix(gl->paramName.str()))
      continue;
    String full(0, "mask_%s", gl->paramName.str());
    if (strcmp(name, full.str()) == 0)
      return true;
  }
  return false;
}

// True if `probe` matches any current typed var's parser-visible name. For a
// non-Range entry that's its bare name; for a Range entry it's <name>_lo /
// <name>_hi (the bare Range name itself does not enter the varMap). On a hit
// `outConflict` is set to the offending typed-var's display name. The typed-
// var Add dialog uses this to catch asymmetric Range collisions; the layer
// add/rename path uses it (with probe = "mask_<layerName>") so a layer name
// that would alias a typed-var slot is rejected up front.
bool HmapLandPlugin::collidesWithTypedVar(const char *probe, SimpleString &outConflict) const
{
  outConflict = "";
  if (!probe || !*probe)
    return false;
  for (int i = 0; i < typedVars.size(); i++)
  {
    const TypedVar &v = typedVars[i];
    if (v.kind == TypedVarKind::Range)
    {
      String loName(0, "%s_lo", v.name.str());
      String hiName(0, "%s_hi", v.name.str());
      if (strcmp(probe, loName.str()) == 0 || strcmp(probe, hiName.str()) == 0)
      {
        outConflict = v.name.str();
        return true;
      }
    }
    else if (strcmp(probe, v.name.str()) == 0)
    {
      outConflict = v.name.str();
      return true;
    }
  }
  return false;
}

bool HmapLandPlugin::scanTypedVarRefs(const TypedVar &v, Tab<String> &outSites) const
{
  outSites.clear();
  // Range exposes two parser-visible names; every other kind exposes one. Build
  // the list once and reuse it across the layer / common scan.
  Tab<String> namesToScan(tmpmem);
  if (v.kind == TypedVarKind::Range)
  {
    namesToScan.push_back(String(0, "%s_lo", v.name.str()));
    namesToScan.push_back(String(0, "%s_hi", v.name.str()));
  }
  else
  {
    namesToScan.push_back(String(v.name.str()));
  }

  for (int n = 0; n < namesToScan.size(); n++)
  {
    const char *needle = namesToScan[n].str();
    if (name_appears_as_ident(commonExprText.str(), needle))
      outSites.push_back(String(0, "common expression (uses '%s')", needle));
    for (int i = 0; i < genLayers.size(); i++)
    {
      auto *gl = PostScriptParamLandLayer::cast(genLayers[i].get());
      if (!gl)
        continue;
      // Scan even disabled / non-useExpr layers: their exprText is still on
      // disk and a delete here would silently break the next time the user
      // toggles them on. Better to surface the reference now.
      if (name_appears_as_ident(gl->exprText.str(), needle))
        outSites.push_back(String(0, "layer '%s' (uses '%s')", gl->paramName.str(), needle));
    }
  }
  return outSites.size() > 0;
}
bool HmapLandPlugin::isGenLayerNameInUse(const char *name, const ScriptParam *exclude) const
{
  // Empty / null names don't enter the varMap (mask_<name> needs a valid identifier
  // suffix), so multiple unnamed layers can coexist without colliding -- treat them
  // as not-in-use here so we don't block legitimate placeholder layers.
  if (!name || !*name)
    return false;
  for (int i = 0; i < genLayers.size(); i++)
  {
    const ScriptParam *gl = genLayers[i].get();
    if (!gl || gl == exclude)
      continue;
    if (strcmp(gl->paramName.str(), name) == 0)
      return true;
  }
  return false;
}
bool HmapLandPlugin::isValidGenLayerName(const char *name)
{
  // Empty / null names are allowed: an unnamed layer simply has no peer-visible
  // mask_<name> binding (multiple unnamed placeholder layers can coexist). For
  // non-empty names we require a valid identifier suffix so the derived
  // mask_<name> binding parses as one identifier; otherwise the layer's mask
  // would be silently unreferenceable from any expression.
  if (!name || !*name)
    return true;
  return lcexpr_is_ident_suffix(name);
}
void HmapLandPlugin::snapshotGenLayerValidity(Tab<bool> &out) const
{
  out.resize(genLayers.size());
  for (int i = 0; i < genLayers.size(); i++)
  {
    auto *gl = PostScriptParamLandLayer::cast(genLayers[i]);
    // Null entries shouldn't happen in practice; treat as "valid" so a missing layer
    // doesn't masquerade as a regression. Same for entries the cast can't recognise.
    out[i] = gl ? gl->exprValid : true;
  }
}
int HmapLandPlugin::findFirstRegressedGenLayer(const Tab<bool> &wasValid, SimpleString &outName, SimpleString &outErr) const
{
  const int n = (int)genLayers.size();
  const int snapN = (int)wasValid.size();
  for (int i = 0; i < n && i < snapN; i++)
  {
    auto *gl = PostScriptParamLandLayer::cast(genLayers[i]);
    if (!gl || !wasValid[i] || gl->exprValid)
      continue;
    outName = gl->paramName;
    outErr = gl->lastErr;
    return i;
  }
  return -1;
}
bool HmapLandPlugin::moveGenLayer(ScriptParam *gl, bool up)
{
  for (int i = 0; i < genLayers.size(); i++)
    if (genLayers[i] == gl)
    {
      if (up && i > 0)
      {
        swap_gen_layers_unsafe(genLayers, i - 1, i);
        // Placeholder landIdx for unnamed layers is assigned in genLayers order, so
        // reordering shifts those slots. Named layers are stable under reorder.
        rebuildLandSlots();
        recompileGenLayerExpressions();
        return true;
      }
      else if (!up && i + 1 < genLayers.size())
      {
        swap_gen_layers_unsafe(genLayers, i, i + 1);
        rebuildLandSlots();
        recompileGenLayerExpressions();
        return true;
      }
      break;
    }
  return false;
}
bool HmapLandPlugin::delGenLayer(ScriptParam *gl)
{
  for (int i = 0; i < genLayers.size(); i++)
    if (genLayers[i] == gl)
    {
      erase_items(genLayers, i, 1);
      for (; i < genLayers.size(); i++)
        static_cast<PostScriptParamLandLayer *>(genLayers[i].get())->changeSlotIdx(i);
      // Dropping a layer may shrink the unique lc1/lc2 set and shift lex slots.
      // The debug overlay (if it targeted the deleted layer) is auto-cleared
      // by the recompile-tail updateDebugWeightTex via the name-lookup miss.
      rebuildLandSlots();
      recompileGenLayerExpressions();
      return true;
    }
  return false;
}
void HmapLandPlugin::regenLayerTex()
{
  for (int i = 0; i < genLayers.size(); i++)
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[i]))
      HmapLandPlugin::hmlService->updateDetailRenderData(gl->drdHandle, gl->elcLayers);
}

void HmapLandPlugin::storeLayerTex()
{
  if (d3d::is_stub_driver() || !hmlService)
    return;
  String prefix(DAGORED2->getPluginFilePath(this, "elc"));

  for (int i = 0; i < genLayers.size(); i++)
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[i]))
    {
      HmapLandPlugin::hmlService->updateDetailRenderData(gl->drdHandle, gl->elcLayers);
      HmapLandPlugin::hmlService->storeDetailRenderData(gl->drdHandle, prefix, true, true);
    }
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

// LandColorGenData out-of-line method bodies live in hmlGenColorMap_lcg.cpp.

void ScriptParamImage::calcMapping(const LandColorGenData &lcg, real x, real z, Point2 &p) const
{
  if (mappingType == MAPPING_HMAP_PERCENT)
  {
    p.x = x / lcg.heightMapSizeX;
    p.y = z / lcg.heightMapSizeZ;

    p -= offset / 100;

    if (float_nonzero(tile.x))
      p.x = p.x * 100 / tile.x;
    if (float_nonzero(tile.y))
      p.y = p.y * 100 / tile.y;
  }
  else if (mappingType == MAPPING_WORLD)
  {
    p.x = x;
    p.y = z;

    p *= lcg.gridCellSize;

    p -= offset;

    if (float_nonzero(tile.x))
      p.x = p.x / tile.x;
    if (float_nonzero(tile.y))
      p.y = p.y / tile.y;
  }
  else if (mappingType == MAPPING_VERT_U || mappingType == MAPPING_VERT_V)
  {
    p.x = p.y = 0;

    if (mappingType == MAPPING_VERT_U)
      p.x = lcg.height;
    else
      p.y = lcg.height;

    p -= offset;

    if (float_nonzero(tile.x))
      p.x = p.x / tile.x;
    if (float_nonzero(tile.y))
      p.y = p.y / tile.y;
  }
  else
  {
    p.x = x;
    p.y = z;

    p -= offset;

    if (float_nonzero(tile.x))
      p.x = p.x * 100 / tile.x;
    if (float_nonzero(tile.y))
      p.y = p.y * 100 / tile.y;

    p.x /= HmapLandPlugin::self->getScriptImageWidth(imageIndex);
    p.y /= HmapLandPlugin::self->getScriptImageHeight(imageIndex);
  }

  flipAndSwapUV(p.x, p.y);
}


E3DCOLOR ScriptParamImage::sampleImage(const LandColorGenData &lcg) const
{
  Point2 p;
  calcMapping(lcg, lcg.heightMapX, lcg.heightMapZ, p);

  E3DCOLOR c = HmapLandPlugin::self->sampleScriptImageUV(imageIndex, p.x, p.y, clampU, clampV);
  c.a = detailType;
  return c;
}

float ScriptParamImage::sampleImage1(const LandColorGenData &lcg) const
{
  Point2 p;
  calcMapping(lcg, lcg.heightMapX, lcg.heightMapZ, p);

  return HmapLandPlugin::self->sampleMask1UV(imageIndex, p.x, p.y, clampU, clampV);
}

float ScriptParamImage::sampleImage8(const LandColorGenData &lcg) const
{
  Point2 p;
  calcMapping(lcg, lcg.heightMapX, lcg.heightMapZ, p);

  return HmapLandPlugin::self->sampleMask8UV(imageIndex, p.x, p.y, clampU, clampV);
}

void ScriptParamImage::setImage(const LandColorGenData &lcg, E3DCOLOR c) const
{
  Point2 p;
  calcMapping(lcg, lcg.heightMapX, lcg.heightMapZ, p);

  HmapLandPlugin::self->paintScriptImageUV(imageIndex, p.x, p.y, p.x, p.y, clampU, clampV, c);
}

bool ScriptParamImage::saveImage() { return HmapLandPlugin::self->saveImage(imageIndex); }


E3DCOLOR ScriptParamImage::sampleImagePixel(const LandColorGenData &lcg) const
{
  E3DCOLOR c = sampleImagePixelTrueAlpha(lcg);
  c.a = detailType;
  return c;
}


E3DCOLOR ScriptParamImage::sampleImagePixelTrueAlpha(const LandColorGenData &lcg) const
{
  Point2 p;
  calcMapping(lcg, lcg.heightMapX, lcg.heightMapZ, p);

  E3DCOLOR c = HmapLandPlugin::self->sampleScriptImagePixelUV(imageIndex, p.x, p.y, clampU, clampV);
  return c;
}


void ScriptParamImage::paintImage(const LandColorGenData &lcg, E3DCOLOR color) const
{
  Point2 p0, p1;
  calcMapping(lcg, lcg.heightMapX, lcg.heightMapZ, p0);
  calcMapping(lcg, lcg.heightMapX + 1.0f, lcg.heightMapZ + 1.0f, p1);

  HmapLandPlugin::self->paintScriptImageUV(imageIndex, p0.x, p0.y, p1.x, p1.y, clampU, clampV, color);
}

float ScriptParamMask::sampleMask1(const LandColorGenData &lcg) const
{
  float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask1UV(imageIndex, fx, fy);
}
float ScriptParamMask::sampleMask8(const LandColorGenData &lcg) const
{
  float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask8UV(imageIndex, fx, fy);
}
float ScriptParamMask::sampleMask1Pixel(const LandColorGenData &lcg) const
{
  float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask1PixelUV(imageIndex, fx, fy);
}
float ScriptParamMask::sampleMask8Pixel(const LandColorGenData &lcg) const
{
  float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask8PixelUV(imageIndex, fx, fy);
}
void ScriptParamMask::setMask1(const LandColorGenData &lcg, bool c) const
{
  float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
  return HmapLandPlugin::self->paintMask1UV(imageIndex, fx, fy, c);
}
void ScriptParamMask::setMask8(const LandColorGenData &lcg, char c) const
{
  float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
  return HmapLandPlugin::self->paintMask8UV(imageIndex, fx, fy, c);
}


inline float PostScriptParamLandLayer::getMaskDirect(float fx, float fy) const
{
  if (imageBpp == 1)
    return HmapLandPlugin::self->sampleMask1UV(imageIndex, fx, fy);
  else if (imageBpp == 8)
    return HmapLandPlugin::self->sampleMask8UV(imageIndex, fx, fy);
  return 1;
}
inline void PostScriptParamLandLayer::setMaskDirect(float fx, float fy, int c)
{
  if (imageBpp == 1)
    HmapLandPlugin::self->paintMask1UV(imageIndex, fx, fy, c);
  else if (imageBpp == 8)
    HmapLandPlugin::self->paintMask8UV(imageIndex, fx, fy, c);
}
inline float PostScriptParamLandLayer::getMaskDirectEx(float fx, float fy) const
{
  int idx = elcMaskCurIdx < 0 ? imageIndex : elcMaskImageIndex;
  switch (elcMaskCurIdx < 0 ? imageBpp : elcMaskImageBpp)
  {
    case 1: return HmapLandPlugin::self->sampleMask1UV(idx, fx, fy);
    case 8: return HmapLandPlugin::self->sampleMask8UV(idx, fx, fy);
  }
  return 1;
}
inline void PostScriptParamLandLayer::setMaskDirectEx(float fx, float fy, int c)
{
  int idx = elcMaskCurIdx < 0 ? imageIndex : elcMaskImageIndex;
  switch (elcMaskCurIdx < 0 ? imageBpp : elcMaskImageBpp)
  {
    case 1: HmapLandPlugin::self->paintMask1UV(idx, fx, fy, c); break;
    case 8: HmapLandPlugin::self->paintMask8UV(idx, fx, fy, c); break;
  }
}
inline float PostScriptParamLandLayer::getMask(float fx, float fy) const
{
  if (detRectMappedMask)
    if (!HmapLandPlugin::self->mapGlobalTCtoDetRectTC(fx, fy))
      return 0;
  return getMaskDirect(fx, fy);
}
inline void PostScriptParamLandLayer::setMask(float fx, float fy, int c)
{
  if (detRectMappedMask)
    if (!HmapLandPlugin::self->mapGlobalTCtoDetRectTC(fx, fy))
      return;
  return setMaskDirect(fx, fy, c);
}

inline float PostScriptParamLandLayer::calcWeight(const LandColorGenData &lcg, float height, float angDiff, float curvature) const
{
  float wt[] = {1, 1, 1, 1};

  float *w = &wt[0];
  if (maskConv == WtRange::WMT_zero)
    *w = 0;
  else if (maskConv != WtRange::WMT_one)
  {
    float fx = lcg.heightMapX / lcg.heightMapSizeX, fy = lcg.heightMapZ / lcg.heightMapSizeZ;
    *w = getMask(fx, fy);
    if (maskConv == WtRange::WMT_smoothStep)
      *w = LandColorGenData::smoothStep(*w);
  }

  w++;
  const float htAngCurv[3] = {height, angDiff, curvature};
  G_STATIC_ASSERT(countof(htAngCurv) == countof(ht_ang_curv));
  for (int i = 0; i < countof(ht_ang_curv); ++i, ++w)
  {
    auto val = htAngCurv[i];
    auto fun = ht_ang_curv[i];
    if (fun.conv == WtRange::WMT_zero)
      *w = 0;
    else if (fun.conv != WtRange::WMT_one)
    {
      if (fabsf(fun.dv) < 1e-4)
        *w = (fabsf(val - fun.v0) < 1e-4) ? 1.0f : 0.0f;
      else
        *w = clamp((val - fun.v0) / fun.dv, 0.0f, 1.0f);
      if (fun.conv == WtRange::WMT_smoothStep)
        *w = LandColorGenData::smoothStep(*w);
    }
  }

  if (wtMode == 2) // max(mask, terrain product)
    return max(wt[0], wt[1] * wt[2] * wt[3]);
  return wtMode == 1 ? (wt[0] + wt[1] + wt[2] + wt[3]) : (wt[0] * wt[1] * wt[2] * wt[3]);
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

// Per-pixel exprVars setup shared by generateLandColors and updateDebugWeightTex.
// Caller has already populated builtins (VAR_HEIGHT/ANGLE/CURVATURE/MASK/
// WORLD_X/WORLD_Y) at expr_vars_ptr. This helper:
//
//   1. Samples each layer's mask into expr_vars_ptr[VAR_COUNT + l] (only for
//      layers flagged in layer_mask_needed; others get 0).
//   2. Zeroes common's user-var range starting at typed_vars_end_nv so
//      Float / Range / Curve typedVar slots written outside the per-pixel
//      loop survive the clear.
//   3. Samples the Mask typedVars that prepare_typed_vars flagged as read.
//   4. Runs the common expression once at this pixel.
//
// Caller proceeds with per-layer / target eval after this returns. The
// helper stays in this TU because PostScriptParamLandLayer::cast / getMask
// are file-scoped here.
void HmapLandPlugin::fillPerPixelExprVarsAndEvalCommon(float *expr_vars_ptr, float fx, float fy, int layer_slots,
  dag::ConstSpan<uint8_t> layer_mask_needed, uint16_t typed_vars_end_nv, dag::ConstSpan<int> used_mask_tvs)
{
  for (int l = 0; l < layer_slots; l++)
  {
    float v = 0.f;
    if (l < (int)layer_mask_needed.size() && layer_mask_needed[l])
      if (auto *glm = PostScriptParamLandLayer::cast(genLayers[l]))
        v = glm->getMask(fx, fy);
    expr_vars_ptr[LcExprContext::VAR_COUNT + l] = v;
  }

  if (commonExprValid)
    for (int u = (int)typed_vars_end_nv; u < (int)commonExprNv; u++)
      expr_vars_ptr[u] = 0.f;

  for (int idx : used_mask_tvs)
  {
    const TypedVarRuntime &tr = typedVarRuntime[idx];
    expr_vars_ptr[tr.primaryVarId] =
      (tr.maskImageBpp == 1) ? sampleMask1UV(tr.maskImageIdx, fx, fy) : sampleMask8UV(tr.maskImageIdx, fx, fy);
  }

  if (commonExprValid)
    (void)lcexpr::evalFinite(commonExprArena, commonExprRoot, expr_vars_ptr, (int)commonExprNv, &evalCurves);
}

void HmapLandPlugin::generateLandColors(const IBBox2 *in_rect, bool finished, bool may_rebuild_lmesh_if_needed)
{
  if (!heightMap.isFileOpened())
    return;

  // Clear readiness up front so an early return below (bad landIdx/detIdx on
  // any genLayer, missing heightmap, etc.) leaves landClsMapGenerated=false.
  // The flag is otherwise sticky across calls: without this reset a later
  // validation failure would inherit the previous pass's success and let
  // buildAndWrite() export stale derived maps. Set to true at the end of the
  // pass once we know we ran to completion.
  landClsMapGenerated = false;

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  updateScriptImageList();

  if (!in_rect)
  {
    if (hasColorTex)
      colorMap.reset(heightMap.getMapSizeX(), heightMap.getMapSizeY(), E3DCOLOR(255, 10, 255, 0));
    createColormapFile(con);
    resizeLandClassMapFile(DAGORED2->getConsole());
  }

  // Ensure detTexMap exists AND matches the current heightmap extent before
  // any applyBlendDetTex writes. prepareDetTexMaps is idempotent when the
  // size already matches, and on resize it calls reset() to rebuild the
  // block grid. The per-pixel applyBlendDetTex path used to create the map
  // only when the pointer was null, so a heightmap resize mid-session would
  // leave the grid stuck at the old extent and setAt() silently dropped
  // writes outside it.
  prepareDetTexMaps();

  // Full-pass rebuild: wipe the per-block slot tables so dead zero-weight
  // slots do not accumulate across regen passes (setAt only appends slots;
  // nothing prunes them mid-session). The sub-rectangle path intentionally
  // skips this: untouched blocks outside the dirty rect must keep their
  // existing detail-tex data, since the loop below only overwrites pixels
  // inside the rect.
  if (!in_rect && detTexMap)
    detTexMap->reset(heightMap.getMapSizeX(), heightMap.getMapSizeY());

  int time0 = ::get_time_msec();

  // Compile-time toggle to disable threading for debugging or A/B-comparing
  // single-threaded against the parallel path. When false, quant is set to
  // span the whole [yStart, yEnd) so parallel_for_inline dispatches a single
  // chunk on the calling thread (thread_id 0) and the per-thread state slots
  // 1..n collapse to unused.
  static constexpr bool parallel_build = true;

  // Predict the worker count without actually initting the pool. The init is
  // deferred to immediately around parallel_for_inline below and paired with
  // a shutdown -- same lazy-init pattern as the export path in
  // hmlIGenEditorPlugin.cpp:2235-2290 -- so the early-return error paths
  // following this block don't leave the pool up for the rest of the editor
  // session. If the pool is already initialised by an outer caller we adopt
  // its worker count; otherwise we use the value we'll pass to init() below
  // (capped at 4 to match the export site).
  int workers_count;
  if (!parallel_build)
    workers_count = 1;
  else if (threadpool::get_num_workers() > 0)
    workers_count = threadpool::get_num_workers();
  else
    workers_count = max<int>(1, min(cpujobs::get_physical_core_count(), 4));

  if (!in_rect)
  {
    con.startProgress();
    con.setActionDesc("generating land colors (%d workers)...", workers_count);
    // Only thread_id == 0 (the calling thread + any worker that happens to
    // get re-mapped to id 0, which is none -- workers get [1, n]) ticks
    // incDone in the lambda, so it sees roughly 1/n_threads of the rows.
    // Dividing the total by workers_count makes the bar reach ~100% when the
    // pass finishes; setDone snaps it to exactly 100% afterwards.
    con.setTotal(max<int>(1, landClsMap.getMapSizeY() / lcmScale / workers_count));
  }

  Color3 grassColor(0.5f, 0.5f, 0.1f);
  Color3 dirtColor(0.6f, 0.4f, 0.3f);

  // Per-pass scratch + setup state. Built once and replicated below into the
  // per-thread `lcg[]` Tab indexed by parallel_for_inline's thread_id; the
  // setup fields (gridCellSize, hmap pointers, curvWt, etc.) become read-only
  // for the duration of the parallel section.
  LandColorGenData lcgInit;
  lcgInit.gridCellSize = gridCellSize;
  lcgInit.heightMapSizeX = heightMap.getMapSizeX();
  lcgInit.heightMapSizeZ = heightMap.getMapSizeY();
  lcgInit.heightMapOffset = heightMapOffset;
  lcgInit.hmap = &heightMap;
  lcgInit.hmapDet = &heightMapDet;
  lcgInit.detDiv = detDivisor;
  lcgInit.detRectC = detRectC;
  lcgInit.initCurvatureFilter();

  int lc1_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "land");
  int lc2_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "adds");
  int lc1_base = lc1_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[lc1_li].bitCount) - 1;
  int lc2_base = lc2_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[lc2_li].bitCount) - 1;
  int phys_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "phys");
  int phys_base = phys_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[phys_li].bitCount) - 1;
  int imp_scale = impLayerIdx < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[impLayerIdx].bitCount) - 1;
  int det_val_max = detLayerIdx < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[detLayerIdx].bitCount) - 1;

  for (int l = 0; l < genLayers.size(); l++)
    if (PostScriptParamLandLayer *gl = static_cast<PostScriptParamLandLayer *>(genLayers[l].get()))
    {
      if (gl->writeDetTex && !gl->badDetTex)
        if (gl->detIdx < 0 || gl->detIdx >= det_val_max)
        {
          DAEDITOR3.conError("Bad detIdx=%d in layer %d \"%s\",  maxDet=%d", gl->detIdx, l, gl->paramName, det_val_max);
          return;
        }
      if (gl->writeLand1 && lc1_base)
        if (gl->landIdx < 0 || gl->landIdx >= lc1_base)
        {
          DAEDITOR3.conError("Bad landIdx=%d in layer %d \"%s\",  maxLand[1]=%d", gl->landIdx, l, gl->paramName, lc1_base);
          return;
        }
      if (gl->writeLand2 && lc2_base)
        if (gl->landIdx < 0 || gl->landIdx >= lc2_base)
        {
          DAEDITOR3.conError("Bad landIdx=%d in layer %d \"%s\",  maxLand[2]=%d", gl->landIdx, l, gl->paramName, lc2_base);
          return;
        }
      if (phys_base)
        if (gl->landIdx < 0 || gl->landIdx >= phys_base)
        {
          DAEDITOR3.conError("Bad landIdx=%d in layer %d \"%s\",  max_phys=%d", gl->landIdx, l, gl->paramName, phys_base);
          return;
        }
    }

  // OR every enabled layer's varMask plus the common expression's. The common expr runs
  // once per pixel before any layer, so its references gate the same per-pixel setup
  // (curvature / mask_<name> sampling) as a layer's would.
  eastl::bitset<lcexpr::MAX_VAR_ID> anyVarMask;
  if (commonExprValid)
    anyVarMask = commonExprVarMask;
  for (int l = 0; l < genLayers.size(); l++)
  {
    auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[l].get());
    if (gl->enabled)
      anyVarMask |= gl->exprVarMask;
  }
  bool layers_use_curv = anyVarMask.test(LcExprContext::VAR_CURVATURE);

  // Per-layer mask pre-sampling. exprVars[VAR_COUNT + l] holds layer l's mask for the
  // current pixel and is (1) the source for the own-layer `mask` variable (copied into
  // VAR_MASK at the start of each layer's eval) and (2) the value exposed as
  // mask_<layer_name> for the first MAX_LAYER_VARS layers.
  //
  // layerMaskNeeded[l] = "sample layer l's mask this pixel". A layer needs sampling if
  // its own expression reads `mask`, or any peer (necessarily within MAX_LAYER_VARS)
  // reads its mask_<name>. Unsampled layers get exprVars[VAR_COUNT + l] = 0. Plain
  // per-layer flag so own-mask tracking stays correct for any number of gen layers.
  const int numLayers = (int)genLayers.size();
  dag::RelocatableFixedVector<uint8_t, LcExprContext::MAX_LAYER_VARS> layerMaskNeeded;
  layerMaskNeeded.resize(numLayers);
  memset(layerMaskNeeded.data(), 0, numLayers);
  for (int l = 0; l < numLayers; l++)
  {
    auto *gl = PostScriptParamLandLayer::cast(genLayers[l]);
    if (!gl || !gl->enabled)
      continue;
    if (gl->exprVarMask.test(LcExprContext::VAR_MASK))
      layerMaskNeeded[l] = 1;
    if (l < LcExprContext::MAX_LAYER_VARS && anyVarMask.test(LcExprContext::VAR_COUNT + l))
      layerMaskNeeded[l] = 1;
  }
  // Per-pixel exprVars layout (rebuilt per pixel inside the worker):
  //   [0, VAR_COUNT) builtins; [VAR_COUNT, VAR_COUNT + numLayers) per-layer mask
  //   slots; [VAR_COUNT + numLayers, commonExprNv) common's user var/let slots;
  //   per-layer private user-var slots overlap in [commonExprNv, gl->exprNv).
  // Each layer gets the same starting offset because per-layer scope means
  // layer slots don't coexist. Size = max(VAR_COUNT + numLayers, lcMaxNv) covers
  // both the per-layer mask range and the widest user-var range any single
  // layer needs. exprVars is fixed-capacity at MAX_VAR_ID with canOverflow=
  // false; per-layer slots only exist up to lcNumLayerVars (capped to
  // MAX_VAR_ID - VAR_COUNT by recompileGenLayerExpressions), so use that
  // instead of raw numLayers when sizing.
  const int exprVarsSize = max((int)lcMaxNv, LcExprContext::VAR_COUNT + lcNumLayerVars);

  int x0 = 0, x1 = heightMap.getMapSizeX();
  int y0 = 0, y1 = heightMap.getMapSizeY();
  if (in_rect)
  {
    x0 = in_rect->lim[0].x;
    x1 = in_rect->lim[1].x;
    y0 = in_rect->lim[0].y + 1;
    y1 = in_rect->lim[1].y + 1;
  }

  // Block-aligned chunking for parallel_for_inline:
  //
  //   - applyBlendDetTex writes through detTexMap->setAt, which mutates the
  //     shared 64x64 DetTexBlock at (x>>6, y>>6). Two threads writing pixels
  //     in the same block race on blockDetTexIdx + weights. Picking quant =
  //     DETTEX_BLOCK_W (64) and aligning the begin row to a 64 boundary
  //     guarantees every chunk [tbegin, tend) maps to ONE block-row only, so
  //     cross-thread block collisions are impossible.
  //
  //   - landClsMap.setData is per-cell; chunks write disjoint y-rows so
  //     pixels never alias.
  //
  //   - The partial-regen path (in_rect) hands an arbitrary [y0, y1). We
  //     widen it out to the surrounding 64-row block bounds [yStart, yEnd)
  //     for parallel_for_inline so the chunk dispatch keeps its block-aligned
  //     invariant; the inner loop then clamps each chunk back to [y0, y1)
  //     via max(tbegin, y0)/min(tend, y1) so we don't process rows outside
  //     the requested rect.
  //
  // Per-thread state, indexed by parallel_for_inline's thread_id. Workers
  // are dispatched with worker_id in [1, add_jobs_count] (max add_jobs_count
  // is get_num_workers() after clamping), and the calling thread reuses 0;
  // sizing to n_workers + 1 covers every thread_id the dispatcher can hand
  // out. Each LCG slot is seeded from lcgInit so the const setup fields
  // (curvWt, gridCellSize, hmap pointers, ...) are populated; lcActiveMask
  // and blendTex start at their default-constructed empty/zero state.
  //
  // Pre-allocating per thread (instead of inside the lambda) avoids
  // re-allocation on every chunk: parallel_for_inline calls the lambda once
  // per quant-sized chunk (~64 rows), not once per thread, so a 4kx4k regen
  // would pay the LandColorGenData copy + Tab<int>::resize +
  // RelocatableFixedVector::resize 64 times per pass.
  const uint32_t n_threads = (uint32_t)(workers_count + 1);

  Tab<LandColorGenData> lcg;
  lcg.resize(n_threads);
  for (uint32_t t = 0; t < n_threads; t++)
    lcg[t] = lcgInit;

  // Flat per-thread layerVal buffer: thread_id-th slice is at
  // [thread_id * layerValStride, (thread_id + 1) * layerValStride). The
  // per-pixel mem_set_0 zero-fills before each pixel, so initial contents
  // don't matter.
  const int layerValStride = (int)landClsLayer.size();
  Tab<int> threadLayerVal;
  threadLayerVal.resize(n_threads * max<int>(1, layerValStride));

  // Flat per-thread exprVars buffer, same indexing scheme. Zero-initialised
  // because the per-pixel loop now only touches slots that actually change
  // per pixel (builtins / layer-mask slots / used-Mask typed-vars / common's
  // user-var range / each layer's private user-var range). Slots that hold
  // constants for the whole pass -- Float / Range / Curve typed-vars, plus
  // any Mask typed-var no expression reads -- are written once below per
  // thread and stay alive across pixels.
  Tab<float> threadExprVars;
  threadExprVars.resize(n_threads * exprVarsSize);
  memset(threadExprVars.data(), 0, threadExprVars.size() * sizeof(float));

  // Typed-var setup hoisted out of the per-pixel loop. Float / Range / Curve
  // are constant per bake; prepare_typed_vars writes them into thread 0's
  // slice and we fan out to the rest. Mask typedVars change per pixel but only
  // those filtered into usedMaskTVs (anyVarMask gates which ones any expression
  // we evaluate actually reads). typedVarsEndNv lifts the per-pixel zero of
  // common's user-var range past the constant slots.
  uint16_t typedVarsEndNv = uint16_t(LcExprContext::VAR_COUNT + lcNumLayerVars);
  dag::RelocatableFixedVector<int, 8> usedMaskTVs;
  prepare_typed_vars(typedVars, typedVarRuntime, anyVarMask, threadExprVars.data(), typedVarsEndNv, usedMaskTVs);
  for (uint32_t t = 1; t < n_threads; t++)
    memcpy(threadExprVars.data() + t * exprVarsSize, threadExprVars.data(), exprVarsSize * sizeof(float));

  // generate landclass map and colors
  const int yStart = max<int>(0, y0 & ~(hmap_storage::DETTEX_BLOCK_W - 1));
  const int yEnd = min<int>(heightMap.getMapSizeY(), (y1 + hmap_storage::DETTEX_BLOCK_W - 1) & ~(hmap_storage::DETTEX_BLOCK_W - 1));
  const uint32_t quant = parallel_build ? uint32_t(hmap_storage::DETTEX_BLOCK_W) : uint32_t((yEnd - yStart) + 1);

  // Lazy threadpool init: same paired init/shutdown pattern the export path
  // in hmlIGenEditorPlugin.cpp:2235-2290 uses. We deliberately do NOT leave
  // the pool up across calls -- otherwise the next caller (export, navmesh
  // rebuild, ...) would see a pre-initialised pool with our queue/stack
  // sizing and skip its own init() with the sizing it actually wants. Caps
  // at 4 workers to match that call site. Skipped entirely on the
  // parallel_build=false debug path -- there parallel_for_inline runs the
  // single chunk synchronously on the calling thread and never touches the
  // pool, so initing it would be wasted work.
  const bool need_init_threadpool = parallel_build && threadpool::get_num_workers() == 0;
  if (need_init_threadpool)
    threadpool::init(min(cpujobs::get_physical_core_count(), 4), 1024, 256 << 10);

  threadpool::parallel_for_inline(yStart, yEnd, quant, [&](uint32_t tbegin, uint32_t tend, uint32_t thread_id) {
    // Bind per-thread state. No allocation here; the slots were sized once
    // before parallel_for_inline so chunks dispatched to the same thread_id
    // re-use the same scratch (and its lcActiveMask accumulates across them
    // until reduced into the merged mask below).
    LandColorGenData &tlcg = lcg[thread_id];
    dag::Span<int> layerVal(threadLayerVal.data() + thread_id * layerValStride, layerValStride);
    float *const exprVarsPtr = threadExprVars.data() + thread_id * exprVarsSize;

    for (uint32_t yU = max<int>(tbegin, y0), yE = min<int>(tend, y1); yU < yE; ++yU)
    {
      const int y = (int)yU;
      tlcg.curv_dy = y - LandColorGenData::FC;

      for (int x = x0; x < x1; ++x)
      {
        real h = tlcg.sampleHeight(x, y);

        real hu = (tlcg.sampleHeight(x + 1, y) - tlcg.sampleHeight(x - 1, y)) * 0.5f;
        real hv = (tlcg.sampleHeight(x, y + 1) - tlcg.sampleHeight(x, y - 1)) * 0.5f;

        real d = gridCellSize;

        Point3 normal(-hu, d, -hv);

        real len = sqrtf(d * d + hu * hu + hv * hv);
        normal /= len;

        tlcg.normal = normal;
        float angDiff = acos(normal.y) * 180 / PI;
        tlcg.height = h;

        tlcg.curv_dx = x - LandColorGenData::FC;
        tlcg.curvatureReady = false;

        for (int ly = 0; ly < lcmScale; ly++)
          for (int lx = 0; lx < lcmScale; lx++)
          {
            tlcg.heightMapX = x + float(lx) / lcmScale;
            tlcg.heightMapZ = y + float(ly) / lcmScale;

            tlcg.resetBlendDetTex();
            mem_set_0(layerVal);
            int detIdx = 0;

            float curv = layers_use_curv ? tlcg.getCurvature() : 0;
            bool in_det = detDivisor ? insideDetRectC(x * detDivisor, y * detDivisor) : false;

            // exprVars layout (per pixel): see comment block above the
            // parallel_for_inline call for the full layout description.
            // Builtins and mask slots are written once per pixel; common's
            // range is zeroed before the common eval; each layer's private
            // range is zeroed before its eval so an unassigned `var x` reads
            // 0 rather than a stale value left by another layer in the same
            // pixel.
            exprVarsPtr[LcExprContext::VAR_HEIGHT] = h;
            exprVarsPtr[LcExprContext::VAR_ANGLE] = angDiff;
            exprVarsPtr[LcExprContext::VAR_CURVATURE] = curv;
            exprVarsPtr[LcExprContext::VAR_MASK] = 0;
            exprVarsPtr[LcExprContext::VAR_WORLD_X] = tlcg.heightMapX * tlcg.gridCellSize + tlcg.heightMapOffset.x;
            exprVarsPtr[LcExprContext::VAR_WORLD_Y] = tlcg.heightMapZ * tlcg.gridCellSize + tlcg.heightMapOffset.y;

            // Per-layer mask sample / common-user-var zero / used-Mask
            // typedVar sample / common-expr eval. layerSlots is capped at
            // lcNumLayerVars (== reservable budget) so a project with 250+
            // layers does not write past exprVars's MAX_VAR_ID-clamped end.
            // The cleared span inside the helper starts at typedVarsEndNv,
            // skipping Float / Range / Curve typedVar slots so those
            // constants stay alive across pixels.
            const float fx = tlcg.heightMapX / tlcg.heightMapSizeX;
            const float fy = tlcg.heightMapZ / tlcg.heightMapSizeZ;
            fillPerPixelExprVarsAndEvalCommon(exprVarsPtr, fx, fy, min(numLayers, lcNumLayerVars), make_span_const(layerMaskNeeded),
              typedVarsEndNv, make_span_const(usedMaskTVs));

            // --- Per-layer eval pass ---
            // Capped at lcNumLayerVars: layers beyond the reserved budget
            // have no exprVars slot at VAR_COUNT + l, so the own-mask alias
            // below would read past exprVars's MAX_VAR_ID-clamped end. Same
            // cap as the per-layer slot fill above and as
            // register_layer_mask_names.
            const int evalLayerCount = min((int)genLayers.size(), lcNumLayerVars);
            for (int l = 0; l < evalLayerCount; l++)
            {
              const PostScriptParamLandLayer &gl = *static_cast<const PostScriptParamLandLayer *>(genLayers[l].get());
              if (!gl.enabled)
                continue;
              if (gl.areaSelect == gl.AREA_main && in_det)
                continue;
              if (gl.areaSelect == gl.AREA_det && !in_det)
                continue;

              // Own-layer `mask` access: alias the layer's pre-sampled slot.
              // Always valid because register_layer_mask_names reserves
              // VAR_COUNT+l for every layer (including those beyond
              // MAX_LAYER_VARS, which simply have no peer-visible name).
              exprVarsPtr[LcExprContext::VAR_MASK] = exprVarsPtr[LcExprContext::VAR_COUNT + l];

              // Zero this layer's private user-var range so an unassigned
              // `var x` reads 0 rather than a stale value left by an earlier
              // layer's eval at this pixel (per-layer scope: each layer's
              // user vars are local to its bytecode).
              for (int u = (int)commonExprNv; u < (int)gl.exprNv; u++)
                exprVarsPtr[u] = 0.f;

              // Raw weight (no upper clamp). Sum-mode expressions
              // legitimately produce wt > 1; we preserve that for the
              // importance channel so it matches the pre-expression
              // calcWeight path. Saturation happens below only for the
              // [0,1]-expected consumers (landclass gate thresholds, detTex
              // blend).
              double wt = gl.exprValid ? lcexpr::evalFinite(gl.exprArena, gl.exprRoot, exprVarsPtr, (int)gl.exprNv, &evalCurves)
                                       : (l == 0 ? 1.0 : 0.0);
              double wtSat = wt > 1.0 ? 1.0 : wt;

// Regression tripwire: validate that the expression path agrees with the
// legacy data-driven calcWeight. Gated off by default; build with
// -DLCEXPR_COMPARE_WITH_OLD=1 (or just flip to 1 here during local testing)
// to turn it on. Skipped for useExpr layers because the user-authored
// expression has no legacy equivalent to compare with.
#ifndef LCEXPR_COMPARE_WITH_OLD
#define LCEXPR_COMPARE_WITH_OLD 0
#endif
#if LCEXPR_COMPARE_WITH_OLD
              if (!gl.useExpr)
              {
                double wtOld = gl.calcWeight(tlcg, h, angDiff, curv);
                // Both wtOld and wt are raw (unclamped). Sum-mode layers can
                // exceed 1 on both sides, so comparing raw-vs-raw is the
                // correct invariant.
                if (fabs(wtOld - wt) > 0.01)
                  DAEDITOR3.conError("EXPR/OLD MISMATCH at (%d,%d) layer %d '%s': expr=%.4f old=%.4f expr='%s'", x, y, l,
                    gl.paramName.str(), wt, wtOld, gl.exprText.str());
              }
#endif

              if (!lx && !ly && gl.writeDetTex && !gl.badDetTex && wtSat > gl.writeDetTexThres)
                tlcg.blendDetTex(gl.detIdx, wtSat);
              if (gl.writeLand1 && wtSat > gl.writeLand1Thres && lc1_li >= 0)
                layerVal[lc1_li] = lc1_base - gl.landIdx;
              if (gl.writeLand2 && wtSat > gl.writeLand2Thres && lc2_li >= 0)
                layerVal[lc2_li] = lc2_base - gl.landIdx;
              if (gl.writeImportance && impLayerIdx >= 0)
                layerVal[impLayerIdx] = wt * imp_scale;
              if (gl.writeDetTex && wtSat > gl.writeDetTexThres && phys_li >= 0)
                layerVal[phys_li] = phys_base - gl.landIdx;
            }

            if (!lx && !ly)
              tlcg.applyBlendDetTex(x, y, detIdx, getDetTexMap());

            unsigned w = 0;
            if (detLayerIdx >= 0)
              w = landClsLayer[detLayerIdx].setLayerData(w, (getDetTexMap() && tlcg.blendTex.size()) ? tlcg.blendTex[0].idx : detIdx);

            for (int l = 0; l < landClsLayer.size(); l++)
              if (l != detLayerIdx)
                w = landClsLayer[l].setLayerData(w, layerVal[l]);

            landClsMap.setData(x * lcmScale + lx, y * lcmScale + ly, w);
          }
      }

      // Tick the progress bar from the calling thread only -- CoolConsole
      // isn't safe to update off the main thread, and parallel_for_inline
      // runs the calling thread under thread_id 0 while workers get ids
      // [1, n_workers]. setTotal above was sized to mapSizeY/lcmScale/workers_count
      // so a thread that handles ~1/n_threads of the rows brings the bar to
      // ~100%; setDone snaps it to exactly 100% after the pass.
      if (thread_id == 0 && !in_rect)
        con.incDone();
    }
  });
  if (need_init_threadpool)
    threadpool::shutdown();

  // Reduce per-thread lcActiveMask into a single merged mask for the
  // post-pass classifier. Each chunk dispatched to thread_id t accumulated
  // bits into lcg[t].lcActiveMask via applyBlendDetTex; multiple chunks on
  // the same thread share the slot so OR-merge is the identity reduction.
  uint64_t lcActiveMask = 0;
  for (uint32_t t = 0; t < n_threads; t++)
    lcActiveMask |= lcg[t].lcActiveMask;

  // Snap to 100% so the bar matches the just-finished work regardless of
  // how many rows actually fell to thread_id 0 (the divisor is approximate
  // -- a thread that lost some chunks to other workers would otherwise
  // leave the bar shy of full). Must mirror the divisor used in setTotal.
  // The in_rect path doesn't touch progress at all, same as before.
  if (!in_rect)
    con.setDone(max<int>(1, landClsMap.getMapSizeY() / lcmScale / workers_count));

  int samples_processed = (y1 - y0) * (x1 - x0) * lcmScale * lcmScale;

  // Mark derived maps populated *before* the optional rebuild below: by here
  // we have run to completion, and rebuildLandmeshPhysMap -> buildAndWritePhysMap
  // re-enters generateLandColors with may_rebuild_lmesh_if_needed=false and
  // gates that inner call on landClsMapGenerated. Without flipping it now, the
  // entry-time landClsMapGenerated=false (which makes early returns leave the
  // flag cleared) would let the inner call run a second full regen.
  landClsMapGenerated = true;

  // Single-pass classify: lcActiveMask now records which slots emitted >=1/255
  // anywhere. Errors and warnings derive from comparing it against the
  // statically-known configured-slot set (detailTexBlkName non-empty), so
  // we no longer need to recurse to validate against a freshly built lmDump.
  // Trigger a one-shot lmDump/landmesh rebuild only when an active configured
  // slot is missing from the current lcRemap (lmDump genuinely stale because
  // a layer was added/removed since the last rebuild).
  // Always re-derive lmDump (and friends) for full-regen calls. lmDump is an
  // in-memory buffer; the cost is bounded (one detTexMap scan + mesh dump
  // serialize, well under a second) and dominated by generateLandColors
  // itself, which we're already paying. Doing it unconditionally drops the
  // stale-lcRemap detection problem entirely: there's nothing to detect
  // because lcRemap and lmDump are always rebuilt to match the just-written
  // detTexMap. Callers that already do their own external rebuild after
  // generateLandColors (notably beforeMainLoop, which re-triangulates
  // landMeshMap before its rebuild) opt out via
  // may_rebuild_lmesh_if_needed=false.
  if (may_rebuild_lmesh_if_needed && !in_rect)
  {
    rebuildLandmeshDump();
    rebuildLandmeshManager();
    delayedResetRenderer();
    hmlService->invalidateClipmap(true);
    pendingLandmeshRebuild = false;
    // rebuildLandmeshPhysMap -> buildAndWritePhysMap calls generateLandColors
    // with may_rebuild_lmesh_if_needed=false; the landClsMapGenerated=true set
    // above makes that inner call short-circuit, so it can't re-enter here.
    rebuildLandmeshPhysMap();
  }

  if (landMeshManager)
  {
    // Error: a slot got non-zero quantized weight but has no configured
    // landclass asset (no detail texture) -> the runtime can't render it.
    for (uint64_t bw = lcActiveMask; bw; bw = __blsr(bw))
    {
      unsigned i = __ctz_unsafe(bw);
      bool configured = (i < detailTexBlkName.size() && !detailTexBlkName[i].empty());
      if (!configured)
      {
        if (i < detailTexBlkName.size())
          con.addMessage(ILogWriter::ERROR, "land class blended without detail texture: <slot %u>", i);
        else
          con.addMessage(ILogWriter::ERROR, "land class blended without detail texture: <unknown idx >=%u>", i);
      }
    }
    // Warning (full regen only): a slot has a configured asset but every
    // pixel quantized to 0, so it contributes nothing to the export and is
    // dead BLK. Bit 63 is the >=63 catch-all so warnings for slot 63+ may
    // false-negative when several high-idx slots collide on it; not an
    // issue for any current level (slot count well under 64).
    if (!in_rect)
    {
      const int n = detailTexBlkName.size();
      for (int i = 0; i < n; ++i)
      {
        if (detailTexBlkName[i].empty())
          continue;
        unsigned bit = i < 63 ? unsigned(i) : 63u;
        if (!(lcActiveMask & (1ULL << bit)))
          con.addMessage(ILogWriter::WARNING, "land class <%s> never reaches export threshold (quantized weight 0 everywhere)",
            detailTexBlkName[i].str());
      }
    }
  }

  if (!in_rect)
  {
    heightMap.unloadAllUnchangedData();
    // landClsMap / colorMap are RAM-only (commit cb65d22e4f9b); skip the
    // flushData calls that used to write them to disk.
    landClsMap.unloadAllUnchangedData();
    if (hasColorTex)
      colorMap.unloadAllUnchangedData();
    con.endProgress();
  }

  int genTime = ::get_time_msec() - time0;
  con.addMessage(ILogWriter::NOTE, "generated in %d msec (%d samples)", genTime, samples_processed);

  con.endLog();
  if (in_rect)
    onLandRegionChanged((*in_rect)[0].x * lcmScale, (*in_rect)[0].y * lcmScale, (*in_rect)[1].x * lcmScale + 1,
      (*in_rect)[1].y * lcmScale + 1, finished);
  else
    onLandRegionChanged(0, 0, landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), finished);

  if (showLandClassColors)
    updateLandClassColorsTex();
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


HmapLandPlugin::ScriptParam *HmapLandPlugin::getEditedScriptImage() { return editedScriptImage; }

IHmapBrushImage::Channel HmapLandPlugin::getEditedChannel() const
{
  if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage))
    return IHmapBrushImage::CHANNEL_RGB;
  ScriptParamImage *edImage = (ScriptParamImage *)editedScriptImage.get();
  return edImage && edImage->bitsPerPixel == 32 ? edImage->channel : IHmapBrushImage::CHANNEL_RGB;
}


bool HmapLandPlugin::showEditedMask() const
{
  // Always show the brush surface while a layer mask is being edited: the
  // editor needs to see what it is painting, regardless of which Debug View
  // mode the panel combobox is on. 32bpp script images keep their per-image
  // showMask toggle (RGB / mask channel selector). Pre-feature this branch
  // returned showBlueWhiteMask, which silently dropped the brush whenever the
  // user switched to DV_DEBUG_EXPR / DV_LAYER -- now decoupled.
  if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage))
    return true;
  ScriptParamImage *edImage = (ScriptParamImage *)editedScriptImage.get();
  return edImage ? (edImage->bitsPerPixel == 32 ? edImage->showMask : true) : true;
}


void HmapLandPlugin::setShowBlueWhiteMask()
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  static int worldToBwMaskVarId = dagGeom->getShaderVariableId("world_to_bw_mask");
  bool show = false;
  Color4 world_to_bw_mask;
  // Mask edit takes precedence: when a mask is being painted the user needs to see
  // the brush surface, not the colorize overlay. Colorize is the fallback when no
  // mask is active. showEditedMask() is the single predicate -- it already encodes
  // "edit mode + this image's overlay should be visible" without leaning on
  // showBlueWhiteMask, so DV_DEBUG_EXPR / DV_LAYER cannot suppress the brush here.
  if (editedScriptImage && showEditedMask())
  {
    show = true;
    world_to_bw_mask.r = 1.f / (esiGridW * esiGridStep);
    world_to_bw_mask.g = 1.f / (esiGridH * esiGridStep);
    world_to_bw_mask.b = (-esiOrigin.x + 0.5) * world_to_bw_mask.r;
    world_to_bw_mask.a = (-esiOrigin.y + 0.5) * world_to_bw_mask.g;
  }
  else if ((showLandClassColors || showDebugWeightOverlay) && bluewhiteTex && detTexMap)
  {
    // Debug overlays (single-layer / debug-expression) share the colored-landclass
    // texture binding path: same texture (bluewhiteTex sized to detTexMap dims),
    // same world->UV mapping, just different texel contents.
    show = true;
    const int W = detTexMap->getMapSizeX();
    const int H = detTexMap->getMapSizeY();
    const float worldW = float(W) * gridCellSize;
    const float worldH = float(H) * gridCellSize;
    world_to_bw_mask.r = 1.f / worldW;
    world_to_bw_mask.g = 1.f / worldH;
    // +0.5 cell shifts the sample to texel centres, matching the mask path
    world_to_bw_mask.b = (-heightMapOffset.x + 0.5f * gridCellSize) * world_to_bw_mask.r;
    world_to_bw_mask.a = (-heightMapOffset.y + 0.5f * gridCellSize) * world_to_bw_mask.g;
  }
  if (show)
    dagGeom->shaderGlobalSetColor4(worldToBwMaskVarId, world_to_bw_mask);
  dagGeom->shaderGlobalSetTexture(blueWhiteMaskVarId, show ? bluewhiteTexId : BAD_TEXTUREID);
}

void HmapLandPlugin::updateGenerationMask(const IBBox2 *rect)
{
  PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage);
  if (!gl || editedScriptImageIdx < 0)
    return;

  const char *mask_nm = gl->elcMaskList.getName(editedScriptImageIdx);
  int img_idx = gl->elcMaskImageIndex;
  int img_bpp = gl->elcMaskImageBpp;
  if (img_bpp != 1 && img_bpp != 8)
  {
    DAEDITOR3.conWarning("strange mask texture <%s> bpp=%d", mask_nm, img_bpp);
    return;
  }

  for (int i = 0; i < genLayers.size(); i++)
  {
    gl = PostScriptParamLandLayer::cast(genLayers[i]);
    Texture *tex = mask_nm ? HmapLandPlugin::hmlService->getDetailRenderDataMaskTex(gl->drdHandle, mask_nm) : NULL;
    if (!tex)
      continue;

    TextureInfo ti;
    uint8_t *imgPtr;
    int stride;
    int x0, y0, x1, y1;

    if (!tex->getinfo(ti))
    {
      DAEDITOR3.conWarning("failed to get mask texture sizes <%s>", mask_nm);
      continue;
    }

    if (rect)
    {
      x0 = clamp(int((rect->lim[0].x * gridCellSize + heightMapOffset.x - esiOrigin.x) / esiGridStep * ti.w / esiGridW), 0, ti.w - 1);
      y1 = ti.h - 1 -
           clamp(int((rect->lim[0].y * gridCellSize + heightMapOffset.y - esiOrigin.y) / esiGridStep * ti.h / esiGridH), 0, ti.h - 1);
      x1 = clamp(int((rect->lim[1].x * gridCellSize + heightMapOffset.x - esiOrigin.x - 1) / esiGridStep * ti.w / esiGridW + 1), 0,
        ti.w - 1);
      y0 = ti.h - 1 -
           clamp(int((rect->lim[1].y * gridCellSize + heightMapOffset.y - esiOrigin.y - 1) / esiGridStep * ti.h / esiGridH + 1), 0,
             ti.h - 1);
    }
    else
      x0 = y0 = 0, x1 = ti.w - 1, y1 = ti.h - 1;

    // fill mask texture
    if (tex->lockimg((void **)&imgPtr, stride, 0, TEXLOCK_READWRITE))
    {
      imgPtr += stride * y0 + x0;
      int use_stride = stride - (x1 - x0 + 1);

      switch (img_bpp)
      {
        case 1:
          for (int y = y0; y <= y1; ++y, imgPtr += use_stride)
            for (int x = x0; x <= x1; ++x, ++imgPtr)
              *imgPtr = real2uchar(HmapLandPlugin::self->sampleMask1UV(img_idx, x / float(ti.w), 1.0f - y / float(ti.h)));
          break;
        case 8:
          for (int y = y0; y <= y1; ++y, imgPtr += use_stride)
            for (int x = x0; x <= x1; ++x, ++imgPtr)
              *imgPtr = real2uchar(HmapLandPlugin::self->sampleMask8UV(img_idx, x / float(ti.w), 1.0f - y / float(ti.h)));
          break;
      }
      tex->unlockimg();
      HmapLandPlugin::hmlService->updateDetailRenderData(gl->drdHandle, gl->elcLayers);
    }
  }
}

// Fill bluewhiteTex with the per-pixel weighted blend of layer colors taken from
// detTexMap. Sized to detTexMap dims; bound via the same blue_white_mask_tex
// shader var as the mask overlay.
void HmapLandPlugin::updateLandClassColorsTex()
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  if (!detTexMap)
    return;
  const int W = detTexMap->getMapSizeX();
  const int H = detTexMap->getMapSizeY();
  if (W <= 0 || H <= 0)
    return;

  if (bluewhiteTex)
  {
    TextureInfo ti;
    bluewhiteTex->getinfo(ti, 0);
    if (ti.w != W || ti.h != H || (ti.cflg & TEXFMT_MASK) != TEXFMT_A8R8G8B8 || !(ti.cflg & TEXCF_SRGBREAD))
    {
      dagGeom->shaderGlobalSetTexture(blueWhiteMaskVarId, BAD_TEXTUREID);
      dagRender->releaseManagedTexVerified(bluewhiteTexId, bluewhiteTex);
    }
  }
  if (!bluewhiteTex)
  {
    // SRGBREAD: layer colors are picked in sRGB; init_albedo wants linear, so let the
    // sampler do sRGB->linear conversion. Without this the colors look washed out.
    bluewhiteTex = d3d::create_tex(NULL, W, H, TEXFMT_A8R8G8B8 | TEXCF_DYNAMIC | TEXCF_SRGBREAD, 1, "blueWhite");
    G_ASSERT(bluewhiteTex);
    bluewhiteTexId = dagRender->registerManagedTex("bluewhiteTex", bluewhiteTex);
  }

  // Build a per-detail-slot color table from layer colors. Multiple layers may
  // share a slot (lex-sorted lc1 collapse); the last-written wins.
  E3DCOLOR slotColors[256];
  for (int i = 0; i < 256; ++i)
    slotColors[i] = E3DCOLOR(96, 96, 96, 255);
  for (int i = 0; i < genLayers.size(); ++i)
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[i]))
      if (gl->detIdx >= 0 && gl->detIdx < 256)
        slotColors[gl->detIdx] = gl->layerColor;

  uint8_t *imgPtr = nullptr;
  int stride = 0;
  if (!bluewhiteTex->lockimg((void **)&imgPtr, stride, 0, TEXLOCK_READWRITE) || stride < W * 4)
  {
    if (imgPtr)
      bluewhiteTex->unlockimg();
    setShowBlueWhiteMask();
    hmlService->invalidateClipmap(false);
    return;
  }

  for (int y = 0; y < H; ++y)
  {
    E3DCOLOR *row = (E3DCOLOR *)(imgPtr + y * stride);
    for (int x = 0; x < W; ++x)
    {
      uint64_t pi = 0, pw = 0;
      detTexMap->getPackedAt(x, y, pi, pw);
      unsigned aR = 0, aG = 0, aB = 0, sumW = 0;
      while (pw)
      {
        const unsigned w = pw & 0xFF;
        const E3DCOLOR c = slotColors[pi & 0xFF];
        aR += c.r * w;
        aG += c.g * w;
        aB += c.b * w;
        sumW += w;
        pi >>= 8;
        pw >>= 8;
      }
      row[x] = sumW ? E3DCOLOR(aR / sumW, aG / sumW, aB / sumW, 255) : E3DCOLOR(0, 0, 0, 255);
    }
  }
  bluewhiteTex->unlockimg();

  setShowBlueWhiteMask();
  hmlService->invalidateClipmap(false);
}

// Debug-overlay texture: per-pixel re-evaluation of either the debug expression
// (DV_DEBUG_EXPR) or one specific layer's expression (DV_LAYER + debugLayerName),
// packed as grayscale into bluewhiteTex and bound via blue_white_mask_tex.
//
// Unlike the colored-landclass overlay, this does NOT read detTexMap -- it's
// fresh expression eval, so it visualises the layer's pre-occlusion weight
// (what the layer would output if no later layer existed). Per-pixel context
// (height, normal, curvature, world coords, mask slots, typed vars, common-expr
// eval) reuses generateLandColors's prepare_typed_vars and
// fillPerPixelExprVarsAndEvalCommon helpers, so the two paths compute the same
// values; downstream the bake runs a per-layer pass while debug runs a single
// target eval.
//
// Output dims = detTexMap dims = heightmap dims (one sample per detTexMap cell;
// no lcmScale subdiv since detTexMap stores one (lx=0,ly=0) sample per cell).
//
// Parallelised the same way as the bake: a per-thread LandColorGenData slot,
// a per-thread exprVars slice, threadpool::parallel_for_inline over rows, and
// a paired init/shutdown when no caller already brought the pool up. Threads
// write disjoint rows of bluewhiteTex (linear stride after lockimg), so no
// cross-thread synchronisation is needed inside the lambda.
void HmapLandPlugin::updateDebugWeightTex()
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  if (!detTexMap)
    return;

  // Pick target expression (debug expression, or a specific layer's).
  const Tab<uint8_t> *targetArena = nullptr;
  uint32_t targetRoot = 0;
  uint16_t targetNv = 0;
  bool targetValid = false;
  const eastl::bitset<lcexpr::MAX_VAR_ID> *targetVarMask = nullptr;
  int targetLayer = -1;
  if (debugViewMode == DV_DEBUG_EXPR)
  {
    targetArena = &debugExprArena;
    targetRoot = debugExprRoot;
    targetNv = debugExprNv;
    targetValid = debugExprValid;
    targetVarMask = &debugExprVarMask;
  }
  bool targetLayerFound = false;
  if (debugViewMode == DV_LAYER && !debugLayerName.empty())
  {
    // Resolve the targeted layer by paramName -- stable across add/move/delete
    // reorders. The lcNumLayerVars cap mirrors the picker (fillPanel) and the
    // bake's per-layer-mask slot range: a name match at index >= lcNumLayerVars
    // would be unrepresentable by the bake's exprVars layout, so treat it as a
    // miss and let the auto-cleanup below clear the mode.
    const int layerCap = min((int)genLayers.size(), lcNumLayerVars);
    for (int i = 0; i < layerCap; i++)
      if (auto *gl = PostScriptParamLandLayer::cast(genLayers[i]))
        if (strcmp(gl->paramName.str(), debugLayerName.str()) == 0)
        {
          targetArena = &gl->exprArena;
          targetRoot = gl->exprRoot;
          targetNv = gl->exprNv;
          targetValid = gl->exprValid;
          targetVarMask = &gl->exprVarMask;
          targetLayer = i;
          targetLayerFound = true;
          break;
        }
  }
  if (!targetValid)
  {
    // Distinguish "permanent" failures (named layer is gone / out of cap) from
    // "transient" ones (named layer present but its own expression is invalid
    // mid-edit, or DV_DEBUG_EXPR text didn't compile). Permanent: drop the
    // mode so the picker reflects the loss. Transient: keep the mode so a
    // successful subsequent recompile-tail repaints automatically. In both
    // cases clear the rendered-overlay flag so setShowBlueWhiteMask doesn't
    // keep binding stale bluewhiteTex content.
    if (debugViewMode == DV_LAYER && !targetLayerFound)
    {
      debugViewMode = DV_NONE;
      debugLayerName = "";
    }
    showDebugWeightOverlay = false;
    setShowBlueWhiteMask();
    hmlService->invalidateClipmap(false);
    return;
  }

  const int W = detTexMap->getMapSizeX();
  const int H = detTexMap->getMapSizeY();
  if (W <= 0 || H <= 0)
    return;

  // Per-pass scratch + setup state, mirroring generateLandColors's lcgInit.
  // Built once and replicated into per-thread lcg slots below.
  LandColorGenData lcgInit;
  lcgInit.gridCellSize = gridCellSize;
  lcgInit.heightMapSizeX = heightMap.getMapSizeX();
  lcgInit.heightMapSizeZ = heightMap.getMapSizeY();
  lcgInit.heightMapOffset = heightMapOffset;
  lcgInit.hmap = &heightMap;
  lcgInit.hmapDet = &heightMapDet;
  lcgInit.detDiv = detDivisor;
  lcgInit.detRectC = detRectC;
  lcgInit.initCurvatureFilter();

  // Reuse / (re)allocate bluewhiteTex at detTexMap dims. Same SRGBREAD format
  // as the colored-landclass overlay so the shader pipeline doesn't need a
  // special case for grayscale debug.
  if (bluewhiteTex)
  {
    TextureInfo ti;
    bluewhiteTex->getinfo(ti, 0);
    if (ti.w != W || ti.h != H || (ti.cflg & TEXFMT_MASK) != TEXFMT_A8R8G8B8 || !(ti.cflg & TEXCF_SRGBREAD))
    {
      dagGeom->shaderGlobalSetTexture(blueWhiteMaskVarId, BAD_TEXTUREID);
      dagRender->releaseManagedTexVerified(bluewhiteTexId, bluewhiteTex);
    }
  }
  if (!bluewhiteTex)
  {
    bluewhiteTex = d3d::create_tex(NULL, W, H, TEXFMT_A8R8G8B8 | TEXCF_DYNAMIC | TEXCF_SRGBREAD, 1, "blueWhite");
    G_ASSERT(bluewhiteTex);
    if (!bluewhiteTex)
    {
      hmlService->invalidateClipmap(false);
      return;
    }
    bluewhiteTexId = dagRender->registerManagedTex("bluewhiteTex", bluewhiteTex);
  }

  uint8_t *imgPtr = nullptr;
  int stride = 0;
  if (!bluewhiteTex->lockimg((void **)&imgPtr, stride, 0, TEXLOCK_READWRITE) || stride < W * 4)
  {
    if (imgPtr)
      bluewhiteTex->unlockimg();
    setShowBlueWhiteMask();
    hmlService->invalidateClipmap(false);
    return;
  }

  // Aggregate var mask: only common (once per pixel) and target (per pixel)
  // are evaluated here, so OR'ing those two is sufficient for the same purpose
  // anyVarMask serves in the bake.
  eastl::bitset<lcexpr::MAX_VAR_ID> debugVarMask;
  if (commonExprValid)
    debugVarMask |= commonExprVarMask;
  if (targetVarMask)
    debugVarMask |= *targetVarMask;
  const int numLayers = (int)genLayers.size();
  const int layerSlots = min(numLayers, lcNumLayerVars);
  dag::RelocatableFixedVector<uint8_t, LcExprContext::MAX_LAYER_VARS> layerMaskNeeded;
  layerMaskNeeded.resize(layerSlots);
  if (layerSlots > 0)
    memset(layerMaskNeeded.data(), 0, layerSlots);
  for (int l = 0; l < layerSlots && l < LcExprContext::MAX_LAYER_VARS; l++)
    if (debugVarMask.test(LcExprContext::VAR_COUNT + l))
      layerMaskNeeded[l] = 1;
  // Single-layer mode: own-layer `mask` aliases the layer's own pre-sampled
  // slot, so always sample it regardless of varMask. The bake mirrors this
  // via gl->exprVarMask.test(VAR_MASK), but here only one target eval runs.
  if (targetLayer >= 0 && targetLayer < layerSlots)
    layerMaskNeeded[targetLayer] = 1;

  // Same MAX_TEMP_REGS reservation as the bake (see lcMaxNv at the end of
  // recompileGenLayerExpressions): the parser returns nv as the post-block
  // nextVarId, but `{ block }` syntax with var/let writes peakTempRegs slots
  // ABOVE that nv. Without the headroom, eval would write past exprVars's
  // per-thread slice and corrupt the next thread's slice in threadExprVars.
  const uint16_t maxNvHere = max(targetNv, commonExprNv);
  const int exprVarsSize = max((int)maxNvHere + lcexpr::MAX_TEMP_REGS, LcExprContext::VAR_COUNT + lcNumLayerVars);

  // Worker count + threadpool init: same lazy pattern as the bake. Adopt an
  // outer pool when present; otherwise init() one and shutdown() it after.
  int workers_count;
  if (threadpool::get_num_workers() > 0)
    workers_count = threadpool::get_num_workers();
  else
    workers_count = max<int>(1, min(cpujobs::get_physical_core_count(), 4));
  const uint32_t n_threads = (uint32_t)(workers_count + 1);

  Tab<LandColorGenData> lcg;
  lcg.resize(n_threads);
  for (uint32_t t = 0; t < n_threads; t++)
    lcg[t] = lcgInit;

  Tab<float> threadExprVars;
  threadExprVars.resize(n_threads * exprVarsSize);
  memset(threadExprVars.data(), 0, threadExprVars.size() * sizeof(float));

  uint16_t typedVarsEndNv = uint16_t(LcExprContext::VAR_COUNT + lcNumLayerVars);
  dag::RelocatableFixedVector<int, 8> usedMaskTVs;
  prepare_typed_vars(typedVars, typedVarRuntime, debugVarMask, threadExprVars.data(), typedVarsEndNv, usedMaskTVs);
  for (uint32_t t = 1; t < n_threads; t++)
    memcpy(threadExprVars.data() + t * exprVarsSize, threadExprVars.data(), exprVarsSize * sizeof(float));

  const bool useCurv = debugVarMask.test(LcExprContext::VAR_CURVATURE);
  const int hxN = heightMap.getMapSizeX();
  const int hyN = heightMap.getMapSizeY();
  // Output cell count clamped to the heightmap map size (detTexMap dims match).
  const int wCells = min(W, hxN);
  const int hCells = min(H, hyN);

  const bool need_init_threadpool = threadpool::get_num_workers() == 0;
  if (need_init_threadpool)
    threadpool::init(min(cpujobs::get_physical_core_count(), 4), 1024, 256 << 10);

  // Aim for ~4 chunks per thread so re-balancing covers thread-skew (heavier
  // expressions, page faults on first touch). Floor at 1 row to keep the
  // dispatcher honest on tiny maps.
  const uint32_t quant = max<uint32_t>(1, uint32_t(hCells) / max<uint32_t>(1, n_threads * 4));

  threadpool::parallel_for_inline(0, hCells, quant, [&](uint32_t tbegin, uint32_t tend, uint32_t thread_id) {
    LandColorGenData &tlcg = lcg[thread_id];
    float *const exprVarsPtr = threadExprVars.data() + thread_id * exprVarsSize;

    for (uint32_t yU = tbegin; yU < tend; ++yU)
    {
      const int y = (int)yU;
      tlcg.curv_dy = y - LandColorGenData::FC;
      E3DCOLOR *row = (E3DCOLOR *)(imgPtr + y * stride);

      for (int x = 0; x < wCells; ++x)
      {
        real h = tlcg.sampleHeight(x, y);
        real hu = (tlcg.sampleHeight(x + 1, y) - tlcg.sampleHeight(x - 1, y)) * 0.5f;
        real hv = (tlcg.sampleHeight(x, y + 1) - tlcg.sampleHeight(x, y - 1)) * 0.5f;
        real d = gridCellSize;
        Point3 normal(-hu, d, -hv);
        real len = sqrtf(d * d + hu * hu + hv * hv);
        normal /= len;
        tlcg.normal = normal;
        const float angDiff = acos(normal.y) * 180 / PI;
        tlcg.height = h;
        tlcg.curv_dx = x - LandColorGenData::FC;
        tlcg.curvatureReady = false;
        tlcg.heightMapX = x;
        tlcg.heightMapZ = y;
        const float curv = useCurv ? tlcg.getCurvature() : 0;

        exprVarsPtr[LcExprContext::VAR_HEIGHT] = h;
        exprVarsPtr[LcExprContext::VAR_ANGLE] = angDiff;
        exprVarsPtr[LcExprContext::VAR_CURVATURE] = curv;
        exprVarsPtr[LcExprContext::VAR_MASK] = 0;
        exprVarsPtr[LcExprContext::VAR_WORLD_X] = tlcg.heightMapX * tlcg.gridCellSize + tlcg.heightMapOffset.x;
        exprVarsPtr[LcExprContext::VAR_WORLD_Y] = tlcg.heightMapZ * tlcg.gridCellSize + tlcg.heightMapOffset.y;

        const float fx = tlcg.heightMapX / tlcg.heightMapSizeX;
        const float fy = tlcg.heightMapZ / tlcg.heightMapSizeZ;
        fillPerPixelExprVarsAndEvalCommon(exprVarsPtr, fx, fy, layerSlots, make_span_const(layerMaskNeeded), typedVarsEndNv,
          make_span_const(usedMaskTVs));

        // Single-layer mode: alias the layer's pre-sampled own-mask into VAR_MASK
        // (mirrors the bake's per-layer setup in generateLandColors).
        if (targetLayer >= 0 && targetLayer < lcNumLayerVars)
          exprVarsPtr[LcExprContext::VAR_MASK] = exprVarsPtr[LcExprContext::VAR_COUNT + targetLayer];

        // Zero target's private user-var range so unassigned `var x` reads 0.
        for (int u = (int)commonExprNv; u < (int)targetNv; u++)
          exprVarsPtr[u] = 0.f;

        double wt = lcexpr::evalFinite(*targetArena, targetRoot, exprVarsPtr, (int)targetNv, &evalCurves);
        if (wt < 0.0)
          wt = 0.0;
        else if (wt > 1.0)
          wt = 1.0;
        const uint8_t g = (uint8_t)(wt * 255.0 + 0.5);
        row[x] = E3DCOLOR(g, g, g, 255);
      }
      // Pad any rightmost columns past hxN (rare: detTexMap wider than heightmap).
      for (int x = wCells; x < W; ++x)
        row[x] = E3DCOLOR(0, 0, 0, 255);
    }
  });
  if (need_init_threadpool)
    threadpool::shutdown();

  // Pad any rows past hyN. Run on the calling thread after the parallel section
  // because no per-thread state is needed and the row count is small.
  for (int y = hCells; y < H; ++y)
  {
    E3DCOLOR *row = (E3DCOLOR *)(imgPtr + y * stride);
    for (int x = 0; x < W; ++x)
      row[x] = E3DCOLOR(0, 0, 0, 255);
  }
  bluewhiteTex->unlockimg();

  // bluewhiteTex now holds a valid debug-overlay paint -- flip the rendered
  // flag back on (it may have been cleared by a transient !targetValid path
  // before this Apply restored a valid expression).
  showDebugWeightOverlay = true;
  setShowBlueWhiteMask();
  hmlService->invalidateClipmap(false);
}

// Apply a new debug view mode and refresh the overlay texture. Single source of
// truth for the showBlueWhiteMask / showLandClassColors / showDebugWeightOverlay
// trio so the panel handler doesn't have to re-derive them and so callers from
// load/refill/layer-add paths stay consistent. layer_name is meaningful only
// when mode == DV_LAYER; ignored otherwise. DV_LAYER with an empty / null
// layer_name normalizes to DV_NONE here -- otherwise the !showDebugWeightOverlay
// branch below would route to updateBlueWhiteMask(nullptr) and leave the trio
// in a persisted, non-self-healing (DV_LAYER, "") state.
void HmapLandPlugin::setDebugViewMode(int mode, const char *layer_name)
{
  if (mode < DV_NONE || mode > DV_LAYER)
    mode = DV_NONE;
  if (mode == DV_LAYER && (!layer_name || !*layer_name))
    mode = DV_NONE;

  debugViewMode = mode;
  debugLayerName = (mode == DV_LAYER) ? layer_name : "";
  showBlueWhiteMask = (mode == DV_BLUE_WHITE);
  showLandClassColors = (mode == DV_LANDCLASS_COLORS);
  showDebugWeightOverlay = (mode == DV_DEBUG_EXPR) || (mode == DV_LAYER && !debugLayerName.empty());

  if (showDebugWeightOverlay)
    updateDebugWeightTex();
  else if (showLandClassColors)
    updateLandClassColorsTex();
  else
    updateBlueWhiteMask(nullptr);
  if (DAGORED2)
    DAGORED2->invalidateViewportCache();
}

void HmapLandPlugin::updateBlueWhiteMask(const IBBox2 *rect)
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  if (!(editedScriptImage && showEditedMask()))
  {
    // No mask being edited: pick the requested overlay (debug > colorize > unbind).
    // updateGenerationMask still has to run so per-layer editable mask textures stay
    // refreshed (used by the brush regardless of overlay mode).
    if (showDebugWeightOverlay)
      updateDebugWeightTex();
    else if (showLandClassColors)
      updateLandClassColorsTex();
    else
      setShowBlueWhiteMask();
    updateGenerationMask(rect);
    hmlService->invalidateClipmap(false);
    return;
  }
  if (bluewhiteTex)
  {
    TextureInfo ti;
    bluewhiteTex->getinfo(ti, 0);
    if (ti.w != esiGridW || ti.h != esiGridH || (ti.cflg & TEXFMT_MASK) != TEXFMT_A8R8G8B8 || !(ti.cflg & TEXCF_SRGBREAD))
    {
      dagGeom->shaderGlobalSetTexture(blueWhiteMaskVarId, BAD_TEXTUREID);
      dagRender->releaseManagedTexVerified(bluewhiteTexId, bluewhiteTex);
    }
  }
  if (!bluewhiteTex)
  {
    bluewhiteTex =
      d3d::create_tex(NULL, esiGridW, esiGridH, TEXFMT_A8R8G8B8 | TEXCF_READABLE | TEXCF_DYNAMIC | TEXCF_SRGBREAD, 1, "blueWhite");
    G_ASSERT(bluewhiteTex);

    bluewhiteTexId = dagRender->registerManagedTex("bluewhiteTex", bluewhiteTex);
    setShowBlueWhiteMask();
  }
  int x0, y0, x1, y1;
  if (!rect)
  {
    x0 = y0 = 0;
    x1 = esiGridW - 1;
    y1 = esiGridH - 1;
  }
  else
  {
    x0 = clamp(int((rect->lim[0].x * gridCellSize + heightMapOffset.x - esiOrigin.x) / esiGridStep), 0, esiGridW - 1);
    y0 = clamp(int((rect->lim[0].y * gridCellSize + heightMapOffset.y - esiOrigin.y) / esiGridStep), 0, esiGridH - 1);
    x1 = clamp(int((rect->lim[1].x * gridCellSize + heightMapOffset.x - esiOrigin.x - 1) / esiGridStep + 1), 0, esiGridW - 1);
    y1 = clamp(int((rect->lim[1].y * gridCellSize + heightMapOffset.y - esiOrigin.y - 1) / esiGridStep + 1), 0, esiGridH - 1);
  }
  uint8_t *imgPtr;
  int stride;

  // fill color texture (RGBA8: write blue-white as (v, v, 255, 255) so the shader's
  // .rgb sample reproduces the legacy half4(mask, mask, 1, 1) look)
  if (bluewhiteTex->lockimg((void **)&imgPtr, stride, 0, TEXLOCK_READWRITE))
  {
    if (stride < (x1 - x0 + 1) * 4)
    {
      logerr("invalid update bluewhite (rgba) stride=%d w=%d", stride, x1 - x0 + 1);
      bluewhiteTex->unlockimg();
      return;
    }

    for (int y = y0; y <= y1; ++y)
    {
      E3DCOLOR *row = (E3DCOLOR *)(imgPtr + y * stride);
      for (int x = x0; x <= x1; ++x)
      {
        uint8_t v = real2uchar(getBrushImageData(x, y, getEditedChannel()));
        row[x] = E3DCOLOR(v, v, 255, 255);
      }
    }

    bluewhiteTex->unlockimg();
  }

  // Rebind blue_white_mask_tex + world_to_bw_mask to the mask-edit mapping.
  // Without this, switching INTO mask edit while a debug-overlay was active
  // would keep world_to_bw_mask pointing at the prior overlay's UV mapping
  // (worldW/worldH derived from detTexMap dims, not esiGridStep). The
  // setShowBlueWhiteMask call inside the create branch above only fires when
  // a fresh texture is allocated, so a reused bluewhiteTex would skip it.
  setShowBlueWhiteMask();
  updateGenerationMask(rect);
  hmlService->invalidateClipmap(false);
}

void HmapLandPlugin::editScriptImage(ScriptParam *image, int idx)
{
  if (editedScriptImage == image && editedScriptImageIdx == idx)
    image = NULL, idx = -1;

  editedScriptImage = image;
  editedScriptImageIdx = idx;

  esiGridW = esiGridH = 1;
  esiGridStep = 1;
  esiOrigin.set(0, 0);
  if (image)
  {
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage))
    {
      IBitMaskImageMgr::BitmapMask *bm = scriptImages[gl->imageIndex];
      /*if (idx >= 0)
      {
        gl->elcMaskCurIdx = idx;
        gl->elcMaskImageIndex = HmapLandPlugin::self->getScriptImage(gl->elcMaskList.getName(idx), 1, -1);
        gl->elcMaskImageBpp = HmapLandPlugin::self->getScriptImageBpp(gl->elcMaskImageIndex);

        bm = scriptImages[gl->elcMaskImageIndex];

        float sz = safediv(1.0f, gl->lc1.assetData->detTex->tile);
        esiGridStep = min(sz/bm->getWidth(), sz/bm->getHeight());
        esiGridW = sz/esiGridStep;
        esiGridH = sz/esiGridStep;
        esiOrigin = -gl->lc1.assetData->detTex->offset;
      }
      else */
      if (gl->detRectMappedMask && hasDetaledRect())
      {
        esiGridStep = min((detRect[1].x - detRect[0].x) / bm->getWidth(), (detRect[1].y - detRect[0].y) / bm->getHeight());
        esiGridW = (detRect[1].x - detRect[0].x) / esiGridStep;
        esiGridH = (detRect[1].y - detRect[0].y) / esiGridStep;
        esiOrigin = detRect[0];
      }
      else
      {
        if (gl->detRectMappedMask)
          DAEDITOR3.conError("mask <%s> is marked as detRectMappedMask, but detail rect is not present", gl->maskName);
        esiGridStep = min(getHeightmapSizeX() * gridCellSize / bm->getWidth(), getHeightmapSizeY() * gridCellSize / bm->getHeight());
        esiGridW = getHeightmapSizeX() * gridCellSize / esiGridStep;
        esiGridH = getHeightmapSizeY() * gridCellSize / esiGridStep;
        esiOrigin = heightMapOffset;
      }
    }
    else
    {
      esiGridW = getHeightmapSizeX() * lcmScale;
      esiGridH = getHeightmapSizeY() * lcmScale;
      esiGridStep = gridCellSize / lcmScale;
      esiOrigin = heightMapOffset;
    }
    updateScriptImageList();

    setShowBlueWhiteMask();
    // Always populate bluewhiteTex on edit start: the mask preview is the
    // whole point of editing, and gating on showBlueWhiteMask would suppress
    // it whenever the Debug View dropdown sits on DV_DEBUG_EXPR / DV_LAYER.
    if (showEditedMask())
      updateBlueWhiteMask(NULL);
  }

  DAGORED2->repaint();
}

real HmapLandPlugin::getBrushImageData(int x, int y, IHmapBrushImage::Channel channel)
{
  if (editedScriptImage)
  {
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage))
      return gl->getMaskDirectEx(float(x) / esiGridW, float(y) / esiGridH);

    // Stand up a minimal LandColorGenData for the duration of this brush
    // sample. ScriptParamImage::sampleImage*Pixel etc. read heightMapX/Z
    // and the size/scale fields off the passed-in lcg; they don't touch
    // hmap / hmapDet / curvWt / etc. so those stay at default.
    LandColorGenData lcg;
    lcg.heightMapX = x / float(lcmScale);
    lcg.heightMapZ = y / float(lcmScale);
    lcg.gridCellSize = gridCellSize;
    lcg.heightMapSizeX = heightMap.getMapSizeX();
    lcg.heightMapSizeZ = heightMap.getMapSizeY();

    ScriptParamImage *edImage = (ScriptParamImage *)editedScriptImage.get();
    if (edImage->bitsPerPixel == 1)
      return edImage->sampleMask1Pixel(lcg);
    else if (edImage->bitsPerPixel == 8)
      return edImage->sampleMask8Pixel(lcg);

    E3DCOLOR c = channel == IHmapBrushImage::CHANNEL_RGB ? edImage->sampleImagePixel(lcg) : edImage->sampleImagePixelTrueAlpha(lcg);

    switch (channel)
    {
      case IHmapBrushImage::CHANNEL_R: return c.r / 255.0f;
      case IHmapBrushImage::CHANNEL_G: return c.g / 255.0f;
      case IHmapBrushImage::CHANNEL_B: return c.b / 255.0f;
      case IHmapBrushImage::CHANNEL_A: return c.a / 255.0f;
    }

    return (c.r + c.g + c.b) / (255.0f * 3);
  }
  if (!detDivisor)
    return heightMap.getInitialData(x, y);
  if (insideDetRectC(x, y))
    return heightMapDet.getInitialData(x, y);
  return heightMap.getInitialData(x / detDivisor, y / detDivisor);
}


void HmapLandPlugin::setBrushImageData(int x, int y, real v, IHmapBrushImage::Channel channel)
{
  if (editedScriptImage)
  {
    int c = real2int(v * 255);

    if (c < 0)
      c = 0;
    else if (c > 255)
      c = 255;

    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage))
      return gl->setMaskDirectEx(float(x) / esiGridW, float(y) / esiGridH, c);

    LandColorGenData lcg;
    lcg.heightMapX = x / float(lcmScale);
    lcg.heightMapZ = y / float(lcmScale);
    lcg.gridCellSize = gridCellSize;
    lcg.heightMapSizeX = heightMap.getMapSizeX();
    lcg.heightMapSizeZ = heightMap.getMapSizeY();

    ScriptParamImage *edImage = (ScriptParamImage *)(ScriptParam *)editedScriptImage;
    if (edImage->bitsPerPixel == 1)
      return edImage->setMask1(lcg, c);
    else if (edImage->bitsPerPixel == 8)
      return edImage->setMask8(lcg, c);

    E3DCOLOR col = edImage->sampleImagePixelTrueAlpha(lcg);

    switch (channel)
    {
      case IHmapBrushImage::CHANNEL_RGB: col = E3DCOLOR(c, c, c, col.a); break;

      case IHmapBrushImage::CHANNEL_R: col.r = c; break;

      case IHmapBrushImage::CHANNEL_G: col.g = c; break;

      case IHmapBrushImage::CHANNEL_B: col.b = c; break;

      case IHmapBrushImage::CHANNEL_A: col.a = c; break;
    }

    edImage->paintImage(lcg, col);
    return;
  }

  if (!detDivisor)
    heightMap.setInitialData(x, y, v);
  else if (insideDetRectC(x, y))
  {
    heightMapDet.setInitialData(x, y, v);
    if (x == detRectC[0].x || y == detRectC[0].y || x == detRectC[1].x - 1 || y == detRectC[1].y - 1)
      if ((x % detDivisor) + (y % detDivisor) == 0)
        heightMap.setInitialData(x / detDivisor, y / detDivisor, v);
  }
  else if ((x % detDivisor) + (y % detDivisor) == 0)
    heightMap.setInitialData(x / detDivisor, y / detDivisor, v);
}
