// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"
#include <oldEditor/de_cm.h>
#include <oldEditor/de_workspace.h>

#include <propPanel/commonWindow/dialogWindow.h>

#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <de3_editorEvents.h>
#include <de3_dynRenderService.h>
#include <de3_skiesService.h>
#include <de3_interface.h>
#include <de3_windService.h>
#include <de3_hmapService.h>

#include <EditorCore/ec_modelessDialogWindowController.h>
#include <EditorCore/ec_workspace.h>
#include <EditorCore/ec_confirmation_dialog.h>
#include <libTools/util/strUtil.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderDbg.h>
#include <debug/dag_debug.h>

#include <workCycle/dag_workCycle.h>
#include <drv/3d/dag_driver.h>
#include <textureUtil/textureUtil.h>
#include <assets/asset.h>
#include <drv/3d/dag_lock.h>

static Tab<String> shadowQualityStr(inimem);

static inline void updateLight(bool update_var_before = false)
{
  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  if (update_var_before)
    ltSrv->updateShaderVars();

  if (ltSrv->updateDirLightFromSunSky())
    ltSrv->updateShaderVars();

  ltSrv->applyLightingToGeometry();

  DAGORED2->spawnEvent(HUID_SunPropertyChanged, NULL);
}


namespace environment
{

class EnvSetDlg;

enum
{
  PID_SUN_GROUP,

  PID_SUN_AZIMUT,
  PID_SUN_ZENITH,
  PID_SUN_ANGSIZE,
  PID_SUN_COLOR,
  PID_SUN_BRIGHTNESS,

  PID_AMB_COLOR,
  PID_AMB_BRIGHTNESS,

  PID_REVERT_SUB_AMB_BUTTON,

  PID_RENDER_GROUP,
  PID_RENDER_TYPE,
  PID_RENDER_OPT,
  PID_RENDER_OPT_LAST = PID_RENDER_OPT + eastl::to_underlying(IDynRenderService::ROPT_COUNT),
  PID_GAMEOBJ_VISIBLE,
  PID_SHADOW_QUALITY,
  PID_EXPOSURE,
  PID_REVERT_RENDER_BUTTON,

  PID_LTDBG_GROUP,
  PID_LTDBG_NMAP,
  PID_LTDBG_DTEX,
  PID_LTDBG_LT,
  PID_LTDBG_CHROME,

  PID_DBGSHOW_GRP,
  PID_DBGSHOW_MODE,

  PID_ENV_BLK,
  PID_PAINT_DETAILS_TEXTURE,

  PID_WIND_GROUP,
  PID_WIND_ENABLED,
  PID_WIND_DIR,
  PID_WIND_STRENGTH,
  PID_WIND_NOISE_STRENGTH,
  PID_WIND_NOISE_SPEED,
  PID_WIND_NOISE_SCALE,
  PID_WIND_NOISE_PERPENDICULAR,
  PID_WIND_LEVEL,
  PID_REVERT_WIND_BUTTON,
};

static bool supportRenderDebug = false;
static int rdbgEnabledGvid = -1;
static int rdbgTypeGvid = -1, rdbgUseNmapGvid = -1, rdbgUseDtexGvid = -1, rdbgUseLtGvid = -1;
static int rdbgUseChromeGvid = -1;
static SimpleString envBlkFn;
static SimpleString paintDetailsTexAsset; // per level painting texture name
static BaseTexture *combinedPaintTex = NULL;
static TEXTUREID combinedPaintTexId = BAD_TEXTUREID;
static IWindService::PreviewSettings windPreviewSettings;

static int rdbgType = 0;
static bool rdbgUseNmap = true, rdbgUseDtex = true, rdbgUseLt = true;
static bool rdbgUseChrome = true;

static DataBlock de_ui_state;

static void detectRenderDebug()
{
  rdbgEnabledGvid = get_shader_glob_var_id("rdbgEnabled", true);
  rdbgTypeGvid = get_shader_glob_var_id("rdbgType", true);
  rdbgUseNmapGvid = get_shader_glob_var_id("rdbgUseNmap", true);
  rdbgUseDtexGvid = get_shader_glob_var_id("rdbgUseDtex", true);
  rdbgUseLtGvid = get_shader_glob_var_id("rdbgUseLt", true);
  rdbgUseChromeGvid = get_shader_glob_var_id("rdbgUseChrome", true);
  supportRenderDebug = VariableMap::isGlobVariablePresent(rdbgEnabledGvid);
}

static void applyRenderDebug()
{
  int en = rdbgType > 0 || !rdbgUseNmap || !rdbgUseDtex || !rdbgUseLt || !rdbgUseChrome;
  ShaderGlobal::set_int_fast(rdbgEnabledGvid, en);
  ShaderGlobal::set_int_fast(rdbgTypeGvid, rdbgType);
  ShaderGlobal::set_int_fast(rdbgUseNmapGvid, rdbgUseNmap);
  ShaderGlobal::set_int_fast(rdbgUseDtexGvid, rdbgUseDtex);
  ShaderGlobal::set_int_fast(rdbgUseLtGvid, rdbgUseLt);
  ShaderGlobal::set_int_fast(rdbgUseChromeGvid, rdbgUseChrome);
  enable_diffuse_mipmaps_debug(rdbgType == -1);
}

static void apply_envi_snapshot()
{
  d3d::GpuAutoLock gpuLock;
  if (envBlkFn.empty())
  {
    EDITORCORE->queryEditorInterface<IDynRenderService>()->setEnvironmentSnapshot(NULL, false);
    if (ISkiesService *skiesSrv = EDITORCORE->queryEditorInterface<ISkiesService>())
      skiesSrv->setWeather(NULL, NULL, NULL);
  }
  else
  {
    String loc_path;
    if (envBlkFn[0] && envBlkFn[1] != ':')
      loc_path.printf(512, "%s%s", DAGORED2->getWorkspace().getAppDir(), envBlkFn);
    else
      loc_path = envBlkFn;
    EDITORCORE->queryEditorInterface<IDynRenderService>()->setEnvironmentSnapshot(loc_path, !envBlkFn.empty());
  }
}

static void set_paint_detail_texture()
{
  static int paintDetailsVarId = get_shader_variable_id("paint_details_tex", true);
  TEXTUREID localPaintColorsTexId, globalPaintColorsTexId;
  IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
  SimpleString globalTexPalette;

  globalPaintColorsTexId = get_managed_texture_id("assets_color_global_tex_palette*");
  if (hmlService)
    if (SharedTex globalPaintTex = dag::get_tex_gameres(hmlService->getGlobalPaintDetailsTexName()))
      globalPaintColorsTexId = globalPaintTex.getTexId();

  if (!paintDetailsTexAsset.empty())
    localPaintColorsTexId = get_managed_texture_id(String(0, "%s*", paintDetailsTexAsset));
  else
    localPaintColorsTexId = globalPaintColorsTexId;

  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(combinedPaintTexId, combinedPaintTex);
  Tab<TEXTUREID> tex_ids;
  tex_ids.push_back(globalPaintColorsTexId);
  tex_ids.push_back(localPaintColorsTexId);
  acquire_managed_tex(globalPaintColorsTexId);
  acquire_managed_tex(localPaintColorsTexId);
  prefetch_and_wait_managed_textures_loaded(tex_ids, true);
  combinedPaintTex =
    texture_util::stitch_textures_horizontal(tex_ids, "combined_paint_tex", TEXFMT_DEFAULT | TEXCF_SRGBREAD).release();
  if (combinedPaintTex)
  {
    combinedPaintTexId = register_managed_tex("paint_details_tex", combinedPaintTex);
    TextureInfo texInfo;
    combinedPaintTex->getinfo(texInfo);
    ShaderGlobal::set_float(get_shader_variable_id("paint_details_tex_inv_h", true), safediv(1.f, (float)texInfo.h));
  }
  else if (VariableMap::isGlobVariablePresent(paintDetailsVarId))
    DAEDITOR3.conError("failed to create combined painting texture (globTid=0x%x(%s) localTid=0x%x paintDetailsTexAsset='%s' var=%d)",
      globalPaintColorsTexId, get_managed_texture_name(globalPaintColorsTexId), localPaintColorsTexId, paintDetailsTexAsset,
      paintDetailsVarId);
  release_managed_tex(globalPaintColorsTexId);
  release_managed_tex(localPaintColorsTexId);
  ShaderGlobal::set_texture(paintDetailsVarId, combinedPaintTexId);
  d3d::SamplerInfo smpInfo;
  smpInfo.filter_mode = d3d::FilterMode::Point;
  ShaderGlobal::set_sampler(get_shader_variable_id("paint_details_tex_samplerstate", true), d3d::request_sampler(smpInfo));
}

class EnvSetDlg : public PropPanel::DialogWindow
{
public:
  EnvSetDlg(void *phandle, SunLightProps *sun_props) :
    DialogWindow(phandle, hdpi::_pxScaled(300), hdpi::_pxScaled(300), "Environment Settings"), sunProps(sun_props)
  {
    IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
    PropPanel::ContainerPropertyControl *_panel = getPanel();
    G_ASSERT(_panel && "No panel in EnvSetDlg");

    G_ASSERT(sun_props && "Sun properties is NULL in EnvSetDlg");

    PropPanel::ContainerPropertyControl *_settings = _panel->createGroup(PID_SUN_GROUP, "Environment settings");
    {
      _settings->createTrackFloat(PID_SUN_AZIMUT, "sun azimut", RadToDeg(sunProps->azimuth), -180.f, 180.f, 0.1);
      _settings->setDefaultValueById(PID_SUN_AZIMUT, SunLightProps::DEFAULT_AZIMUTH);

      _settings->createTrackFloat(PID_SUN_ZENITH, "sun zenith", RadToDeg(sunProps->zenith), -5.f, 90.f, 0.1);
      _settings->setDefaultValueById(PID_SUN_ZENITH, SunLightProps::DEFAULT_ZENITH);

      _settings->createEditFloat(PID_SUN_ANGSIZE, "sun angSize", sunProps->angSize, 3);
      _settings->setDefaultValueById(PID_SUN_ANGSIZE, SunLightProps::DEFAULT_ANG_SIZE);

      _settings->createEditFloat(PID_SUN_BRIGHTNESS, "sun integral luminocity", sunProps->calcLuminocity(), 3);
      _settings->setDefaultValueById(PID_SUN_BRIGHTNESS,
        SunLightProps::calcLuminocity(SunLightProps::DEFAULT_FIRST_SUN_BRIGTHNESS, sunProps->angSize));

      _settings->createSimpleColor(PID_SUN_COLOR, "sun color", sunProps->color);
      _settings->setDefaultValueById(PID_SUN_COLOR, SunLightProps::DEFAULT_COLOR);

      ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
      SkyLightProps &sky = ltSrv->getSky();
      _settings->createSimpleColor(PID_AMB_COLOR, "ambient color", sky.color);
      _settings->setDefaultValueById(PID_AMB_COLOR, SkyLightProps::DEFAULT_COLOR);

      _settings->createEditFloat(PID_AMB_BRIGHTNESS, "ambient brightness", sky.brightness, 2);
      _settings->setDefaultValueById(PID_AMB_BRIGHTNESS, SkyLightProps::DEFAULT_BRIGHTNESS);

      _settings->createButton(PID_REVERT_SUB_AMB_BUTTON, "Revert to default");

      _settings->setBoolValue(true);
    }

    addAmbientWindPanel(_panel);

    PropPanel::ContainerPropertyControl *_dbgShow =
      (drSrv && drSrv->getDebugShowTypeNames().size() > 1) ? _panel->createGroup(PID_DBGSHOW_GRP, "Debug settings") : NULL;
    if (_dbgShow)
    {
      dag::ConstSpan<const char *> modes = drSrv->getDebugShowTypeNames();
      PropPanel::ContainerPropertyControl *_dbgShowRG = _dbgShow->createRadioGroup(PID_DBGSHOW_MODE, "Debug show mode");
      for (int i = 0; i < modes.size(); i++)
        _dbgShowRG->createRadio(i, modes[i]);
      _dbgShow->setInt(PID_DBGSHOW_MODE, drSrv->getDebugShowType());
    }

    if (supportRenderDebug)
    {
      PropPanel::ContainerPropertyControl &ltDbgGrp = *_panel->createGroup(PID_SUN_GROUP, "Render debug");
      PropPanel::ContainerPropertyControl &ltGrp = *ltDbgGrp.createRadioGroup(PID_LTDBG_GROUP, "Debug mode");
      ltGrp.createRadio(0, "Final render (no debug)");
      ltGrp.createRadio(-1, "Debug mipmaps (mips 0..5=RGBCMY)");
      if (VariableMap::isGlobVariablePresent(rdbgTypeGvid))
      {
        ltGrp.createRadio(1, "Debug normal maps (gray diffuse with ligthing)");
        ltGrp.createRadio(2, "Debug specular (black diffuse with lighting)");
        ltGrp.createRadio(3, "Debug lighting type (ltmap=G vltmap=R dynamic=B)");
        ltGrp.createRadio(4, "Debug texel size (0.1=black, 1.0=gray 10=white)");
        ltGrp.createRadio(5, "Debug specular tex (use specular tex as diffuse)");
      }
      if (VariableMap::isGlobVariablePresent(rdbgUseNmapGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_NMAP, "Enable normal maps", rdbgUseNmap);
      if (VariableMap::isGlobVariablePresent(rdbgUseDtexGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_DTEX, "Enable detail tex", rdbgUseDtex);
      if (VariableMap::isGlobVariablePresent(rdbgUseLtGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_LT, "Enable lighting", rdbgUseLt);
      if (VariableMap::isGlobVariablePresent(rdbgUseChromeGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_CHROME, "Enable chrome (envi reflection)", rdbgUseChrome);
      ltDbgGrp.setInt(PID_LTDBG_GROUP, rdbgType);
    }

    PropPanel::ContainerPropertyControl *_render_grp = _panel->createGroup(PID_RENDER_GROUP, "Render settings");
    {
      static const char *rtypeName[] = {"classic", "dynamic-A", "deferred"};
      dag::ConstSpan<int> rtypes = drSrv->getSupportedRenderTypes();
      Tab<String> rtypeStr;
      rtypeStr.resize(rtypes.size());
      for (int i = 0; i < rtypes.size(); i++)
        rtypeStr[i] = rtypeName[rtypes[i]];

      _render_grp->createCombo(PID_RENDER_TYPE, "render type", rtypeStr, find_value_idx(rtypes, drSrv->getRenderType()));

      if (auto *gameObjSrv = IDaEditor3Engine::get().findService("_goEntMgr"))
      {
        _render_grp->createCheckBox(PID_GAMEOBJ_VISIBLE, "gameObj previews", gameObjSrv->getServiceVisible());
        _render_grp->setDefaultValueById(PID_GAMEOBJ_VISIBLE, true);
      }

      for (int i = 0; i < drSrv->ROPT_COUNT; i++)
        if (drSrv->getRenderOptSupported(i))
          _render_grp->createCheckBox(PID_RENDER_OPT + i, drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i));
      if (drSrv->getShadowQualityNames().size() > 0 && drSrv->getShadowQualityNames()[0])
      {
        shadowQualityStr.resize(drSrv->getShadowQualityNames().size());
        for (int i = 0; i < shadowQualityStr.size(); i++)
          shadowQualityStr[i] = drSrv->getShadowQualityNames()[i];
        _render_grp->createCombo(PID_SHADOW_QUALITY, "shadow quality", shadowQualityStr, drSrv->getShadowQuality());
        _render_grp->setDefaultValueById(PID_SHADOW_QUALITY, 0);
      }
      if (drSrv->hasExposure())
      {
        _render_grp->createTrackFloatLogarithmic(PID_EXPOSURE, "exposure (0-32 logarithmic)", drSrv->getExposure(), 0.0f, 32.0f, 0.0f,
          sqrt(32.0f));
        _render_grp->setDefaultValueById(PID_EXPOSURE, 1.0f);
      }

      rtypeStr.clear();
      rtypeStr.push_back() = "Environment BLK files|envi.blk";
      _render_grp->createFileButton(PID_ENV_BLK, "envi snapshot settings");
      _render_grp->setStrings(PID_ENV_BLK, rtypeStr);
      _render_grp->setText(PID_ENV_BLK, envBlkFn);

      _render_grp->createStatic(-1, "paint details texture");
      _render_grp->createButton(PID_PAINT_DETAILS_TEXTURE,
        paintDetailsTexAsset.empty() ? "-- no palette tex --" : paintDetailsTexAsset.str());

      _render_grp->createButton(PID_REVERT_RENDER_BUTTON, "Revert to default");
    }
  }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    switch (pcb_id)
    {
      case PID_PAINT_DETAILS_TEXTURE:
        if (const char *asset = DAEDITOR3.selectAssetX(paintDetailsTexAsset, "Select palette texture", "tex", "palette", true))
        {
          paintDetailsTexAsset = asset;
          panel->setCaption(pcb_id, paintDetailsTexAsset.empty() ? "-- no palette tex --" : paintDetailsTexAsset);
          set_paint_detail_texture();
        }
        break;
      case PID_REVERT_SUB_AMB_BUTTON:
      case PID_REVERT_WIND_BUTTON:
      case PID_REVERT_RENDER_BUTTON: OnRevertToDfaultsButtonClicked(pcb_id); break;
    }

    updateRevertButtons();
  }

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
    switch (pcb_id)
    {
      case PID_ENV_BLK:
      {
        const char *appDir = DAGORED2->getWorkspace().getAppDir();
        String fn(panel->getText(pcb_id));

        if (fn.length())
        {
          envBlkFn = ::make_path_relative(fn, appDir);
          panel->setText(pcb_id, envBlkFn);
        }
        else
          envBlkFn = NULL;
        apply_envi_snapshot();
      }
      break;

      case PID_SUN_AZIMUT:
        sunProps->azimuth = DegToRad(panel->getFloat(pcb_id));
        updateLight();
        break;

      case PID_SUN_ZENITH:
        sunProps->zenith = DegToRad(panel->getFloat(pcb_id));
        updateLight();
        break;

      case PID_SUN_ANGSIZE:
        sunProps->angSize = panel->getFloat(pcb_id);
        getPanel()->setDefaultValueById(PID_SUN_BRIGHTNESS,
          SunLightProps::calcLuminocity(SunLightProps::DEFAULT_FIRST_SUN_BRIGTHNESS, sunProps->angSize));
        updateLight();
        break;

      case PID_SUN_BRIGHTNESS:
        sunProps->brightness = sunProps->calcBrightness(panel->getFloat(pcb_id));
        updateLight();
        break;

      case PID_SUN_COLOR:
        sunProps->color = panel->getColor(pcb_id);
        updateLight(true);
        break;

      case PID_AMB_COLOR:
      {
        ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
        SkyLightProps &sky = ltSrv->getSky();
        sky.color = panel->getColor(pcb_id);
        updateLight();
      }
      break;

      case PID_AMB_BRIGHTNESS:
      {
        ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
        SkyLightProps &sky = ltSrv->getSky();
        sky.brightness = panel->getFloat(pcb_id);
        updateLight();
      }
      break;

      case PID_WIND_NOISE_STRENGTH:
      case PID_WIND_NOISE_SPEED:
      case PID_WIND_NOISE_SCALE:
      case PID_WIND_NOISE_PERPENDICULAR:
      case PID_WIND_STRENGTH:
      case PID_WIND_DIR:
      case PID_WIND_ENABLED:
      {
        windPreviewSettings.ambient.windNoiseStrength = panel->getFloat(PID_WIND_NOISE_STRENGTH);
        windPreviewSettings.ambient.windNoiseSpeed = panel->getFloat(PID_WIND_NOISE_SPEED);
        windPreviewSettings.ambient.windNoiseScale = panel->getFloat(PID_WIND_NOISE_SCALE);
        windPreviewSettings.ambient.windNoisePerpendicular = panel->getFloat(PID_WIND_NOISE_PERPENDICULAR);
        windPreviewSettings.windStrength = panel->getFloat(PID_WIND_STRENGTH);
        windPreviewSettings.windAzimuth = panel->getFloat(PID_WIND_DIR);
        windPreviewSettings.enabled = panel->getBool(PID_WIND_ENABLED);

        if (pcb_id == PID_WIND_ENABLED)
          enableAmbientWindPanel(panel);

        IWindService *windSrv = EDITORCORE->queryEditorInterface<IWindService>();
        if (windSrv)
          windSrv->setPreview(windPreviewSettings);
      }
      break;
      case PID_WIND_LEVEL:
      {
        windPreviewSettings.levelPath = panel->getText(pcb_id);

        IWindService *windSrv = EDITORCORE->queryEditorInterface<IWindService>();
        if (windSrv)
          windSrv->setPreview(windPreviewSettings);
      }
      break;

      case PID_GAMEOBJ_VISIBLE:
      {
        if (auto *gameObjSrv = IDaEditor3Engine::get().findService("_goEntMgr"))
          gameObjSrv->setServiceVisible(panel->getBool(pcb_id));
        break;
      }

      case PID_SHADOW_QUALITY: drSrv->setShadowQuality(panel->getInt(pcb_id)); break;

      case PID_EXPOSURE: drSrv->setExposure(panel->getFloat(pcb_id)); break;

      case PID_RENDER_TYPE: drSrv->setRenderType(drSrv->getSupportedRenderTypes()[panel->getInt(pcb_id)]); break;

      case PID_DBGSHOW_MODE: drSrv->setDebugShowType(panel->getInt(pcb_id)); break;

      case PID_LTDBG_GROUP:
        rdbgType = panel->getInt(pcb_id);
        applyRenderDebug();
        break;

      case PID_LTDBG_NMAP:
        rdbgUseNmap = panel->getBool(pcb_id);
        applyRenderDebug();
        break;

      case PID_LTDBG_DTEX:
        rdbgUseDtex = panel->getBool(pcb_id);
        applyRenderDebug();
        break;

      case PID_LTDBG_LT:
        rdbgUseLt = panel->getBool(pcb_id);
        applyRenderDebug();
        break;

      case PID_LTDBG_CHROME:
        rdbgUseChrome = panel->getBool(pcb_id);
        applyRenderDebug();
        break;

      default:
        if (pcb_id >= PID_RENDER_OPT && pcb_id < PID_RENDER_OPT_LAST)
        {
          drSrv->setRenderOptEnabled(pcb_id - PID_RENDER_OPT, panel->getBool(pcb_id));
          if (drSrv->getRenderOptEnabled(pcb_id - PID_RENDER_OPT) != panel->getBool(pcb_id))
            panel->setBool(pcb_id, drSrv->getRenderOptEnabled(pcb_id - PID_RENDER_OPT));
        }
    }
    DAGORED2->repaint();

    updateRevertButtons();
  }

  void setAmbientWindPanel(PropPanel::ContainerPropertyControl *_panel)
  {
    _panel->setBool(PID_WIND_ENABLED, windPreviewSettings.enabled);
    _panel->setFloat(PID_WIND_DIR, windPreviewSettings.windAzimuth);
    _panel->setFloat(PID_WIND_STRENGTH, windPreviewSettings.windStrength);
    _panel->setFloat(PID_WIND_NOISE_STRENGTH, windPreviewSettings.ambient.windNoiseStrength);
    _panel->setFloat(PID_WIND_NOISE_SPEED, windPreviewSettings.ambient.windNoiseSpeed);
    _panel->setFloat(PID_WIND_NOISE_SCALE, windPreviewSettings.ambient.windNoiseScale);
    _panel->setFloat(PID_WIND_NOISE_PERPENDICULAR, windPreviewSettings.ambient.windNoisePerpendicular);
    _panel->setText(PID_WIND_LEVEL, windPreviewSettings.levelPath);

    enableAmbientWindPanel(_panel);
  }

  void addAmbientWindPanel(PropPanel::ContainerPropertyControl *_panel)
  {
    PropPanel::ContainerPropertyControl *_wind = _panel->createGroup(PID_WIND_GROUP, "Ambient wind settings");
    _wind->createCheckBox(PID_WIND_ENABLED, "Override weather settings (Preview only!)", false);
    _wind->createTrackFloat(PID_WIND_DIR, "Direction", 0, 0, 360, 1);
    _wind->createTrackFloat(PID_WIND_STRENGTH, "Strength (Beaufort)", 0, 0, 12, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_STRENGTH, "Noise strength (Multiplier of strength)", 0, 0, 10, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_SPEED, "Noise speed (Beaufort)", 0, 0, 12, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_SCALE, "Noise scale (Meters)", 0, 0, 200, 1);
    _wind->createTrackFloat(PID_WIND_NOISE_PERPENDICULAR, "Noise perpendicular", 0, 0, 2, 0.1);
    _wind->createFileButton(PID_WIND_LEVEL, "Level ecs blk with wind settings");

    static const IWindService::PreviewSettings defaults{};
    _wind->setDefaultValueById(PID_WIND_DIR, defaults.windAzimuth);
    _wind->setDefaultValueById(PID_WIND_STRENGTH, defaults.windStrength);
    _wind->setDefaultValueById(PID_WIND_NOISE_STRENGTH, defaults.ambient.windNoiseStrength);
    _wind->setDefaultValueById(PID_WIND_NOISE_SPEED, defaults.ambient.windNoiseSpeed);
    _wind->setDefaultValueById(PID_WIND_NOISE_SCALE, defaults.ambient.windNoiseScale);
    _wind->setDefaultValueById(PID_WIND_NOISE_PERPENDICULAR, defaults.ambient.windNoisePerpendicular);
    _wind->setDefaultValueById(PID_WIND_LEVEL, defaults.levelPath);

    _wind->createButton(PID_REVERT_WIND_BUTTON, "Revert to default");

    setAmbientWindPanel(_panel);
  }

  void enableAmbientWindPanel(PropPanel::ContainerPropertyControl *_panel)
  {
    bool enableOverride = windPreviewSettings.enabled;
    _panel->setEnabledById(PID_WIND_DIR, enableOverride);
    _panel->setEnabledById(PID_WIND_STRENGTH, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_STRENGTH, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_SPEED, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_SCALE, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_PERPENDICULAR, enableOverride);

    IWindService *windSrv = EDITORCORE->queryEditorInterface<IWindService>();
    _panel->setEnabledById(PID_WIND_LEVEL, enableOverride && windSrv && windSrv->isLevelEcsSupported());
  }

protected:
  void updateRevertButtons()
  {
    updateRevertButton(PID_SUN_GROUP, PID_REVERT_SUB_AMB_BUTTON);
    updateRevertButton(PID_WIND_GROUP, PID_REVERT_WIND_BUTTON);
    updateRevertButton(PID_RENDER_GROUP, PID_REVERT_RENDER_BUTTON);
  }

  void updateRevertButton(int pcb_group, int pcb_id)
  {
    if (PropPanel::PropertyControlBase *control = getPanel()->getById(pcb_group))
    {
      const bool enabled = !control->isDefaultValueSet();
      if (PropPanel::PropertyControlBase *control = getPanel()->getById(pcb_id))
      {
        getPanel()->setEnabledById(pcb_id, enabled);
        getPanel()->setTooltipId(pcb_id, enabled ? "" : "All set to default");
      }
    }
  }

  void OnRevertToDfaultsButtonClicked(int button)
  {
    if (
      ConfirmationDialog("Revert to defaults", "Are you sure you want to revert to defaults?").showDialog() == PropPanel::DIALOG_ID_OK)
    {
      switch (button)
      {
        case PID_REVERT_SUB_AMB_BUTTON: getPanel()->getById(PID_SUN_GROUP)->applyDefaultValue(); break;
        case PID_REVERT_WIND_BUTTON: getPanel()->getById(PID_WIND_GROUP)->applyDefaultValue(); break;
        case PID_REVERT_RENDER_BUTTON: getPanel()->getById(PID_RENDER_GROUP)->applyDefaultValue(); break;
      }
    }
  }

  SunLightProps *sunProps;
};

class EnvironmentSettingsDialogController : public ModelessDialogWindowController<EnvSetDlg>
{
public:
  const char *getWindowId() const override { return WindowIds::MAIN_SETTINGS_ENVIRONMENT; }

  void releaseWindow() override
  {
    saveSettingsFromDialog();
    ModelessDialogWindowController::releaseWindow();
  }

  void saveSettingsFromDialog()
  {
    if (dialog)
    {
      de_ui_state.clearData();
      dialog->getPanel()->saveState(de_ui_state, /*by_name = */ true);
    }
  }

private:
  void createDialog() override
  {
    ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
    if (ltSrv->getSunCount() == 0)
    {
      logerr("Sun count is zero. The environment dialog will not be displayed.");
      return;
    }

    SunLightProps *defSun = ltSrv->getSun(0);
    dialog.reset(new EnvSetDlg(nullptr, defSun));
    dialog->setDialogButtonText(PropPanel::DIALOG_ID_OK, "Close");
    dialog->removeDialogButton(PropPanel::DIALOG_ID_CANCEL);
    dialog->getPanel()->loadState(de_ui_state, /*by_name = */ true);

    if (!dialog->hasEverBeenShown())
      dialog->setWindowSize(IPoint2(hdpi::_pxS(500), (int)(ImGui::GetIO().DisplaySize.y * 0.8f)));

    // ImGui scrolls to the focused item, which in this case is at the bottom and requires scrolling. Prevent that.
    dialog->setInitialFocus(PropPanel::DIALOG_ID_NONE);
  }
};

static EnvironmentSettingsDialogController environment_settings_dialog_controller;

IModelessWindowController *get_environment_settings_dialog_controller() { return &environment_settings_dialog_controller; }

void load_settings(DataBlock &blk)
{
  DataBlock appblk(DAGORED2->getWorkspace().getAppBlkPath());
  IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
  const DataBlock &reBlk = *blk.getBlockByNameEx("render");
  drSrv->setRenderType(reBlk.getInt("renderType", drSrv->RTYPE_CLASSIC));
  for (int i = 0; i < drSrv->ROPT_COUNT; i++)
    if (drSrv->getRenderOptSupported(i))
      drSrv->setRenderOptEnabled(i, blk.getBool(drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i)));
  drSrv->setShadowQuality(blk.getInt("shadowQ", appblk.getInt("DE_defaultShadowQ", 0)));
  if (drSrv->hasExposure())
    drSrv->setExposure(blk.getReal("exposure", appblk.getReal("DE_defaultExposure", 1.0f)));

  rdbgType = blk.getInt("rdbgType", 0);
  rdbgUseNmap = blk.getBool("rdbgUseNmap", true);
  rdbgUseDtex = blk.getBool("rdbgUseDtex", true);
  rdbgUseLt = blk.getBool("rdbgUseLt", true);
  rdbgUseChrome = blk.getBool("rdbgUseChrome", true);
  envBlkFn = blk.getStr("environment_settings_fn", NULL);
  paintDetailsTexAsset = blk.getStr("paint_details_tex", NULL);
  paintDetailsTexAsset = DagorAsset::fpath2asset(paintDetailsTexAsset);

  detectRenderDebug();
  applyRenderDebug();
  apply_envi_snapshot();
  set_paint_detail_texture();

  DataBlock &windBlk = *blk.addBlock("ambientWind");
  IWindService::readSettingsBlk(windPreviewSettings, windBlk);
  if (IWindService *windSrv = EDITORCORE->queryEditorInterface<IWindService>())
    windSrv->setPreview(windPreviewSettings);
}

void save_settings(DataBlock &blk)
{
  IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
  DataBlock &reBlk = *blk.addBlock("render");
  reBlk.setInt("renderType", drSrv->getRenderType());
  for (int i = 0; i < drSrv->ROPT_COUNT; i++)
    if (drSrv->getRenderOptSupported(i))
      blk.setBool(drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i));
  blk.setInt("shadowQ", drSrv->getShadowQuality());
  if (drSrv->hasExposure())
    blk.setReal("exposure", drSrv->getExposure());

  blk.setInt("rdbgType", rdbgType);
  blk.setBool("rdbgUseNmap", rdbgUseNmap);
  blk.setBool("rdbgUseDtex", rdbgUseDtex);
  blk.setBool("rdbgUseLt", rdbgUseLt);
  blk.setBool("rdbgUseChrome", rdbgUseChrome);
  blk.setStr("environment_settings_fn", envBlkFn);
  blk.setStr("paint_details_tex", paintDetailsTexAsset);

  DataBlock &windBlk = *blk.addBlock("ambientWind");
  IWindService::writeSettingsBlk(windBlk, windPreviewSettings);
}

void load_ui_state(const DataBlock &per_app_settings)
{
  de_ui_state.clearData();
  if (const DataBlock *uiState = per_app_settings.getBlockByName("environmentSettingsUiState"))
    de_ui_state.setFrom(uiState);
}

void save_ui_state(DataBlock &per_app_settings)
{
  environment_settings_dialog_controller.saveSettingsFromDialog();
  per_app_settings.addNewBlock(&de_ui_state, "environmentSettingsUiState");
}

bool on_asset_changed(const DagorAsset &asset)
{
  if (strcmp(asset.getName(), paintDetailsTexAsset) != 0 && strcmp(asset.getName(), "assets_color_global_tex_palette") != 0)
    return false;

  logdbg("Updating paint details texture asset %s", asset.getName());
  set_paint_detail_texture();
  return true;
}

void clear() { ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(combinedPaintTexId, combinedPaintTex); }

}; // namespace environment