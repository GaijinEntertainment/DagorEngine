// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
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
#include <math/dag_intrin.h>
#include <debug/dag_debug.h>
#include <landClassEval/lcExprFusion.h>
#include <landClassEval/lcExprGenLayerConvert.h>
#include "hmlExprFactory.h"
#include "smooth_noise.h"
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

struct LcExprContext
{
  enum VarSlot : uint16_t
  {
    VAR_HEIGHT = 0,
    VAR_ANGLE,
    VAR_CURVATURE,
    VAR_MASK,
    VAR_WORLD_X,
    VAR_WORLD_Y,
    VAR_COUNT
  };
  static_assert(VAR_COUNT <= 32, "exprVarMask is a 32-bit bitset; widen it (or computeVarMask) before raising VAR_COUNT");
  lcexpr::NameMap varMap;
  lcexpr::FuncParseMap parseMap;
  lcexpr::NodeEmitMap emitMap;
  lcexpr::FusionRule fusionRules[16] = {};
  int numFusionRules = 0;
  uint16_t nextVarId = 0;
  bool inited = false;

  void init()
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
    smooth_noise::init();

    nextVarId = 0;
    lcexpr::register_var(varMap, nextVarId, "height");
    lcexpr::register_var(varMap, nextVarId, "angle");
    lcexpr::register_var(varMap, nextVarId, "curvature");
    lcexpr::register_var(varMap, nextVarId, "mask");
    lcexpr::register_var(varMap, nextVarId, "world_x");
    lcexpr::register_var(varMap, nextVarId, "world_y");
  }
};
static LcExprContext g_lcExpr;

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

  float sampleMask1();
  float sampleMask8();
  float sampleMask1Pixel();
  float sampleMask8Pixel();
  void setMask1(bool c);
  void setMask8(char c);

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

  void flipAndSwapUV(real &u, real &v)
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

  void calcMapping(real x, real z, Point2 &p);

  E3DCOLOR sampleImage();
  float sampleImage1();
  float sampleImage8();
  void setImage(E3DCOLOR);

  E3DCOLOR sampleImagePixel();
  E3DCOLOR sampleImagePixelTrueAlpha();
  void paintImage(E3DCOLOR color);

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
  uint32_t exprVarMask = 0;
  bool useExpr = false;
  bool exprValid = false;

  // shrink_arena: reclaim unused arena capacity at the end. Set to false on the UI
  // slider-drag rebuild path (rebuildExprFromOldFormat) where compileExpr runs many
  // times per second and the realloc cost dominates.
  void compileExpr(bool shrink_arena = true)
  {
    g_lcExpr.init();
    exprArena.clear();
    exprValid = false;
    exprRoot = 0;
    exprVarMask = 0;

    Tab<lcexpr::IRNode> ir;
    lcexpr::NameMap vm(g_lcExpr.varMap);
    uint16_t nv = g_lcExpr.nextVarId;
    eastl::string err;
    int irRoot = lcexpr::parseToIR(exprText.str(), ir, g_lcExpr.parseMap, g_lcExpr.emitMap, vm, nv, err);
    if (irRoot < 0)
    {
      DAEDITOR3.conError("layer '%s' expression parse error: %s (expr='%s')", paramName.str(), err.c_str(), exprText.str());
      updateExprDisplay();
      return;
    }
    // eval reads exprVars[] unchecked, so reject any expression that references a variable
    // beyond the fixed VAR_COUNT slots we supply. This replaces what used to be a per-pixel
    // bounds check inside the node evaluators.
    int maxVarId = lcexpr::computeMaxVarId(ir, irRoot);
    if (maxVarId >= (int)LcExprContext::VAR_COUNT)
    {
      DAEDITOR3.conError("layer '%s' expression references unknown variable (max varId %d, only %d supplied): %s", paramName.str(),
        maxVarId, (int)LcExprContext::VAR_COUNT, exprText.str());
      updateExprDisplay();
      return;
    }
    exprVarMask = lcexpr::computeVarMask(ir, irRoot);
    uint32_t fused = lcexpr::tryFuse(ir, irRoot, exprArena, g_lcExpr.fusionRules, g_lcExpr.numFusionRules);
    uint32_t root = (fused != lcexpr::PARSE_ERROR) ? fused : lcexpr::compile(ir, irRoot, exprArena, g_lcExpr.emitMap);
    if (root == lcexpr::PARSE_ERROR)
    {
      DAEDITOR3.conError("layer '%s' compile failed: %s", paramName.str(), exprText.str());
      exprArena.clear();
      exprVarMask = 0;
      updateExprDisplay();
      return;
    }
    exprRoot = root;
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
    compileExpr(false); // UI slider-drag path: skip shrink_to_fit to avoid per-keystroke realloc.
                        // compileExpr() itself calls updateExprDisplay on every exit.
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
                                  "Variables: height, angle, curvature, mask, world_x, world_y\n"
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
                                  "  fbm(world_x * 0.01, world_y * 0.01, 4)";
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
        gl.compileExpr();
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
        g_lcExpr.init(); // defensive: guarantee maps are built before we use them
        SimpleString newText = panel->getText(PID_EXPR_EDIT);
        Tab<lcexpr::IRNode> ir;
        lcexpr::NameMap vm(g_lcExpr.varMap);
        uint16_t nv = g_lcExpr.nextVarId;
        eastl::string err;
        int irRoot = lcexpr::parseToIR(newText.str(), ir, g_lcExpr.parseMap, g_lcExpr.emitMap, vm, nv, err);
        if (irRoot < 0)
        {
          wingw::message_box(wingw::MBS_EXCL, "Parse error", "Expression error: %s", err.c_str());
          return;
        }
        int maxVarId = lcexpr::computeMaxVarId(ir, irRoot);
        if (maxVarId >= (int)LcExprContext::VAR_COUNT)
        {
          wingw::message_box(wingw::MBS_EXCL, "Unknown variable",
            "Expression references a variable that is not supplied.\n"
            "Only height, angle, curvature, mask, world_x, world_y are available.");
          return;
        }
        gl.exprText = newText;
        gl.compileExpr();
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
      enabled = panel.getBool(pid);
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
            HmapLandPlugin::self->addGenLayer(panel->getText(0), slotIdx);
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                  "Generation layers changed.\nRegenerate all?\n(generation may be started manually with Ctrl-G later)") ==
                wingw::MB_ID_YES)
              HmapLandPlugin::self->refillPanel(true);
            else
              HmapLandPlugin::self->refillPanel();
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
            paramName = panel->getText(0);
            HmapLandPlugin::self->refillPanel();
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
    compileExpr();
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
  inline float calcWeight(float height, float angDiff, float curvature) const;

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

  // Pass 3 + 4: lex-sort unique lc1 / lc2 names (independently), assign each
  // layer its derived landIdx / landIdx2, then register every layer's asset
  // at its new slot pair.
  rebuildLandSlots();

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
  // placeholder slot based on the layer's final position in genLayers, so
  // it must run AFTER any insert_before reorder -- moveGenLayer() rebuilds
  // at each swap, which also handles the placeholder-slot shift.
  PostScriptParamLandLayer *l = new PostScriptParamLandLayer(genLayers.size(), -1, name);
  l->exprText = l->buildExprFromOldFormat().c_str();
  l->compileExpr();
  genLayers.push_back(l);
  // writeDetTex / writeLand1 now default true in the constructor (aligned with load()),
  // so no need to special-case the first layer here.

  if (insert_before != -1)
    for (int i = genLayers.size() - 1; i > insert_before; i--)
      moveGenLayer(genLayers[i], true);
  else
    rebuildLandSlots();
}
bool HmapLandPlugin::moveGenLayer(ScriptParam *gl, bool up)
{
  for (int i = 0; i < genLayers.size(); i++)
    if (genLayers[i] == gl)
    {
      if (up && i > 0)
      {
        Ptr<ScriptParam> tmp = genLayers[i];
        genLayers[i] = genLayers[i - 1];
        genLayers[i - 1] = tmp;
        static_cast<PostScriptParamLandLayer *>(genLayers[i - 1].get())
          ->swapSlotIdx(*static_cast<PostScriptParamLandLayer *>(genLayers[i].get()));
        // Placeholder landIdx for unnamed layers is assigned in genLayers
        // order, so reordering shifts those slots. Named layers are stable
        // under reorder but re-registering is cheap for N<=20 layers.
        rebuildLandSlots();
        return true;
      }
      else if (!up && i + 1 < genLayers.size())
      {
        Ptr<ScriptParam> tmp = genLayers[i];
        genLayers[i] = genLayers[i + 1];
        genLayers[i + 1] = tmp;
        static_cast<PostScriptParamLandLayer *>(genLayers[i].get())
          ->swapSlotIdx(*static_cast<PostScriptParamLandLayer *>(genLayers[i + 1].get()));
        rebuildLandSlots();
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
      rebuildLandSlots();
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

namespace LandColorGenData
{
enum
{
  FSZ = 7,
  FC = 3
};
static real curvWt[FSZ][FSZ];
static bool curvatureReady = false;
static HeightMapStorage *hmap = NULL;
static HeightMapStorage *hmapDet = NULL;
static int detDiv = 0;
static IBBox2 detRectC;
static int curv_dx = 0, curv_dy = 0;

static Point3 normal;
static real height, curvature;

static real heightMapX, heightMapZ;

static real gridCellSize;
static Point2 heightMapOffset;
static int heightMapSizeX, heightMapSizeZ;

struct WeighedTexIdx
{
  float w;
  int idx;
  static int cmp(const WeighedTexIdx *a, const WeighedTexIdx *b)
  {
    float d = b->w - a->w;
    return d > 0 ? 1 : (d < 0 ? -1 : 0);
  }
};
static Tab<WeighedTexIdx> blendTex(inimem);

static float getNormalX() { return normal.x; }
static float getNormalY() { return normal.y; }
static float getNormalZ() { return normal.z; }
static float getHeight() { return height; }

static bool insideDetRectC(int x, int y) { return x >= detRectC[0].x && x < detRectC[1].x && y >= detRectC[0].y && y < detRectC[1].y; }
static float sampleHeight(int x, int y)
{
  if (detDiv && insideDetRectC(x * detDiv, y * detDiv))
  {
    x *= detDiv;
    y *= detDiv;
    int x0 = max(x - detDiv / 2, detRectC[0].x), y0 = max(y - detDiv / 2, detRectC[0].y);
    int x1 = min(x + detDiv / 2, detRectC[1].x), y1 = min(y + detDiv / 2, detRectC[1].y);
    float h = 0;
    for (y = y0; y <= y1; y++)
      for (x = x0; x <= x1; x++)
        h += hmapDet->getFinalData(x, y);
    return h / (x1 - x0 + 1) / (y1 - y0 + 1);
  }
  return hmap->getFinalData(x, y);
}
static float getCurvature()
{
  if (curvatureReady)
    return curvature;

  double curv = 0;
  for (int fy = 0; fy < FSZ; ++fy)
    for (int fx = 0; fx < FSZ; ++fx)
      curv += sampleHeight(fx + curv_dx, fy + curv_dy) * curvWt[fy][fx];

  curvatureReady = true;
  return curvature = curv;
}

static float getHeightMapX() { return heightMapX; }
static float getHeightMapZ() { return heightMapZ; }
static int getHeightMapSizeX() { return heightMapSizeX; }
static int getHeightMapSizeZ() { return heightMapSizeZ; }

static unsigned makeColor(int r, int g, int b, int t)
{
  if (r < 0)
    r = 0;
  else if (r > 255)
    r = 255;
  if (g < 0)
    g = 0;
  else if (g > 255)
    g = 255;
  if (b < 0)
    b = 0;
  else if (b > 255)
    b = 255;

  return E3DCOLOR(r, g, b, t);
}

static unsigned setType(unsigned u, int t)
{
  E3DCOLOR c = u;
  c.a = t;
  return c;
}

static int getType(unsigned u) { return E3DCOLOR(u).a; }

static unsigned blendColors(unsigned u1, unsigned u2, float t)
{
  if (t <= 0)
    return u1;
  if (t >= 1)
    return u2;

  E3DCOLOR c1 = u1, c2 = u2;

  return E3DCOLOR(real2int(c1.r * (1 - t) + c2.r * t), real2int(c1.g * (1 - t) + c2.g * t), real2int(c1.b * (1 - t) + c2.b * t),
    (t <= 0.5f ? c1.a : c2.a));
}
// Bit i set when slot i emitted a non-zero quantized weight in the just-
// completed generateLandColors pass (idx clamped to bit 63 as overflow).
// Drives both the lmDump-staleness rebuild trigger and the post-pass
// invalid/inactive classification, replacing the old two-pass loop.
static uint64_t lcActiveMask = 0;
static void resetBlendDetTex() { blendTex.clear(); }

static void blendDetTex(unsigned idx, float t)
{
  if (t <= 0)
    return;

  if (!blendTex.size() || t > 1.0f)
    t = 1.0f;

  int id = -1;
  for (int i = 0; i < blendTex.size(); i++)
    if (blendTex[i].idx != idx)
    {
      blendTex[i].w *= (1.0f - t);
    }
    else
    {
      blendTex[i].w = blendTex[i].w * (1.0f - t) + t;
      id = i;
    }

  if (id < 0)
  {
    WeighedTexIdx &w = blendTex.push_back();
    w.idx = idx;
    w.w = t;
  }
}

static void applyBlendDetTex(int x, int y, int def_det_tex)
{
  float sum = 0;
  if (!blendTex.size())
  {
    if (!HmapLandPlugin::self->getDetTexMap())
      return;
    WeighedTexIdx &w = blendTex.push_back();
    w.idx = def_det_tex;
    sum = w.w = 1.0f;
  }
  else
  {
    // Sum positive contributions and locate the dominant entry. The dominant
    // index is what the landClsMap detLayer records as this pixel's
    // representative texture (hmlGenColorMap.cpp reads blendTex[0].idx
    // after this call), so we swap the heaviest entry into slot 0.
    // w<=0 entries are dropped silently in the quantize loop below (they
    // would also quantize to 0 anyway).
    int maxI = 0;
    float maxW = 0;
    for (int i = 0, ie = blendTex.size(); i < ie; ++i)
      if (blendTex[i].w > 0)
      {
        sum += blendTex[i].w;
        if (blendTex[i].w > maxW)
        {
          maxW = blendTex[i].w;
          maxI = i;
        }
      }
    if (sum <= 0)
      return; // nothing positive to emit; pixel keeps whatever was there
    if (maxI != 0)
      eastl::swap(blendTex[0], blendTex[maxI]);
  }

  // prepareDetTexMaps() was already called at the top of generateLandColors,
  // so detMap is non-null and correctly sized for the current heightmap.
  hmap_storage::BlockedDetTexMap *detMap = HmapLandPlugin::self->getDetTexMap();
  if (!detMap)
    return;

  // Quantize weights into uint8 (0..255) and stream every nonzero (idx, wt)
  // pair into the block. setAt is order-independent and getPackedAtLocal
  // does its own descending top-8 at read time, so we don't need to sort or
  // pre-truncate here -- just compact in the same loop that quantizes,
  // skipping w<=0 (negative/dropped blends) and qw==0 (sub-1/255 weights
  // that would only pollute the block's slot table).
  dag::RelocatableFixedVector<uint8_t, 31, true, framemem_allocator, uint8_t, false> wIdxBuf;
  const int n = blendTex.size();
  wIdxBuf.resize(n << 1);
  auto wtBuf = wIdxBuf.data(), idxBuf = wIdxBuf.data() + n;
  float wSum255 = 255.0f / sum;
  int nCnt = 0;
  for (int i = 0; i < n; i++)
  {
    if (blendTex[i].w <= 0)
      continue;
    uint8_t qw = uint8_t(floorf(blendTex[i].w * wSum255));
    if (!qw)
      continue;
    unsigned tx = blendTex[i].idx;
    idxBuf[nCnt] = uint8_t(tx & 0xFF);
    wtBuf[nCnt] = qw;
    lcActiveMask |= 1ULL << (tx < 63 ? tx : 63);
    ++nCnt;
  }
  detMap->setAt(x, y, idxBuf, wtBuf, nCnt);
}

static unsigned mulColors(unsigned u1, unsigned u2)
{
  E3DCOLOR c1 = u1, c2 = u2;

  return E3DCOLOR((c1.r * c2.r + 127) / 255, (c1.g * c2.g + 127) / 255, (c1.b * c2.b + 127) / 255, c1.a);
}

static unsigned mulColors2x(unsigned u1, unsigned u2)
{
  E3DCOLOR c1 = u1, c2 = u2;

  int r = (c1.r * c2.r * 2 + 127) / 255;
  int g = (c1.g * c2.g * 2 + 127) / 255;
  int b = (c1.b * c2.b * 2 + 127) / 255;

  if (r > 255)
    r = 255;
  if (g > 255)
    g = 255;
  if (b > 255)
    b = 255;

  return E3DCOLOR(r, g, b, c1.a);
}

static unsigned dissolveOver(unsigned u1, unsigned u2, float t, float mask)
{
  if (t <= 0)
    return u1;
  if (t >= 1)
    return u2;

  if (t >= mask)
    return u2;
  return u1;
}

static float calcBlendK(float val0, float val1, float v)
{
  float d = val1 - val0;
  if (!float_nonzero(d))
  {
    if (v < val0)
      return 0;
    return 1;
  }

  float t = (v - val0) / d;

  if (t <= 0)
    return 0;
  if (t >= 1)
    return 1;
  return t;
}

static float smoothStep(float t)
{
  if (t <= 0)
    return 0;
  if (t >= 1)
    return 1;
  return (3 - 2 * t) * t * t;
}
}; // namespace LandColorGenData

void ScriptParamImage::calcMapping(real x, real z, Point2 &p)
{
  if (mappingType == MAPPING_HMAP_PERCENT)
  {
    p.x = x / LandColorGenData::heightMapSizeX;
    p.y = z / LandColorGenData::heightMapSizeZ;

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

    p *= LandColorGenData::gridCellSize;

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
      p.x = LandColorGenData::height;
    else
      p.y = LandColorGenData::height;

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


E3DCOLOR ScriptParamImage::sampleImage()
{
  Point2 p;
  calcMapping(LandColorGenData::heightMapX, LandColorGenData::heightMapZ, p);

  E3DCOLOR c = HmapLandPlugin::self->sampleScriptImageUV(imageIndex, p.x, p.y, clampU, clampV);
  c.a = detailType;
  return c;
}

float ScriptParamImage::sampleImage1()
{
  Point2 p;
  calcMapping(LandColorGenData::heightMapX, LandColorGenData::heightMapZ, p);

  return HmapLandPlugin::self->sampleMask1UV(imageIndex, p.x, p.y, clampU, clampV);
}

float ScriptParamImage::sampleImage8()
{
  Point2 p;
  calcMapping(LandColorGenData::heightMapX, LandColorGenData::heightMapZ, p);

  return HmapLandPlugin::self->sampleMask8UV(imageIndex, p.x, p.y, clampU, clampV);
}

void ScriptParamImage::setImage(E3DCOLOR c)
{
  Point2 p;
  calcMapping(LandColorGenData::heightMapX, LandColorGenData::heightMapZ, p);

  HmapLandPlugin::self->paintScriptImageUV(imageIndex, p.x, p.y, p.x, p.y, clampU, clampV, c);
}

bool ScriptParamImage::saveImage() { return HmapLandPlugin::self->saveImage(imageIndex); }


E3DCOLOR ScriptParamImage::sampleImagePixel()
{
  E3DCOLOR c = sampleImagePixelTrueAlpha();
  c.a = detailType;
  return c;
}


E3DCOLOR ScriptParamImage::sampleImagePixelTrueAlpha()
{
  Point2 p;
  calcMapping(LandColorGenData::heightMapX, LandColorGenData::heightMapZ, p);

  E3DCOLOR c = HmapLandPlugin::self->sampleScriptImagePixelUV(imageIndex, p.x, p.y, clampU, clampV);
  return c;
}


void ScriptParamImage::paintImage(E3DCOLOR color)
{
  Point2 p0, p1;
  calcMapping(LandColorGenData::heightMapX, LandColorGenData::heightMapZ, p0);
  calcMapping(LandColorGenData::heightMapX + 1.0f, LandColorGenData::heightMapZ + 1.0f, p1);

  HmapLandPlugin::self->paintScriptImageUV(imageIndex, p0.x, p0.y, p1.x, p1.y, clampU, clampV, color);
}

float ScriptParamMask::sampleMask1()
{
  float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
        fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask1UV(imageIndex, fx, fy);
}
float ScriptParamMask::sampleMask8()
{
  float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
        fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask8UV(imageIndex, fx, fy);
}
float ScriptParamMask::sampleMask1Pixel()
{
  float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
        fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask1PixelUV(imageIndex, fx, fy);
}
float ScriptParamMask::sampleMask8Pixel()
{
  float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
        fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
  return HmapLandPlugin::self->sampleMask8PixelUV(imageIndex, fx, fy);
}
void ScriptParamMask::setMask1(bool c)
{
  float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
        fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
  return HmapLandPlugin::self->paintMask1UV(imageIndex, fx, fy, c);
}
void ScriptParamMask::setMask8(char c)
{
  float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
        fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
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

inline float PostScriptParamLandLayer::calcWeight(float height, float angDiff, float curvature) const
{
  float wt[] = {1, 1, 1, 1};

  float *w = &wt[0];
  if (maskConv == WtRange::WMT_zero)
    *w = 0;
  else if (maskConv != WtRange::WMT_one)
  {
    float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX,
          fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
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

  if (!in_rect)
  {
    con.startProgress();
    con.setActionDesc("generating land colors...");
    con.setTotal(landClsMap.getMapSizeY() / lcmScale);
  }

  Color3 grassColor(0.5f, 0.5f, 0.1f);
  Color3 dirtColor(0.6f, 0.4f, 0.3f);

  LandColorGenData::gridCellSize = gridCellSize;
  LandColorGenData::heightMapSizeX = heightMap.getMapSizeX();
  LandColorGenData::heightMapSizeZ = heightMap.getMapSizeY();
  LandColorGenData::heightMapOffset = heightMapOffset;

  using LandColorGenData::FC;
  using LandColorGenData::FSZ;

  double wtSum = 0, kSum = 0;
  LandColorGenData::lcActiveMask = 0;
  LandColorGenData::hmap = &heightMap;
  LandColorGenData::hmapDet = &heightMapDet;
  LandColorGenData::detDiv = detDivisor;
  LandColorGenData::detRectC = detRectC;

  Tab<int> layerVal;
  int lc1_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "land");
  int lc2_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "adds");
  int lc1_base = lc1_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[lc1_li].bitCount) - 1;
  int lc2_base = lc2_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[lc2_li].bitCount) - 1;
  int phys_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "phys");
  int phys_base = phys_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[phys_li].bitCount) - 1;
  int imp_scale = impLayerIdx < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[impLayerIdx].bitCount) - 1;
  int det_val_max = detLayerIdx < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[detLayerIdx].bitCount) - 1;
  layerVal.resize(landClsLayer.size());

  for (int fy = 0; fy < FSZ; ++fy)
    for (int fx = 0; fx < FSZ; ++fx)
    {
      if (fx == FC && fy == FC)
        continue;

      int dx = fx - FC;
      int dy = fy - FC;
      real d = sqrtf(dx * dx + dy * dy);

      real k = (FC + 1 - d) / (FC + 1);
      if (k < 0)
        k = 0;

      k = (3 - 2 * k) * k * k;

      LandColorGenData::curvWt[fy][fx] = k / (d * gridCellSize);
      wtSum += LandColorGenData::curvWt[fy][fx];
      kSum += k;
    }

  LandColorGenData::curvWt[FC][FC] = -wtSum;

  for (int fy = 0; fy < FSZ; ++fy)
    for (int fx = 0; fx < FSZ; ++fx)
      LandColorGenData::curvWt[fy][fx] /= kSum;

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

  uint32_t anyVarMask = 0;
  for (int l = 0; l < genLayers.size(); l++)
  {
    auto *gl = static_cast<PostScriptParamLandLayer *>(genLayers[l].get());
    if (gl->enabled)
      anyVarMask |= gl->exprVarMask;
  }
  bool layers_use_curv = (anyVarMask & (1u << LcExprContext::VAR_CURVATURE)) != 0;

  int x0 = 0, x1 = heightMap.getMapSizeX();
  int y0 = 0, y1 = heightMap.getMapSizeY();
  if (in_rect)
  {
    x0 = in_rect->lim[0].x;
    x1 = in_rect->lim[1].x;
    y0 = in_rect->lim[0].y + 1;
    y1 = in_rect->lim[1].y + 1;
  }

  // generate landclass map and colors
  for (int y = y0; y < y1; ++y)
  {
    LandColorGenData::curv_dy = y - FC;

    for (int x = x0; x < x1; ++x)
    {
      real h = LandColorGenData::sampleHeight(x, y);

      real hu = (LandColorGenData::sampleHeight(x + 1, y) - LandColorGenData::sampleHeight(x - 1, y)) * 0.5f;
      real hv = (LandColorGenData::sampleHeight(x, y + 1) - LandColorGenData::sampleHeight(x, y - 1)) * 0.5f;

      real d = gridCellSize;

      Point3 normal(-hu, d, -hv);

      real len = sqrtf(d * d + hu * hu + hv * hv);
      normal /= len;

      LandColorGenData::normal = normal;
      float angDiff = acos(normal.y) * 180 / PI;
      LandColorGenData::height = h;

      LandColorGenData::curv_dx = x - FC;
      LandColorGenData::curvatureReady = false;

      for (int ly = 0; ly < lcmScale; ly++)
        for (int lx = 0; lx < lcmScale; lx++)
        {
          LandColorGenData::heightMapX = x + float(lx) / lcmScale;
          LandColorGenData::heightMapZ = y + float(ly) / lcmScale;

          LandColorGenData::resetBlendDetTex();
          mem_set_0(layerVal);
          int detIdx = 0;

          float curv = layers_use_curv ? LandColorGenData::getCurvature() : 0;
          bool in_det = detDivisor ? insideDetRectC(x * detDivisor, y * detDivisor) : false;

          float exprVars[LcExprContext::VAR_COUNT];
          exprVars[LcExprContext::VAR_HEIGHT] = h;
          exprVars[LcExprContext::VAR_ANGLE] = angDiff;
          exprVars[LcExprContext::VAR_CURVATURE] = curv;
          exprVars[LcExprContext::VAR_MASK] = 0;
          exprVars[LcExprContext::VAR_WORLD_X] =
            LandColorGenData::heightMapX * LandColorGenData::gridCellSize + LandColorGenData::heightMapOffset.x;
          exprVars[LcExprContext::VAR_WORLD_Y] =
            LandColorGenData::heightMapZ * LandColorGenData::gridCellSize + LandColorGenData::heightMapOffset.y;

          // --- Expression eval pass ---
          for (int l = 0; l < genLayers.size(); l++)
          {
            PostScriptParamLandLayer &gl = *static_cast<PostScriptParamLandLayer *>(genLayers[l].get());
            if (!gl.enabled)
              continue;
            if (gl.areaSelect == gl.AREA_main && in_det)
              continue;
            if (gl.areaSelect == gl.AREA_det && !in_det)
              continue;

            if (gl.exprVarMask & (1u << LcExprContext::VAR_MASK))
            {
              float fx = LandColorGenData::heightMapX / LandColorGenData::heightMapSizeX;
              float fy = LandColorGenData::heightMapZ / LandColorGenData::heightMapSizeZ;
              exprVars[LcExprContext::VAR_MASK] = gl.getMask(fx, fy);
            }
            else
              exprVars[LcExprContext::VAR_MASK] = 0;

            // Raw weight (no upper clamp). Sum-mode expressions legitimately produce
            // wt > 1; we preserve that for the importance channel so it matches the
            // pre-expression calcWeight path. Saturation happens below only for the
            // [0,1]-expected consumers (landclass gate thresholds, detTex blend).
            double wt =
              gl.exprValid ? lcexpr::evalFinite(gl.exprArena, gl.exprRoot, exprVars, LcExprContext::VAR_COUNT) : (l == 0 ? 1.0 : 0.0);
            double wtSat = wt > 1.0 ? 1.0 : wt;

// Regression tripwire: validate that the expression path agrees with the legacy
// data-driven calcWeight. Gated off by default; build with -DLCEXPR_COMPARE_WITH_OLD=1
// (or just flip to 1 here during local testing) to turn it on. Skipped for useExpr
// layers because the user-authored expression has no legacy equivalent to compare with.
#ifndef LCEXPR_COMPARE_WITH_OLD
#define LCEXPR_COMPARE_WITH_OLD 0
#endif
#if LCEXPR_COMPARE_WITH_OLD
            if (!gl.useExpr)
            {
              double wtOld = gl.calcWeight(h, angDiff, curv);
              // Both wtOld and wt are raw (unclamped). Sum-mode layers can exceed 1 on
              // both sides, so comparing raw-vs-raw is the correct invariant.
              if (fabs(wtOld - wt) > 0.01)
                DAEDITOR3.conError("EXPR/OLD MISMATCH at (%d,%d) layer %d '%s': expr=%.4f old=%.4f expr='%s'", x, y, l,
                  gl.paramName.str(), wt, wtOld, gl.exprText.str());
            }
#endif

            if (!lx && !ly && gl.writeDetTex && !gl.badDetTex && wtSat > gl.writeDetTexThres)
              LandColorGenData::blendDetTex(gl.detIdx, wtSat);
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
            LandColorGenData::applyBlendDetTex(x, y, detIdx);

          unsigned w = 0;
          if (detLayerIdx >= 0)
            w = landClsLayer[detLayerIdx].setLayerData(w,
              (getDetTexMap() && LandColorGenData::blendTex.size()) ? LandColorGenData::blendTex[0].idx : detIdx);

          for (int l = 0; l < landClsLayer.size(); l++)
            if (l != detLayerIdx)
              w = landClsLayer[l].setLayerData(w, layerVal[l]);

          landClsMap.setData(x * lcmScale + lx, y * lcmScale + ly, w);
        }
    }

    if (!in_rect)
      con.incDone();
  }

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
    for (uint64_t bw = LandColorGenData::lcActiveMask; bw; bw = __blsr(bw))
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
        if (!(LandColorGenData::lcActiveMask & (1ULL << bit)))
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
  if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(editedScriptImage))
    return showBlueWhiteMask;
  ScriptParamImage *edImage = (ScriptParamImage *)editedScriptImage.get();
  return edImage ? (edImage->bitsPerPixel == 32 ? edImage->showMask : showBlueWhiteMask) : true;
}


void HmapLandPlugin::setShowBlueWhiteMask()
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  static int worldToBwMaskVarId = dagGeom->getShaderVariableId("world_to_bw_mask");
  bool show = false;
  Color4 world_to_bw_mask;
  // Mask edit takes precedence: when a mask is being painted the user needs to see
  // the brush surface, not the colorize overlay. Colorize is the fallback when no
  // mask is active.
  if ((editedScriptImage && showEditedMask()) && showBlueWhiteMask)
  {
    show = true;
    world_to_bw_mask.r = 1.f / (esiGridW * esiGridStep);
    world_to_bw_mask.g = 1.f / (esiGridH * esiGridStep);
    world_to_bw_mask.b = (-esiOrigin.x + 0.5) * world_to_bw_mask.r;
    world_to_bw_mask.a = (-esiOrigin.y + 0.5) * world_to_bw_mask.g;
  }
  else if (showLandClassColors && bluewhiteTex && detTexMap)
  {
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

void HmapLandPlugin::updateBlueWhiteMask(const IBBox2 *rect)
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  if (!(editedScriptImage && showEditedMask()))
  {
    // No mask being edited: colorize takes precedence when enabled, otherwise unbind.
    // updateGenerationMask still has to run so per-layer editable mask textures stay
    // refreshed (used by the brush regardless of overlay mode).
    if (showLandClassColors)
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
    bluewhiteTex = d3d::create_tex(NULL, esiGridW, esiGridH, TEXFMT_A8R8G8B8 | TEXCF_DYNAMIC | TEXCF_SRGBREAD, 1, "blueWhite");
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
    if (showBlueWhiteMask)
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

    LandColorGenData::heightMapX = x / float(lcmScale);
    LandColorGenData::heightMapZ = y / float(lcmScale);

    LandColorGenData::gridCellSize = gridCellSize;
    LandColorGenData::heightMapSizeX = heightMap.getMapSizeX();
    LandColorGenData::heightMapSizeZ = heightMap.getMapSizeY();

    ScriptParamImage *edImage = (ScriptParamImage *)editedScriptImage.get();
    if (edImage->bitsPerPixel == 1)
      return edImage->sampleMask1Pixel();
    else if (edImage->bitsPerPixel == 8)
      return edImage->sampleMask8Pixel();

    E3DCOLOR c = channel == IHmapBrushImage::CHANNEL_RGB ? edImage->sampleImagePixel() : edImage->sampleImagePixelTrueAlpha();

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

    LandColorGenData::heightMapX = x / float(lcmScale);
    LandColorGenData::heightMapZ = y / float(lcmScale);

    LandColorGenData::gridCellSize = gridCellSize;
    LandColorGenData::heightMapSizeX = heightMap.getMapSizeX();
    LandColorGenData::heightMapSizeZ = heightMap.getMapSizeY();

    ScriptParamImage *edImage = (ScriptParamImage *)(ScriptParam *)editedScriptImage;
    if (edImage->bitsPerPixel == 1)
      return edImage->setMask1(c);
    else if (edImage->bitsPerPixel == 8)
      return edImage->setMask8(c);

    E3DCOLOR col = edImage->sampleImagePixelTrueAlpha();

    switch (channel)
    {
      case IHmapBrushImage::CHANNEL_RGB: col = E3DCOLOR(c, c, c, col.a); break;

      case IHmapBrushImage::CHANNEL_R: col.r = c; break;

      case IHmapBrushImage::CHANNEL_G: col.g = c; break;

      case IHmapBrushImage::CHANNEL_B: col.b = c; break;

      case IHmapBrushImage::CHANNEL_A: col.a = c; break;
    }

    edImage->paintImage(col);
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
