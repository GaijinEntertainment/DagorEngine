#include "hmlPlugin.h"
#include "landClassSlotsMgr.h"

#include <de3_hmapService.h>
#include <de3_landClassData.h>
#include <de3_assetService.h>
#include <de3_hmapDetLayerProps.h>
#include <assets/asset.h>
#include <3d/dag_drv3d.h>

#include <sqplus.h>
#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>
#include <coolConsole/coolConsole.h>
#include <dllPluginCore/core.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel2/comWnd/dialog_window.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#undef ERROR

using hdpi::_pxScaled;

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//
const int PID_STATIC_OFFSET = 10000;


static char defScript[] = "\r\n"
                          "let function degToRad(a)\r\n"
                          "{\r\n"
                          "  return a*PI/180;\r\n"
                          "}\r\n"
                          "\r\n"
                          "\r\n"
                          "::getParamsList <- function getParamsList()\r\n"
                          "{\r\n"
                          "  local list=\r\n"
                          "  [\r\n"
                          "    {name= \"fullGrassAngle\", type=\"real\", value=20},\r\n"
                          "    {name=\"startGrassAngle\", type=\"real\", value=40},\r\n"
                          "\r\n"
                          "    {name=\"sandCurvature\", type=\"real\", value=50},\r\n"
                          "\r\n"
                          "    {name=\"dirtColor\" , type=\"color\", value=makeColor(120,  90,  40, 0)},\r\n"
                          "    {name=\"sandColor\" , type=\"color\", value=makeColor(160, 150, 120, 0)},\r\n"
                          "    {name=\"grassColor\", type=\"color\", value=makeColor(120, 120,  30, 1)},\r\n"
                          "  ];\r\n"
                          "  return list;\r\n"
                          "}\r\n"
                          "\r\n"
                          "\r\n"
                          "::getGenerator <- function getGenerator()\r\n"
                          "{\r\n"
                          "  local  fullGrassCos=cos(degToRad( fullGrassAngle));\r\n"
                          "  local startGrassCos=cos(degToRad(startGrassAngle));\r\n"
                          "\r\n"
                          "  local sandCurv=sandCurvature/100.0;\r\n"
                          "\r\n"
                          "  return function(lcMap)\r\n"
                          "    : (\r\n"
                          "      getNormalY,\r\n"
                          "      getHeight,\r\n"
                          "      getCurvature,\r\n"
                          "      sampleImage,\r\n"
                          "      sampleImageAt,\r\n"
                          "      sampleMask,\r\n"
                          "      sampleMaskAlpha,\r\n"
                          "      sampleMaskRed,\r\n"
                          "      sampleMaskGreen,\r\n"
                          "      sampleMaskBlue,\r\n"
                          "      sampleMaskAt,\r\n"
                          "      sampleMaskAlphaAt,\r\n"
                          "      sampleMaskRedAt,\r\n"
                          "      sampleMaskGreenAt,\r\n"
                          "      sampleMaskBlueAt,\r\n"
                          "      setMask,\r\n"
                          "      mulColors,\r\n"
                          "      mulColors2x,\r\n"
                          "      blendColors,\r\n"
                          "      dissolveOver,\r\n"
                          "      calcBlendK,\r\n"
                          "      smoothStep,\r\n"
                          "\r\n"
                          "      fullGrassCos,\r\n"
                          "      startGrassCos,\r\n"
                          "      sandCurv,\r\n"
                          "\r\n"
                          "      dirtColor,\r\n"
                          "      sandColor,\r\n"
                          "      grassColor\r\n"
                          "    )\r\n"
                          "  {\r\n"
                          "    local normalY=getNormalY();\r\n"
                          "    local height=getHeight();\r\n"
                          "    local curvature=getCurvature();\r\n"
                          "\r\n"
                          "    // base 'dirt' / 'stone'\r\n"
                          "    local res=dirtColor;\r\n"
                          "\r\n"
                          "    // add 'sand' by curvature\r\n"
                          "    res=blendColors(res, sandColor, calcBlendK(0.0, sandCurv, curvature));\r\n"
                          "\r\n"
                          "    // add 'grass' by angle\r\n"
                          "    res=blendColors(res, grassColor,\r\n"
                          "      smoothStep(calcBlendK(startGrassCos, fullGrassCos, normalY)));\r\n"
                          "\r\n"
                          "    lcMap[0] = getType(res);\r\n"
                          "    return res;\r\n"
                          "  }\r\n"
                          "}\r\n";


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ScriptParamInt : public HmapLandPlugin::ScriptParam
{
public:
  int value;
  int paramPid;

  ScriptParamInt(const char *name, int val) : ScriptParam(name), value(val), paramPid(-1) {}

  virtual void fillParams(PropPanel2 &panel, int &pid)
  {
    paramPid = pid++;
    panel.createEditInt(paramPid, paramName, value);
  }

  virtual void onPPChange(int pid, PropPanel2 &panel)
  {
    if (pid != paramPid)
      return;
    value = panel.getInt(pid);
  }

  virtual void save(DataBlock &blk) { blk.setInt(paramName, value); }

  virtual void load(const DataBlock &blk) { value = blk.getInt(paramName, value); }

  virtual void setToScript(HSQUIRRELVM vm)
  {
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_pushinteger(vm, value);
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);
  }
};


class ScriptParamReal : public HmapLandPlugin::ScriptParam
{
public:
  real value;
  int paramPid;

  ScriptParamReal(const char *name, real val) : ScriptParam(name), value(val), paramPid(-1) {}

  virtual void fillParams(PropPanel2 &panel, int &pid)
  {
    paramPid = pid++;
    panel.createEditFloat(paramPid, paramName, value);
  }

  virtual void onPPChange(int pid, PropPanel2 &panel)
  {
    if (pid != paramPid)
      return;
    value = panel.getFloat(pid);
  }

  virtual void save(DataBlock &blk) { blk.setReal(paramName, value); }

  virtual void load(const DataBlock &blk) { value = blk.getReal(paramName, value); }

  virtual void setToScript(HSQUIRRELVM vm)
  {
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_pushfloat(vm, value);
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);
  }
};


class ScriptParamColor : public HmapLandPlugin::ScriptParam
{
public:
  E3DCOLOR value;
  int paramPid;

  ScriptParamColor(const char *name, E3DCOLOR val) : ScriptParam(name), value(val), paramPid(-1) {}

  virtual void fillParams(PropPanel2 &panel, int &pid)
  {
    paramPid = pid++;
    panel.createColorBox(paramPid, paramName, value);
  }

  virtual void onPPChange(int pid, PropPanel2 &panel)
  {
    if (pid != paramPid)
      return;
    value = panel.getColor(pid);
  }

  virtual void save(DataBlock &blk) { blk.setE3dcolor(paramName, value); }

  virtual void load(const DataBlock &blk) { value = blk.getE3dcolor(paramName, value); }

  virtual void setToScript(HSQUIRRELVM vm)
  {
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_pushinteger(vm, value);
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);
  }
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

  virtual void *get_interface(int id) { return id == 'Mask' ? this : NULL; }

  virtual void fillParams(PropPanel2 &panel, int &pid)
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

  virtual void onPPChange(int pid, PropPanel2 &panel) {}

  virtual void onPPBtnPressed(int pid, PropPanel2 &panel)
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

  virtual void save(DataBlock &main_blk) {}

  virtual void load(const DataBlock &main_blk) {}

  virtual void setToScript(HSQUIRRELVM vm)
  {
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_pushuserpointer(vm, this);
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);
  }

  virtual void finishEdit(PropPanel2 &panel) { panel.setCaption(editButtonPid, "Edit"); }

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

  virtual void *get_interface(int id) { return (id == 'Mask' || id == 'Img') ? this : NULL; }

  virtual void fillParams(PropPanel2 &panel, int &pid)
  {

    PropertyContainerControlBase *op = panel.createGroup(pid++, paramName);

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
    PropertyContainerControlBase *rg = op->createRadioGroup(mappingTypePid, "Mapping type", mappingType);
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

  virtual void onPPChange(int pid, PropPanel2 &panel)
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

  virtual void onPPBtnPressed(int pid, PropPanel2 &panel)
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

  virtual void save(DataBlock &main_blk)
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

  virtual void load(const DataBlock &main_blk)
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

  virtual void setToScript(HSQUIRRELVM vm)
  {
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_pushuserpointer(vm, this);
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);
  }

  virtual void finishEdit(PropPanel2 &panel)
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

  E3DCOLOR ScriptParamImage::sampleImageAt(real u, real v)
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
  ~ScriptParamLandClass()
  {
    unregisterAsset();
    LandClassSlotsManager::unsubscribeLandClassUpdateNotify(this);
  }

  virtual void fillParams(PropPanel2 &panel, int &pid)
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

  virtual void onPPChange(int pid, PropPanel2 &panel) {}

  virtual void onPPBtnPressed(int pid, PropPanel2 &panel)
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

  virtual void save(DataBlock &main_blk)
  {
    DataBlock &blk = *main_blk.addBlock(paramName);

    blk.setStr("asset", asset);
  }

  virtual void load(const DataBlock &main_blk)
  {
    const DataBlock &blk = *main_blk.getBlockByNameEx(paramName);
    const char *name = blk.getStr("asset", NULL);
    if (name)
      asset = name;
    registerAsset();
  }

  virtual void setToScript(HSQUIRRELVM vm)
  {
    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_newarray(vm, perLayerAssetIdx.size());
    for (int i = 0; i < perLayerAssetIdx.size(); i++)
    {
      sq_pushinteger(vm, i);
      sq_pushinteger(vm, (i == det_layer_id) ? detailIdx : perLayerAssetIdx[i]);
      sq_rawset(vm, -3);
    }
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);

    for (int i = 0; i < images.size(); i++)
      images[i]->setToScript(vm);
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
  virtual void onLandClassAssetChanged(landclass::AssetData *data)
  {
    if (assetData == data)
    {
      unregisterAsset();
      registerAsset();
      HmapLandPlugin::self->getLandClassMgr().regenerateObjects(HmapLandPlugin::self->getlandClassMap());
    }
  }
  virtual void onLandClassAssetTexturesChanged(landclass::AssetData *data) override {}
  virtual void onSplineClassAssetChanged(splineclass::AssetData *data) {}
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
  ~ScriptParamDetailTex() { unregisterDetTex(); }

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

  virtual void fillParams(PropPanel2 &panel, int &pid)
  {
    String btn_name;
    PropertyContainerControlBase *op = panel.createGroup(pid++, String(64, "%s: slot %d", (char *)paramName, p.detailSlot));

    basePid = pid;
    pid += IPID__NUM;

    // detail tex
    if (!prepareName(btn_name, p.blkName))
      btn_name = "---";
    op->createButton(basePid + IPID_LANDCLASS, btn_name);
    op->setBoolValue(true);
  }

  virtual void onPPChange(int pid, PropPanel2 &panel) {}

  virtual void onPPBtnPressed(int pid, PropPanel2 &panel)
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

  virtual void save(DataBlock &main_blk)
  {
    DataBlock &blk = *main_blk.addBlock(paramName);
    blk.setStr("asset", p.blkName);
  }

  virtual void load(const DataBlock &main_blk)
  {
    const DataBlock &blk = *main_blk.getBlockByNameEx(paramName);
    const char *asset = blk.getStr("asset", NULL);

    if (asset)
      p.blkName = asset;
    registerDetTex();
  }

  virtual void setToScript(HSQUIRRELVM vm)
  {
    sq_pushroottable(vm);
    sq_pushstring(vm, paramName, -1);
    sq_pushinteger(vm, p.detailSlot);
    sq_newslot(vm, -3, false);
    sq_pop(vm, 1);
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
  bool sumWeights;
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

  WtRange ht, ang, curv;
  bool writeDetTex, writeLand1, writeLand2, writeImportance;
  float writeDetTexThres, writeLand1Thres, writeLand2Thres;
  bool editableLandClass;

  struct PropsDlg : public ControlEventHandler
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
    PropsDlg(PostScriptParamLandLayer &_gl) : gl(_gl)
    {
      dlg = EDITORCORE->createDialog(_pxScaled(290), _pxScaled(460),
        String(0, gl.paramName.empty() ? "Layer #%d" : "Layer #%d: %s", gl.slotIdx + 1, gl.paramName));
      ::GetWindowRect((HWND)dlg->getHandle(), &wr);
      dlg->setCloseHandler(this);
      dlg->showButtonPanel(false);
    }
    ~PropsDlg()
    {
      EDITORCORE->deleteDialog(dlg);
      dlg = NULL;
    }

    bool isVisible() const { return dlg->isVisible(); }
    void show()
    {
      ::SetWindowPos((HWND)dlg->getHandle(), NULL, wr.left, wr.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
      dlg->show();
      PropertyContainerControlBase &panel = *dlg->getPanel();
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

      PropertyContainerControlBase &pCalc = *panel.createGroup(PID_LAYER_WEIGHTCALC, "Weight formula");
      PropertyContainerControlBase &pOut = *panel.createGroup(PID_LAYER_OUTPUT, "Output and thresholds");

      pCalc.createCheckBox(PID_LAYER_SUMWT, "Sum weights (instead of multiply)", gl.sumWeights);
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
      pCalc.createEditFloat(PID_WTR_CURV_V0, "base", gl.curv.v0, 2, gl.ht.conv);
      pCalc.createEditFloat(PID_WTR_CURV_DV, "delta", gl.curv.dv, 2, gl.ht.conv, false);

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

          PropertyContainerControlBase &pElc = *panel.createExtGroup(base + PID_ELC_GRP, String(0, "Land detail layer #%d", i + 1));
          panel.setInt(base + PID_ELC_GRP, (1 << EXT_BUTTON_INSERT) | (1 << EXT_BUTTON_REMOVE) | (i > 0 ? (1 << EXT_BUTTON_UP) : 0) |
                                             (i + 1 < gl.elcLayers.blockCount() ? (1 << EXT_BUTTON_DOWN) : 0));

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
      ::SetWindowText((HWND)dlg->getHandle(),
        String(0, gl.paramName.empty() ? "Layer #%d" : "Layer #%d: %s", gl.slotIdx + 1, gl.paramName));
    }

    virtual void onChange(int pid, PropertyContainerControlBase *panel)
    {
      if (pid == PID_LAYER_SUMWT)
        gl.sumWeights = panel->getBool(pid);
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
        if (PropPanel2 *pp = HmapLandPlugin::self->getPropPanel())
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
      else if (pid == PID_WRITE_LC1)
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
    virtual void onClick(int pid, PropertyContainerControlBase *panel)
    {
      if (pid == -DIALOG_ID_CLOSE)
      {
        ::GetWindowRect((HWND)dlg->getHandle(), &wr);
        if (PropPanel2 *pp = HmapLandPlugin::self->getPropPanel())
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
        const char *name = HmapLandPlugin::self->pickScriptImage(gl.maskName, 8);
        if (name)
        {
          gl.maskName = name;
          if (*name && gl.maskConv == WtRange::WMT_one)
            gl.maskConv = WtRange::WMT_asIs;
          else if (!*name && gl.maskConv != WtRange::WMT_one)
            gl.maskConv = WtRange::WMT_one;

          panel->setInt(PID_WTR_MASK_CONV, gl.maskConv);
          panel->setEnabledById(PID_MASK_REFSYS, HmapLandPlugin::self->hasDetaledRect() && gl.maskConv);
          panel->setEnabledById(PID_SEL_MASK, gl.maskConv);
          panel->setCaption(pid, *name ? name : "-- no mask --");
          if (PropPanel2 *pp = HmapLandPlugin::self->getPropPanel())
          {
            pp->setCaption(gl.pidBase + gl.PID_SEL_MASK, *name ? name : "-- no mask --");
            pp->setEnabledById(gl.pidBase + gl.PID_SEL_MASK, true);
            pp->setEnabledById(gl.pidBase + gl.PID_EDIT_MASK, gl.maskConv);
          }
          gl.imageIndex = HmapLandPlugin::self->getScriptImage(gl.maskName, 1, -1);
          gl.imageBpp = HmapLandPlugin::self->getScriptImageBpp(gl.imageIndex);
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
          case EXT_BUTTON_REMOVE:
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                  "Do you really want to delete land detail layer #%d (%s)?", lidx + 1,
                  gl.elcLayers.getBlock(lidx)->getStr("lName")) != wingw::MB_ID_YES)
              return;
            gl.elcLayers.removeBlock(lidx);
            break;

          case EXT_BUTTON_INSERT:
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

          case EXT_BUTTON_UP:
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

          case EXT_BUTTON_DOWN:
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
    virtual void onDoubleClick(int pid, PropertyContainerControlBase *panel) {}
    virtual void onPostEvent(int pid, PropPanel2 *panel)
    {
      if (pid == 1)
        refillPanel();
    }

    PostScriptParamLandLayer &gl;
    CDialogWindow *dlg;
    RECT wr;
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
    sumWeights = false;
    badDetTex = false;
    detIdx = slotIdx;
    detRectMappedMask = false;
    editableLandClass = false;
    drdHandle = NULL;
    elcMaskCurIdx = elcMaskImageIndex = elcMaskImageBpp = -1;
    areaSelect = 0;
    writeDetTex = writeLand1 = writeLand2 = writeImportance = false;
    writeDetTexThres = writeLand1Thres = writeLand2Thres = 1e-3;
    LandClassSlotsManager::subscribeLandClassUpdateNotify(this);
    dlg = new PropsDlg(*this);
  }
  ~PostScriptParamLandLayer()
  {
    HmapLandPlugin::hmlService->destroyDetailRenderData(drdHandle);
    unregisterAsset(lc1);
    unregisterAsset(lc2);
    LandClassSlotsManager::unsubscribeLandClassUpdateNotify(this);
    del_it(dlg);
  }
  virtual void *get_interface(int id) { return id == 'Layr' ? this : NULL; }

  virtual void fillParams(PropPanel2 &_panel, int &pid)
  {
    pidBase = pid;
    PropertyContainerControlBase &panel = *_panel.createExtGroup(pidBase + PID_LAYER_CAPTION,
      String(0, paramName.empty() ? "Layer #%d" : "Layer #%d: %s", slotIdx + 1, paramName));
    _panel.setInt(pidBase + PID_LAYER_CAPTION,
      (1 << EXT_BUTTON_INSERT) | (1 << EXT_BUTTON_REMOVE) | (1 << EXT_BUTTON_RENAME) | (1 << EXT_BUTTON_UP) | (1 << EXT_BUTTON_DOWN));

    panel.createCheckBox(pidBase + PID_LAYER_ENABLED, "Layer enabled", enabled);
    panel.createButton(pidBase + PID_EDIT_PROPS, "Props...", true, false);
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


  virtual void onPPChange(int pid, PropPanel2 &panel)
  {
    if (pid == pidBase + PID_LAYER_ENABLED)
      enabled = panel.getBool(pid);
  }

  virtual void onPPBtnPressed(int pid, PropPanel2 &panel)
  {
    if (pid == pidBase + PID_SEL_LC1)
    {
      const char *lc_asset = DAEDITOR3.selectAssetX(lc1.asset, "Select primary landclass", "land");

      if (lc_asset)
      {
        lc1.asset = lc_asset;
        panel.setCaption(pid, lc_asset);
        registerAsset(lc1);
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
        registerAsset(lc2);

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
    else if (pid == -DIALOG_ID_CLOSE)
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
      const char *name = HmapLandPlugin::self->pickScriptImage(maskName, 8);
      if (name)
      {
        if (*name && maskConv == WtRange::WMT_one)
          maskConv = WtRange::WMT_asIs;
        else if (!*name && maskConv != WtRange::WMT_one)
          maskConv = WtRange::WMT_one;

        maskName = name;
        panel.setCaption(pid, *name ? name : "-- no mask --");
        dlg->dlg->getPanel()->setCaption(dlg->PID_SEL_MASK, *name ? name : "-- no mask --");
        dlg->dlg->getPanel()->setInt(dlg->PID_WTR_MASK_CONV, maskConv);
        dlg->dlg->getPanel()->setEnabledById(dlg->PID_MASK_REFSYS, HmapLandPlugin::self->hasDetaledRect() && maskConv);
        dlg->dlg->getPanel()->setEnabledById(dlg->PID_SEL_MASK, maskConv);
        panel.setEnabledById(pidBase + PID_EDIT_MASK, maskConv);
        imageIndex = HmapLandPlugin::self->getScriptImage(maskName, 1, -1);
        imageBpp = HmapLandPlugin::self->getScriptImageBpp(imageIndex);
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
        case EXT_BUTTON_REMOVE:
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

        case EXT_BUTTON_INSERT:
        {
          CDialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(120), "Insert generation layer");
          PropPanel2 *panel = dialog->getPanel();
          panel->createEditBox(0, "Enter generation layer name:");
          panel->setFocusById(0);

          int ret = dialog->showDialog();
          if (ret == DIALOG_ID_OK)
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

        case EXT_BUTTON_UP:
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

        case EXT_BUTTON_DOWN:
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

        case EXT_BUTTON_RENAME:
        {
          CDialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(120), "Rename generation layer");
          PropPanel2 *panel = dialog->getPanel();
          panel->createEditBox(0, "Change generation layer name:", paramName);
          panel->setFocusById(0);

          int ret = dialog->showDialog();
          if (ret == DIALOG_ID_OK)
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
  virtual bool onPPChangeEx(int pid, PropPanel2 &p)
  {
    if (pid < pidBase || pid >= pidBase + PID__COUNT)
      return false;
    onPPChange(pid, p);
    ;
    return true;
  }
  virtual bool onPPBtnPressedEx(int pid, PropPanel2 &p)
  {
    if (pid < pidBase || pid >= pidBase + PID__COUNT)
      return false;
    onPPBtnPressed(pid, p);
    return true;
  }

  virtual void finishEdit(PropPanel2 &panel)
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

  virtual void save(DataBlock &blk)
  {
    blk.setStr("name", paramName);
    if (!enabled)
      blk.setBool("enabled", enabled);
    if (sumWeights)
      blk.setBool("sumWt", sumWeights);
    if (!lc1.asset.empty())
      blk.setStr("lc1", lc1.asset);
    if (!lc2.asset.empty())
      blk.setStr("lc2", lc2.asset);
    blk.setInt("lcIdx", landIdx);

    if (areaSelect)
      blk.setInt("area_select", areaSelect);

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

  virtual void load(const DataBlock &blk)
  {
    paramName = blk.getStr("name", "");
    enabled = blk.getBool("enabled", true);
    sumWeights = blk.getBool("sumWt", false);
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
  }

  virtual void setToScript(HSQUIRRELVM vm) {}

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


  void registerAsset(LandClassRec &lc)
  {
    if (lc.asset.empty())
      return unregisterAsset(lc);

    void *handle = HmapLandPlugin::self->getLayersHandle();
    int lc_li = HmapLandPlugin::hmlService->getBitLayerIndexByName(handle, &lc == &lc1 ? "land" : "adds");
    int lc_base = lc_li < 0 ? 0 : (1 << HmapLandPlugin::hmlService->getBitLayersList(handle)[lc_li].bitCount) - 1;
    if (lc_li >= 0 && landIdx >= 0)
      HmapLandPlugin::self->getLandClassMgr().setLandClass(lc_li, lc_base - landIdx, lc.asset);

    int phys_li = HmapLandPlugin::hmlService->getBitLayerIndexByName(handle, "phys");
    if (phys_li >= 0 && landIdx >= 0)
      HmapLandPlugin::self->getLandClassMgr().setLandClass(phys_li, landIdx, lc.asset);

    regetAsset(lc);
    if (&lc == &lc2)
      return;

    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    int det_base =
      (det_layer_id < 0 || !HmapLandPlugin::self->usesGenScript())
        ? 0
        : (1 << HmapLandPlugin::hmlService->getBitLayersList(HmapLandPlugin::self->getLayersHandle())[det_layer_id].bitCount) - 1;
    if (det_layer_id >= 0 && slotIdx >= 0 && lc.assetData)
      HmapLandPlugin::self->setDetailTexSlot(detIdx = (det_base ? det_base - slotIdx : slotIdx), lc.asset);
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

    if (&lc == &lc2)
      return;

    int det_layer_id = HmapLandPlugin::self->getDetLayerIdx();
    int det_base =
      (det_layer_id < 0 || !HmapLandPlugin::self->usesGenScript())
        ? 0
        : (1 << HmapLandPlugin::hmlService->getBitLayersList(HmapLandPlugin::self->getLayersHandle())[det_layer_id].bitCount) - 1;
    if (det_layer_id >= 0 && slotIdx >= 0)
      HmapLandPlugin::self->setDetailTexSlot(detIdx = (det_base ? det_base - slotIdx : slotIdx), NULL);
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
  virtual void onLandClassAssetChanged(landclass::AssetData *data)
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
  virtual void onLandClassAssetTexturesChanged(landclass::AssetData *data) override {}
  virtual void onSplineClassAssetChanged(splineclass::AssetData *data) {}


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
  Tab<bool> usedIdx(tmpmem);
  int lc1_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "land");
  int bits = lc1_li < 0 ? 0 : hmlService->getBitLayersList(getLayersHandle())[lc1_li].bitCount;
  usedIdx.resize((1 << bits) - 1);
  mem_set_0(usedIdx);

  for (int i = 0, idx = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      PostScriptParamLandLayer *l = new PostScriptParamLandLayer(idx, 0, "");
      l->load(*blk.getBlock(i));
      if (l->landIdx < 0 || l->landIdx >= usedIdx.size() || usedIdx[l->landIdx])
      {
        l->landIdx = find_value_idx(usedIdx, false);
        DAEDITOR3.conWarning("replaced layer[%d].landIdx with %d", idx, l->landIdx);
      }
      if (l->landIdx >= 0)
        usedIdx[l->landIdx] = true;
      genLayers.push_back(l);
      l->registerAssets();
      idx++;
    }
  return true;
}
bool HmapLandPlugin::saveGenLayers(DataBlock &blk)
{
  for (int i = 0; i < genLayers.size(); i++)
    genLayers[i]->save(*blk.addNewBlock("layer"));
  return true;
}
void HmapLandPlugin::prepareEditableLandClasses()
{
  for (int i = 0; i < genLayers.size(); i++)
    if (PostScriptParamLandLayer *gl = PostScriptParamLandLayer::cast(genLayers[i]))
      gl->prepareDrdHandle();
}
void HmapLandPlugin::addGenLayer(const char *name, int insert_before)
{
  Tab<bool> usedIdx(tmpmem);
  int lc1_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "land");
  int bits = lc1_li < 0 ? 0 : hmlService->getBitLayersList(getLayersHandle())[lc1_li].bitCount;
  usedIdx.resize((1 << bits) - 1);
  mem_set_0(usedIdx);
  for (int i = 0; i < genLayers.size(); i++)
  {
    PostScriptParamLandLayer &gl = *static_cast<PostScriptParamLandLayer *>(genLayers[i].get());
    if (gl.landIdx >= 0)
      usedIdx[gl.landIdx] = true;
  }

  PostScriptParamLandLayer *l = new PostScriptParamLandLayer(genLayers.size(), find_value_idx(usedIdx, false), name);
  genLayers.push_back(l);
  if (genLayers.size() == 1)
    l->writeDetTex = l->writeLand1 = true;

  l->registerAssets();
  if (insert_before != -1)
    for (int i = genLayers.size() - 1; i > insert_before; i--)
      moveGenLayer(genLayers[i], true);
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
        static_cast<PostScriptParamLandLayer *>(genLayers[i - 1].get())->changeSlotIdx(i - 1);
        static_cast<PostScriptParamLandLayer *>(genLayers[i].get())->changeSlotIdx(i);
        return true;
      }
      else if (!up && i + 1 < genLayers.size())
      {
        Ptr<ScriptParam> tmp = genLayers[i];
        genLayers[i] = genLayers[i + 1];
        genLayers[i + 1] = tmp;
        static_cast<PostScriptParamLandLayer *>(genLayers[i].get())->changeSlotIdx(i);
        static_cast<PostScriptParamLandLayer *>(genLayers[i + 1].get())->changeSlotIdx(i + 1);
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
static bool blendedInvalidLandClasses = false;
static void resetBlendDetTex() { blendTex.clear(); }

static void blendDetTex(unsigned idx, float t)
{
  if (t <= 0)
    return;

  if (!blendTex.size() || t > 1.0f)
    t = 1.0f;
  if (idx >= HmapLandPlugin::self->lcRemap.size() || HmapLandPlugin::self->lcRemap[idx] == 0xFF)
    blendedInvalidLandClasses = true;

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
    if (!HmapLandPlugin::self->getDetTexIdxMap())
      return;
    WeighedTexIdx &w = blendTex.push_back();
    w.idx = def_det_tex;
    sum = w.w = 1.0f;
  }
  else
  {
    sort(blendTex, &WeighedTexIdx::cmp);
    // if (blendTex.size() > HmapLandPlugin::HMAX_DET_TEX_NUM)
    //   blendTex.resize(HmapLandPlugin::HMAX_DET_TEX_NUM);
    for (int i = 0; i < blendTex.size(); i++)
      if (blendTex[i].w > 0)
        sum += blendTex[i].w;
      else
      {
        blendTex.resize(i);
        break;
      }
    /*if (blendTex.size() > 1)
      debug("blend %d tex, sum=%.4f, %.3f,%.3f,%.3f", blendTex.size(), sum,
        blendTex[0].w, blendTex[1].w, blendTex[2].w);*/
  }

  MapStorage<uint64_t> *map_idx = HmapLandPlugin::self->getDetTexIdxMap();
  MapStorage<uint64_t> *map_wt = HmapLandPlugin::self->getDetTexWtMap();
  if (!map_idx)
  {
    HmapLandPlugin::self->prepareDetTexMaps();
    map_idx = HmapLandPlugin::self->getDetTexIdxMap();
    map_wt = HmapLandPlugin::self->getDetTexWtMap();
  }

  uint64_t p_idx = 0;
  uint64_t p_wt = 0;
  for (int i = 0; i < blendTex.size(); i++)
  {
    p_wt |= (uint64_t(floorf(blendTex[i].w / sum * 255.0f)) & 0xFF) << (i * 8);
    p_idx |= (uint64_t(blendTex[i].idx) & 0xFF) << (i * 8);
  }
  /*if (blendTex.size() > 1)
    debug("wt=%08X%08X idx=%08X%08X",
      unsigned(p_wt>>32), unsigned(p_wt), unsigned(p_idx>>32), unsigned(p_idx));*/

  map_idx->setData(x, y, p_idx);
  map_wt->setData(x, y, p_wt);
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

static unsigned sampleImage(SqPlus::SQAnythingPtr ptr)
{
  if (!ptr)
    return E3DCOLOR(255, 255, 255, 0);

  E3DCOLOR c = ((ScriptParamImage *)ptr)->sampleImage();
  return c;
}

static float sampleImage1(SqPlus::SQAnythingPtr ptr) { return ptr ? ((ScriptParamImage *)ptr)->sampleImage1() : 1.0; }

static float sampleImage8(SqPlus::SQAnythingPtr ptr) { return ptr ? ((ScriptParamImage *)ptr)->sampleImage8() : 1.0; }

static unsigned sampleImageAt(SqPlus::SQAnythingPtr ptr, float u, float v)
{
  if (!ptr)
    return E3DCOLOR(255, 255, 255, 0);

  E3DCOLOR c = ((ScriptParamImage *)ptr)->sampleImageAt(u, v);
  return c;
}

static bool saveImage(SqPlus::SQAnythingPtr ptr) { return ((ScriptParamImage *)ptr)->saveImage(); }

static void setMask(SqPlus::SQAnythingPtr ptr, float val)
{
  ScriptParamMask &m = *(ScriptParamMask *)ptr;
  if (m.bitsPerPixel == 1)
    m.setMask1(val > 0);
  else if (m.bitsPerPixel == 8)
    m.setMask8((unsigned char)(val * 255.0));
}

static void setMask1(SqPlus::SQAnythingPtr ptr, float val)
{
  if (!ptr)
    return;
  ((ScriptParamMask *)ptr)->setMask1(val > 0);
}

static void setMask8(SqPlus::SQAnythingPtr ptr, float val)
{
  if (!ptr)
    return;
  ((ScriptParamMask *)ptr)->setMask8((unsigned char)(val * 255.0));
}

static float sampleMask(SqPlus::SQAnythingPtr ptr)
{
  ScriptParamMask &m = *(ScriptParamMask *)ptr;
  if (m.bitsPerPixel == 1)
    return m.sampleMask1();
  if (m.bitsPerPixel == 8)
    return m.sampleMask8();
  if (m.bitsPerPixel == 32)
  {
    E3DCOLOR c = ((ScriptParamImage *)ptr)->sampleImage();
    return ((int)c.r + (int)c.g + (int)c.b) / 3.0 / 255.0;
  }
  return 1.0;
}

static float sampleMask1(SqPlus::SQAnythingPtr ptr) { return ptr ? ((ScriptParamMask *)ptr)->sampleMask1() : 1.0; }

static float sampleMask8(SqPlus::SQAnythingPtr ptr) { return ptr ? ((ScriptParamMask *)ptr)->sampleMask8() : 1.0; }

static void registerInScript(HSQUIRRELVM vm)
{
#define REG(func) SqPlus::RegisterGlobal(vm, func, _T(#func))

  REG(getNormalX);
  REG(getNormalY);
  REG(getNormalZ);
  REG(getHeight);
  REG(getCurvature);
  REG(getHeightMapX);
  REG(getHeightMapZ);
  REG(getHeightMapSizeX);
  REG(getHeightMapSizeZ);
  REG(makeColor);
  REG(setType);
  REG(getType);
  REG(blendColors);
  REG(blendDetTex);
  REG(mulColors);
  REG(mulColors2x);
  REG(dissolveOver);
  REG(calcBlendK);
  REG(smoothStep);
  REG(sampleImage);
  REG(sampleImage1);
  REG(sampleImage8);
  REG(sampleImageAt);
  REG(setMask);
  REG(setMask1);
  REG(setMask8);
  REG(sampleMask);
  REG(sampleMask1);
  REG(sampleMask8);
  REG(saveImage);

#undef REG
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
  if (ht.conv == WtRange::WMT_zero)
    *w = 0;
  else if (ht.conv != WtRange::WMT_one)
  {
    if (fabsf(ht.dv) < 1e-4)
      *w = (fabsf(height - ht.v0) < 1e-4);
    else
      *w = clamp((height - ht.v0) / ht.dv, 0.0f, 1.0f);
    if (ht.conv == WtRange::WMT_smoothStep)
      *w = LandColorGenData::smoothStep(*w);
  }

  w++;
  if (ang.conv == WtRange::WMT_zero)
    *w = 0;
  else if (ang.conv != WtRange::WMT_one)
  {
    if (fabsf(ang.dv) < 1e-4)
      *w = (fabsf(angDiff - ang.v0) < 1e-4);
    else
      *w = clamp((angDiff - ang.v0) / ang.dv, 0.0f, 1.0f);
    if (ang.conv == WtRange::WMT_smoothStep)
      *w = LandColorGenData::smoothStep(*w);
  }

  w++;
  if (curv.conv == WtRange::WMT_zero)
    *w = 0;
  else if (curv.conv != WtRange::WMT_one)
  {
    if (fabsf(curv.dv) < 1e-4)
      *w = (fabsf(curvature - curv.v0) < 1e-4);
    else
      *w = clamp((curvature - curv.v0) / curv.dv, 0.0f, 1.0f);
    if (curv.conv == WtRange::WMT_smoothStep)
      *w = LandColorGenData::smoothStep(*w);
  }

  return sumWeights ? (wt[0] + wt[1] + wt[2] + wt[3]) : (wt[0] * wt[1] * wt[2] * wt[3]);
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

static void scriptPrintFunc(HSQUIRRELVM v, const SQChar *s, ...)
{
  static char temp[2048];
  va_list vl;
  va_start(vl, s);
  vsprintf(temp, s, vl);
  va_end(vl);

  int len = (int)strlen(temp);
  if (temp[len] == '\n')
    temp[len] = 0;

  CoolConsole &con = DAGORED2->getConsole();
  con.addMessage(ILogWriter::WARNING, "Script: %s", temp);
}

static const char *skipBom(const char *in)
{
  if (in && (strncmp(in, "\xef\xbb\xbf", 3) == 0))
    return in + 3;
  return in;
}

bool HmapLandPlugin::compileAndRunColorGenScript(bool &was_inited, SQPRINTFUNCTION &old_print_func, SQPRINTFUNCTION &old_err_func)
{
  if (!heightMap.isFileOpened())
    return false;

  was_inited = SquirrelVM::IsInitialized();

  if (!was_inited)
    SquirrelVM::Init();

  old_print_func = sq_getprintfunc(SquirrelVM::GetVMPtr());
  old_err_func = sq_geterrorfunc(SquirrelVM::GetVMPtr());
  sq_setprintfunc(SquirrelVM::GetVMPtr(), ::scriptPrintFunc, ::scriptPrintFunc);

  try
  {
    LandColorGenData::registerInScript(SquirrelVM::GetVMPtr());

    String scriptFilename(colorGenScriptFilename);
    file_ptr_t f = df_open(scriptFilename, DF_READ | DF_IGNORE_MISSING);
    if (!f)
    {
      scriptFilename.append(".nut");
      f = df_open(scriptFilename, DF_READ | DF_IGNORE_MISSING);
    }

    if (!f)
    {
      logerr_ctx("failed to open script file %s", scriptFilename.str());
      return false;
    }

    Tab<char> sqBuf;
    sqBuf.resize(df_length(f) + 1);
    df_read(f, sqBuf.data(), data_size(sqBuf) - 1);
    sqBuf.back() = '\0';

    SquirrelObject sqInst, ret;
    if (!SquirrelVM::CompileBuffer(skipBom(sqBuf.data()), sqInst, nullptr, scriptFilename))
      throw SquirrelError();

    if (!SquirrelVM::RunScript(sqInst, ret))
      throw SquirrelError();
  }
  catch (SquirrelError e)
  {
    CoolConsole &con = DAGORED2->getConsole();
    con.addMessage(ILogWriter::ERROR, "Script error: %s", e.desc);
    return false;
  }

  return true;
}


bool HmapLandPlugin::createDefaultScript(const char *path) const
{
  file_ptr_t f = ::df_open(path, DF_WRITE | DF_CREATE);
  if (f)
  {
    ::df_write(f, ::defScript, (int)strlen(::defScript));
    ::df_close(f);
    return true;
  }

  return false;
}


void HmapLandPlugin::closeColorGenScript(bool &was_inited, SQPRINTFUNCTION &old_print_func, SQPRINTFUNCTION &old_err_func)
{
  sq_setprintfunc(SquirrelVM::GetVMPtr(), old_print_func, old_err_func);

  if (was_inited)
    SquirrelVM::Shutdown();
}


bool HmapLandPlugin::getColorGenVarsFromScript()
{
  if (!heightMap.isFileOpened())
    return false;

  if (colorGenScriptFilename.empty() && genLayers.size())
    return true; // allow gen-layers only setup
  if (!usesGenScript())
  {
    DAEDITOR3.conWarning("Script not used");
    return true;
  }

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  con.addMessage(ILogWriter::NOTE, "getting parameters from generator script...");

  // save old tuned values first
  for (int i = 0; i < colorGenParams.size(); ++i)
    colorGenParams[i]->save(*colorGenParamsData);

  SimpleString editedImageName;
  if (editedScriptImage)
  {
    editedImageName = editedScriptImage->paramName;
    editedScriptImage = NULL;
  }

  detailedLandMask = NULL;
  colorGenParams.clear();

  bool wasSqInited;
  SQPRINTFUNCTION oldPrintFunc, oldErrFunc;

  if (!compileAndRunColorGenScript(wasSqInited, oldPrintFunc, oldErrFunc))
  {
    con.addMessage(ILogWriter::ERROR, "Error running generator script");
    return false;
  }

  try
  {
    SqPlus::SquirrelFunction<SquirrelObject> func(_T("getParamsList"));

    if (!func.func.IsNull())
    {
      SquirrelObject list = func();
      int len = list.Len();

      con.addMessage(ILogWriter::NOTE, "%d script params in the list", len);

      const SQChar *defName = _T("value");
      const SQChar *layersName = _T("layers");

      for (int pi = 0; pi < len; ++pi)
      {
        SquirrelObject param = list.GetValue(pi);

        const char *name = param.GetString(_T("name"));
        if (!name)
        {
          con.addMessage(ILogWriter::WARNING, "param #%d has no name - skipped", pi);
          continue;
        }

        const char *typeName = param.GetString(_T("type"));
        if (!typeName)
        {
          con.addMessage(ILogWriter::WARNING, "param #%d has no type - skipped", pi);
          continue;
        }

        if (stricmp(typeName, "int") == 0 || stricmp(typeName, "integer") == 0)
        {
          int val = 0;
          if (param.Exists(defName))
            val = param.GetInt(defName);
          colorGenParams.push_back(new ScriptParamInt(name, val));
        }
        else if (stricmp(typeName, "real") == 0 || stricmp(typeName, "float") == 0)
        {
          real val = 0;
          if (param.Exists(defName))
            val = param.GetFloat(defName);
          colorGenParams.push_back(new ScriptParamReal(name, val));
        }
        else if (stricmp(typeName, "color") == 0)
        {
          E3DCOLOR val = E3DCOLOR(255, 255, 255, 0);
          if (param.Exists(defName))
            val = param.GetInt(defName);
          colorGenParams.push_back(new ScriptParamColor(name, val));
        }
        else if (stricmp(typeName, "image") == 0)
        {
          ScriptParamImage::DefParams def;
          if (param.Exists(defName))
          {
            SquirrelObject defValues = param.GetValue(defName);

            if (defValues.GetType() == OT_TABLE)
            {
              if (defValues.Exists("imageName"))
                def.imageName = defValues.GetString("imageName");
              if (defValues.Exists("mappingType"))
                def.mappingType = defValues.GetInt("mappingType");
              if (defValues.Exists("detailType"))
                def.detailType = defValues.GetInt("detailType");
              if (defValues.Exists("tileX"))
                def.tile.x = defValues.GetFloat("tileX");
              if (defValues.Exists("tileY"))
                def.tile.y = defValues.GetFloat("tileY");
              if (defValues.Exists("offsetX"))
                def.offset.x = defValues.GetFloat("offsetX");
              if (defValues.Exists("offsetY"))
                def.offset.y = defValues.GetFloat("offsetY");
              if (defValues.Exists("clampU"))
                def.clampU = defValues.GetBool("clampU");
              if (defValues.Exists("clampV"))
                def.clampV = defValues.GetBool("clampV");
              if (defValues.Exists("flipU"))
                def.flipU = defValues.GetBool("flipU");
              if (defValues.Exists("flipV"))
                def.flipV = defValues.GetBool("flipV");
              if (defValues.Exists("swapUV"))
                def.swapUV = defValues.GetBool("swapUV");
            }
            else if (defValues.GetType() == OT_STRING)
              def.imageName = defValues.ToString();
          }
          colorGenParams.push_back(new ScriptParamImage(name, def));
        }
        else if (stricmp(typeName, "mask1") == 0 || stricmp(typeName, "mask8") == 0)
        {
          int bpp = stricmp(typeName, "mask1") == 0 ? 1 : 8;
          int div = 0;
          bool det = false;
          if (strcmp(name, "$detailedLandMask") == 0)
          {
            if (param.Exists(_T("step")))
              div = getGridCellSize() / param.GetFloat(_T("step"));
            else if (param.Exists(_T("divisor")))
              div = param.GetFloat(_T("divisor"));
          }
          else if (detailedLandMask && param.Exists(_T("detailed")))
            det = param.GetBool(_T("detailed"));

          if (div > 1)
            colorGenParams.push_back(detailedLandMask = new ScriptParamMask(name, bpp, true, div));
          else
          {
            div = detailedLandMask ? static_cast<ScriptParamMask *>(detailedLandMask.get())->divisor : 1;
            colorGenParams.push_back(new ScriptParamMask(name, bpp, det, div));
          }
        }
        else if (stricmp(typeName, "grid") == 0)
        {
          con.addMessage(ILogWriter::ERROR, "obsolete grid-type parameter '%s'", name);
        }
        else if (stricmp(typeName, "landClass") == 0)
        {
          ScriptParamLandClass::DefParams def;
          clear_and_resize(def.layerIdx, landClsLayer.size());
          mem_set_ff(def.layerIdx);
          int lcs_attr_id = hmlService->getBitLayerAttrId(getLayersHandle(), "landClassSlot");

          if (param.Exists(layersName))
          {
            SquirrelObject defValues = param.GetValue(layersName);

            if (defValues.GetType() == OT_TABLE && defValues.BeginIteration())
            {
              SquirrelObject key, value;
              while (defValues.Next(key, value))
              {
                int id = hmlService->getBitLayerIndexByName(getLayersHandle(), key.ToString());
                if (id < 0)
                {
                  con.addMessage(ILogWriter::ERROR, "Unknown layer '%s' in parameter '%s'", key.ToString(), name);
                  continue;
                }

                if (id == getDetLayerIdx())
                  def.detailIdx = value.ToInteger();

                if (hmlService->testBitLayerAttr(getLayersHandle(), id, lcs_attr_id))
                  def.layerIdx[id] = value.ToInteger();
              }
              defValues.EndIteration();
            }
          }

          if (param.Exists(defName))
          {
            SquirrelObject defValues = param.GetValue(defName);

            if (defValues.GetType() == OT_TABLE)
            {
              if (defValues.Exists("asset"))
                def.asset = defValues.GetString("asset");
            }
            else if (defValues.GetType() == OT_STRING)
              def.asset = defValues.ToString();
          }

          colorGenParams.push_back(new ScriptParamLandClass(name, def));
        }
        else if (stricmp(typeName, "detailTex") == 0)
        {
          if (!param.Exists("slot"))
          {
            con.addMessage(ILogWriter::ERROR, "parameter '%s' type '%s' must have 'slot' property", name, typeName);
            continue;
          }

          ScriptParamDetailTex::DefParams def;
          def.detailSlot = param.GetInt("slot");
          if (param.Exists(defName))
          {
            SquirrelObject defValues = param.GetValue(defName);

            if (defValues.GetType() == OT_TABLE)
            {
              if (defValues.Exists("asset"))
                def.blkName = defValues.GetString("asset");
            }
            else if (defValues.GetType() == OT_STRING)
              def.blkName = defValues.ToString();
          }
          colorGenParams.push_back(new ScriptParamDetailTex(name, def));
        }
        else
        {
          con.addMessage(ILogWriter::ERROR, "Unknown parameter '%s' type '%s'", name, typeName);
        }
      }
    }
  }
  catch (SquirrelError e)
  {
    con.addMessage(ILogWriter::ERROR, "Script error: %s", e.desc);
  }

  closeColorGenScript(wasSqInited, oldPrintFunc, oldErrFunc);

  ScriptParam *imageParam = NULL;

  for (int i = 0; i < colorGenParams.size(); ++i)
  {
    colorGenParams[i]->load(*colorGenParamsData);

    if (strcmp(colorGenParams[i]->paramName, editedImageName) == 0)
      imageParam = colorGenParams[i];
  }

  editScriptImage(imageParam);
  con.endLog();

  return true;
}


void HmapLandPlugin::generateLandColors(const IBBox2 *in_rect, bool finished)
{
  if (!heightMap.isFileOpened())
    return;

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  updateScriptImageList();

  bool wasSqInited;
  SQPRINTFUNCTION oldPrintFunc, oldErrFunc;
  bool use_script = usesGenScript();

  if (!use_script && genLayers.size())
    ; // allow gen-layers only setup
  else if (!compileAndRunColorGenScript(wasSqInited, oldPrintFunc, oldErrFunc))
  {
    con.addMessage(ILogWriter::ERROR, "Error running generator script");
    return;
  }

  if (!in_rect)
  {
    if (hasColorTex)
      colorMap.reset(heightMap.getMapSizeX(), heightMap.getMapSizeY(), E3DCOLOR(255, 10, 255, 0));
    createColormapFile(con);
    resizeLandClassMapFile(DAGORED2->getConsole());
  }

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
  int samples_processed = 0;
  LandColorGenData::blendedInvalidLandClasses = false;
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
  int phys_li_max = phys_li < 0 ? 0 : (1 << hmlService->getBitLayersList(getLayersHandle())[phys_li].bitCount) - 1;
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
    }

  bool layers_use_curv = false;
  for (int l = 0; l < genLayers.size(); l++)
    if (static_cast<PostScriptParamLandLayer *>(genLayers[l].get())->enabled &&
        static_cast<PostScriptParamLandLayer *>(genLayers[l].get())->curv.conv != PostScriptParamLandLayer::WtRange::WMT_one)
    {
      layers_use_curv = true;
      break;
    }

  try
  {
    SqPlus::SquirrelFunction<unsigned> genFunc;
    SquirrelObject layers;

    if (use_script)
    {
      for (int i = 0; i < colorGenParams.size(); ++i)
        colorGenParams[i]->save(*colorGenParamsData);

      HSQUIRRELVM vm = SquirrelVM::GetVMPtr();
      for (int i = 0; i < colorGenParams.size(); ++i)
        colorGenParams[i]->setToScript(vm);
      for (int i = 0; i < landClsLayer.size(); i++)
      {
        sq_pushroottable(vm);
        sq_pushstring(vm, String(32, "_%s", hmlService->getBitLayerName(landClsLayersHandle, i)), -1);
        sq_pushinteger(vm, i);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
      }

      SqPlus::SquirrelFunction<SquirrelObject> getGenFunc(_T("getGenerator"));
      genFunc.v = SquirrelVM::GetVMPtr();
      genFunc.object = SquirrelVM::GetRootTable();
      genFunc.func = getGenFunc();
      layers = SquirrelVM::CreateArray(landClsLayer.size());
    }

    int x0 = 0, x1 = heightMap.getMapSizeX();
    int y0 = 0, y1 = heightMap.getMapSizeY();

    if (in_rect)
    {
      x0 = in_rect->lim[0].x;
      x1 = in_rect->lim[1].x;
      y0 = in_rect->lim[0].y + 1;
      y1 = in_rect->lim[1].y + 1;
    }

    for (int y = y0, cnt = 0; y < y1; ++y)
    {
      if (--cnt <= 0)
        cnt = heightMap.getElemSize();

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

            if (use_script)
            {
              unsigned res = genFunc(layers) & 0x00FFFFFF;
              if (hasColorTex && !lx && !ly)
                colorMap.setData(x, y, res);

              if (!LandColorGenData::blendTex.size())
                detIdx = layers.GetInt(detLayerIdx);

              for (int l = 0; l < landClsLayer.size(); l++)
              {
                if (l == detLayerIdx)
                  continue;
                layerVal[l] = layers.GetInt(l);
                layers.SetValue(l, 0);
              }
            }

            float curv = layers_use_curv ? LandColorGenData::getCurvature() : 0;
            bool in_det = detDivisor ? insideDetRectC(x * detDivisor, y * detDivisor) : false;
            int physSplattingLc = 0;
            double maxPhysWeight = -1.0;
            for (int l = 0; l < genLayers.size(); l++)
            {
              PostScriptParamLandLayer &gl = *static_cast<PostScriptParamLandLayer *>(genLayers[l].get());
              if (!gl.enabled)
                continue;
              if (gl.areaSelect == gl.AREA_main && in_det)
                continue;
              if (gl.areaSelect == gl.AREA_det && !in_det)
                continue;

              double wt = gl.calcWeight(h, angDiff, curv);
              if (!lx && !ly && gl.writeDetTex && !gl.badDetTex)
                LandColorGenData::blendDetTex(gl.detIdx, wt);
              if (gl.writeLand1 && wt > gl.writeLand1Thres && lc1_li >= 0)
                layerVal[lc1_li] = lc1_base - gl.landIdx;
              if (gl.writeLand2 && wt > gl.writeLand2Thres && lc2_li >= 0)
                layerVal[lc2_li] = lc2_base - gl.landIdx;
              if (gl.writeImportance && impLayerIdx >= 0)
                layerVal[impLayerIdx] = wt * imp_scale;
              if (gl.writeDetTex && (wt >= maxPhysWeight) && (gl.landIdx < phys_li_max)) // physmat "splatting" (choose lc with biggest
                                                                                         // weight)
              {
                maxPhysWeight = wt;
                physSplattingLc = gl.landIdx;
              }
            }
            if (phys_li >= 0)
              layerVal[phys_li] = physSplattingLc;

            if (!lx && !ly)
              LandColorGenData::applyBlendDetTex(x, y, detIdx);

            unsigned w = 0;
            if (detLayerIdx >= 0)
              w = landClsLayer[detLayerIdx].setLayerData(w,
                (getDetTexIdxMap() && LandColorGenData::blendTex.size()) ? LandColorGenData::blendTex[0].idx : detIdx);

            for (int l = 0; l < landClsLayer.size(); l++)
              if (l != detLayerIdx)
                w = landClsLayer[l].setLayerData(w, layerVal[l]);

            samples_processed++;
            landClsMap.setData(x * lcmScale + lx, y * lcmScale + ly, w);
          }
      }

      if (!in_rect && cnt == heightMap.getElemSize())
      {
        if (hasColorTex && !colorMap.flushData())
          con.addMessage(ILogWriter::ERROR, "Error writing data to color map file");
        if (!landClsMap.flushData())
          con.addMessage(ILogWriter::ERROR, "Error writing data to land-class map file");

        heightMap.unloadUnchangedData(y + 1);
        landClsMap.unloadUnchangedData(y * lcmScale + 1);
        if (hasColorTex)
          colorMap.unloadUnchangedData(y + 1);
      }

      if (!in_rect)
        con.incDone();
    }
  }
  catch (SquirrelError e)
  {
    con.addMessage(ILogWriter::ERROR, "Script error: %s", e.desc);
  }
  if (LandColorGenData::blendedInvalidLandClasses)
    con.addMessage(ILogWriter::ERROR, "Some of empty land classes were blend as textures!");

  if (!in_rect)
  {
    heightMap.unloadAllUnchangedData();

    if (!landClsMap.flushData())
      con.addMessage(ILogWriter::ERROR, "Error writing data to land-class map file");
    if (hasColorTex && !colorMap.flushData())
      con.addMessage(ILogWriter::ERROR, "Error writing data to color map file");
    landClsMap.unloadAllUnchangedData();
    if (hasColorTex)
      colorMap.unloadAllUnchangedData();
  }

  if (use_script)
  {
    SqPlus::SquirrelFunction<SquirrelObject> endGenerator(_T("endGenerator"));
    if (!endGenerator.func.IsNull())
    {
      SqPlus::SquirrelFunction<SquirrelObject> endGeneratorFunc(SquirrelVM::GetRootTable(), endGenerator());
      if (!endGeneratorFunc.func.IsNull())
        endGeneratorFunc();
    }

    closeColorGenScript(wasSqInited, oldPrintFunc, oldErrFunc);
  }

  if (!in_rect)
    con.endProgress();

  int genTime = ::get_time_msec() - time0;
  con.addMessage(ILogWriter::NOTE, "generated in %d msec (%d samples)", genTime, samples_processed);

  con.endLog();
  if (in_rect)
    onLandRegionChanged((*in_rect)[0].x * lcmScale, (*in_rect)[0].y * lcmScale, (*in_rect)[1].x * lcmScale + 1,
      (*in_rect)[1].y * lcmScale + 1, finished);
  else
    onLandRegionChanged(0, 0, landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), finished);
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
  bool show = (editedScriptImage && showEditedMask()) && showBlueWhiteMask;
  if (show)
  {
    Color4 world_to_bw_mask;
    world_to_bw_mask.r = 1.f / (esiGridW * esiGridStep);
    world_to_bw_mask.g = 1.f / (esiGridH * esiGridStep);
    world_to_bw_mask.b = (-esiOrigin.x + 0.5) * world_to_bw_mask.r;
    world_to_bw_mask.a = (-esiOrigin.y + 0.5) * world_to_bw_mask.g;

    dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("world_to_bw_mask"), world_to_bw_mask);
  }
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
    unsigned texLockFlags = TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_RWMASK | TEXLOCK_SYSTEXLOCK;
    if (tex->lockimg((void **)&imgPtr, stride, 0, texLockFlags))
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

void HmapLandPlugin::updateBlueWhiteMask(const IBBox2 *rect)
{
  static int blueWhiteMaskVarId = dagGeom->getShaderVariableId("blue_white_mask_tex");
  if (!(editedScriptImage && showEditedMask()))
  {
    setShowBlueWhiteMask();
    updateGenerationMask(rect);
    hmlService->invalidateClipmap(false);
    return;
  }
  if (bluewhiteTex)
  {
    TextureInfo ti;
    bluewhiteTex->getinfo(ti, 0);
    if (ti.w != esiGridW || ti.h != esiGridH)
    {
      dagGeom->shaderGlobalSetTexture(blueWhiteMaskVarId, BAD_TEXTUREID);
      dagRender->releaseManagedTexVerified(bluewhiteTexId, bluewhiteTex);
    }
  }
  if (!bluewhiteTex)
  {
    bluewhiteTex = d3d::create_tex(NULL, esiGridW, esiGridH, TEXFMT_L8 | TEXCF_READABLE | TEXCF_DYNAMIC, 1, "blueWhite");
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

  // fill color texture
  unsigned texLockFlags = TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_RWMASK | TEXLOCK_SYSTEXLOCK;
  if (bluewhiteTex->lockimg((void **)&imgPtr, stride, 0, texLockFlags))
  {
    if (stride < x1 - x0 + 1)
    {
      logerr("invalid update bluewhite");
      bluewhiteTex->unlockimg();
      return;
    }

    imgPtr += stride * y0 + x0;
    int use_stride = stride - (x1 - x0 + 1);
    for (int y = y0; y <= y1; ++y, imgPtr += use_stride)
    {
      for (int x = x0; x <= x1; ++x, ++imgPtr)
        *imgPtr = real2uchar(getBrushImageData(x, y, getEditedChannel()));
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
