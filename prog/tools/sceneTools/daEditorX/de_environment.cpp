#include "de_appwnd.h"
#include <oldEditor/de_workspace.h>

#include <propPanel2/comWnd/dialog_window.h>

#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <de3_editorEvents.h>
#include <de3_dynRenderService.h>
#include <de3_skiesService.h>
#include <de3_interface.h>
#include <de3_windService.h>

#include <EditorCore/ec_workspace.h>
#include <libTools/util/strUtil.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderDbg.h>
#include <debug/dag_debug.h>

#include <workCycle/dag_workCycle.h>
#include <3d/dag_drv3d.h>
#include <textureUtil/textureUtil.h>
#include <assets/asset.h>

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

  PID_RENDER_GROUP,
  PID_RENDER_TYPE,
  PID_RENDER_OPT,
  PID_RENDER_OPT_LAST = PID_RENDER_OPT + IDynRenderService::ROPT_COUNT,
  PID_SHADOW_QUALITY,
  PID_EXPOSURE,

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
  PID_WIND_LEVEL
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

static E3DCOLOR color4ToE3dcolor(Color4 col, float &brightness)
{
  brightness = col.r;
  if (col.g > brightness)
    brightness = col.g;
  if (col.b > brightness)
    brightness = col.b;

  if (brightness <= 0.0)
    return E3DCOLOR(0, 0, 0, 0);

  col *= 255.0 / brightness;
  return E3DCOLOR(col.r, col.g, col.b, col.a);
}


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

static void applyRenderDebug(bool repaint = true)
{
  int en = rdbgType > 0 || !rdbgUseNmap || !rdbgUseDtex || !rdbgUseLt || !rdbgUseChrome;
  ShaderGlobal::set_int_fast(rdbgEnabledGvid, en);
  ShaderGlobal::set_int_fast(rdbgTypeGvid, rdbgType);
  ShaderGlobal::set_int_fast(rdbgUseNmapGvid, rdbgUseNmap);
  ShaderGlobal::set_int_fast(rdbgUseDtexGvid, rdbgUseDtex);
  ShaderGlobal::set_int_fast(rdbgUseLtGvid, rdbgUseLt);
  ShaderGlobal::set_int_fast(rdbgUseChromeGvid, rdbgUseChrome);
  enable_diffuse_mipmaps_debug(rdbgType == -1);
  if (repaint)
    EDITORCORE->queryEditorInterface<IDynRenderService>()->renderViewportFrame(NULL);
}

static void apply_envi_snapshot()
{
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
  globalPaintColorsTexId = get_managed_texture_id("assets_color_global_tex_palette*");
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
    combinedPaintTex->texfilter(TEXFILTER_POINT);
    TextureInfo texInfo;
    combinedPaintTex->getinfo(texInfo);
    ShaderGlobal::set_real(get_shader_variable_id("paint_details_tex_inv_h", true), safediv(1.f, (float)texInfo.h));
  }
  else if (VariableMap::isGlobVariablePresent(paintDetailsVarId))
    DAEDITOR3.conError("failed to create combined painting texture (globTid=0x%x(%s) localTid=0x%x paintDetailsTexAsset='%s' var=%d)",
      globalPaintColorsTexId, get_managed_texture_name(globalPaintColorsTexId), localPaintColorsTexId, paintDetailsTexAsset,
      paintDetailsVarId);
  release_managed_tex(globalPaintColorsTexId);
  release_managed_tex(localPaintColorsTexId);
  ShaderGlobal::set_texture(paintDetailsVarId, combinedPaintTexId);
}

class EnvSetDlg : public CDialogWindow
{
public:
  EnvSetDlg(void *phandle, SunLightProps *sun_props) :
    CDialogWindow(phandle, hdpi::_pxScaled(400), hdpi::_pxScaled(800), "Environment Settings"), sunProps(sun_props)
  {
    IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
    PropertyContainerControlBase *_panel = getPanel();
    G_ASSERT(_panel && "No panel in EnvSetDlg");

    G_ASSERT(sun_props && "Sun properties is NULL in EnvSetDlg");

    PropertyContainerControlBase *_settings = _panel->createGroup(PID_SUN_GROUP, "Environment settings");
    {
      _settings->createTrackFloat(PID_SUN_AZIMUT, "sun azimut", RadToDeg(sunProps->azimuth), -180.f, 180.f, 0.1);
      _settings->createTrackFloat(PID_SUN_ZENITH, "sun zenith", RadToDeg(sunProps->zenith), -5.f, 90.f, 0.1);
      _settings->createEditFloat(PID_SUN_ANGSIZE, "sun angSize", sunProps->angSize, 3);
      _settings->createEditFloat(PID_SUN_BRIGHTNESS, "sun integral luminocity", sunProps->calcLuminocity(), 3);
      _settings->createSimpleColor(PID_SUN_COLOR, "sun color", sunProps->color);
      ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
      SkyLightProps &sky = ltSrv->getSky();
      _settings->createSimpleColor(PID_AMB_COLOR, "ambient color", sky.color);
      _settings->createEditFloat(PID_AMB_BRIGHTNESS, "ambient brightness", sky.brightness, 2);
      _settings->setBoolValue(true);
    }

    addAmbientWindPanel(_panel);

    PropertyContainerControlBase *_dbgShow =
      (drSrv && drSrv->getDebugShowTypeNames().size() > 1) ? _panel->createGroup(PID_DBGSHOW_GRP, "Debug settings") : NULL;
    if (_dbgShow)
    {
      dag::ConstSpan<const char *> modes = drSrv->getDebugShowTypeNames();
      PropertyContainerControlBase *_dbgShowRG = _dbgShow->createRadioGroup(PID_DBGSHOW_MODE, "Debug show mode");
      for (int i = 0; i < modes.size(); i++)
        _dbgShowRG->createRadio(i, modes[i]);
      _dbgShow->setInt(PID_DBGSHOW_MODE, drSrv->getDebugShowType());
    }

    if (supportRenderDebug)
    {
      PropertyContainerControlBase &ltDbgGrp = *_panel->createGroup(PID_SUN_GROUP, "Render debug");
      PropertyContainerControlBase &ltGrp = *ltDbgGrp.createRadioGroup(PID_LTDBG_GROUP, "Debug mode");
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

    PropertyContainerControlBase *_render_grp = _panel->createGroupBox(PID_RENDER_GROUP, "Render");
    {
      static const char *rtypeName[] = {"classic", "dynamic-A", "deferred"};
      dag::ConstSpan<int> rtypes = drSrv->getSupportedRenderTypes();
      Tab<String> rtypeStr;
      rtypeStr.resize(rtypes.size());
      for (int i = 0; i < rtypes.size(); i++)
        rtypeStr[i] = rtypeName[rtypes[i]];

      _render_grp->createCombo(PID_RENDER_TYPE, "render type", rtypeStr, find_value_idx(rtypes, drSrv->getRenderType()));

      for (int i = 0; i < drSrv->ROPT_COUNT; i++)
        if (drSrv->getRenderOptSupported(i))
          _render_grp->createCheckBox(PID_RENDER_OPT + i, drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i));
      if (drSrv->getShadowQualityNames().size() > 0 && drSrv->getShadowQualityNames()[0])
      {
        shadowQualityStr.resize(drSrv->getShadowQualityNames().size());
        for (int i = 0; i < shadowQualityStr.size(); i++)
          shadowQualityStr[i] = drSrv->getShadowQualityNames()[i];
        _render_grp->createCombo(PID_SHADOW_QUALITY, "shadow quality", shadowQualityStr, drSrv->getShadowQuality());
      }
      if (drSrv->hasExposure())
        _render_grp->createTrackFloatLogarithmic(PID_EXPOSURE, "exposure (0-10 logarithmic)", drSrv->getExposure(), 0.0f, 10.0f, 0.0f,
          sqrt(10.0f));

      rtypeStr.clear();
      rtypeStr.push_back() = "Environment BLK files|envi.blk";
      _render_grp->createFileButton(PID_ENV_BLK, "envi snapshot settings");
      _render_grp->setStrings(PID_ENV_BLK, rtypeStr);
      _render_grp->setText(PID_ENV_BLK, envBlkFn);

      _render_grp->createStatic(-1, "paint details texture");
      _render_grp->createButton(PID_PAINT_DETAILS_TEXTURE,
        paintDetailsTexAsset.empty() ? "-- no palette tex --" : paintDetailsTexAsset.str());
    }
  }

  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel)
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
    }
  }

  virtual void onChange(int pcb_id, PropPanel2 *panel)
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
          else if (pcb_id == PID_RENDER_OPT + drSrv->ROPT_NO_POSTFX && !panel->getBool(pcb_id))
            drSrv->renderFramesToGetStableAdaptation(0.1, 10);
        }
    }
    DAGORED2->repaint();
    EDITORCORE->queryEditorInterface<IDynRenderService>()->renderViewportFrame(NULL);
  }

  void setAmbientWindPanel(PropertyContainerControlBase *_panel)
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

  void addAmbientWindPanel(PropertyContainerControlBase *_panel)
  {
    PropertyContainerControlBase *_wind = _panel->createGroup(PID_WIND_GROUP, "Ambient wind settings");
    _wind->createCheckBox(PID_WIND_ENABLED, "Override weather settings (Preview only!)", false);
    _wind->createTrackFloat(PID_WIND_DIR, "Direction", 0, 0, 360, 1);
    _wind->createTrackFloat(PID_WIND_STRENGTH, "Strength (Beaufort)", 0, 0, 12, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_STRENGTH, "Noise strength (Multiplier of strength)", 0, 0, 10, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_SPEED, "Noise speed (Beaufort)", 0, 0, 12, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_SCALE, "Noise scale (Meters)", 0, 0, 200, 1);
    _wind->createTrackFloat(PID_WIND_NOISE_PERPENDICULAR, "Noise perpendicular", 0, 0, 2, 0.1);
    _wind->createFileButton(PID_WIND_LEVEL, "Level ecs blk with wind settings");

    setAmbientWindPanel(_panel);
  }

  void enableAmbientWindPanel(PropertyContainerControlBase *_panel)
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
  SunLightProps *sunProps;
};

void show_environment_settings(void *phandle)
{
  IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  if (ltSrv->getSunCount() == 0)
    return;

  SunLightProps *defSun = ltSrv->getSun(0);
  SkyLightProps &sky = ltSrv->getSky();

  SunLightProps sunProps = *defSun;
  SimpleString saved_envBlkFn(envBlkFn);
  int rtype = drSrv->getRenderType();
  int shadowQuality = drSrv->getShadowQuality();
  int saved_dstype = drSrv->getDebugShowType();
  bool saved_ropt[drSrv->ROPT_COUNT];
  for (int i = 0; i < drSrv->ROPT_COUNT; i++)
    saved_ropt[i] = drSrv->getRenderOptEnabled(i);
  SkyLightProps skyProps = sky;


  EnvSetDlg envSetDlg(phandle, defSun);
  envSetDlg.autoSize();
  if (envSetDlg.showDialog() != DIALOG_ID_OK)
  {
    *defSun = sunProps;
    sky = skyProps;
    applyRenderDebug(false);
    updateLight();

    drSrv->setRenderType(rtype);
    drSrv->setShadowQuality(shadowQuality);
    drSrv->setDebugShowType(saved_dstype);
    for (int i = 0; i < drSrv->ROPT_COUNT; i++)
      drSrv->setRenderOptEnabled(i, saved_ropt[i]);
    envBlkFn = saved_envBlkFn;
    apply_envi_snapshot();
    drSrv->renderFramesToGetStableAdaptation(0.1, 10);
  }
}


void load_settings(DataBlock &blk)
{
  DataBlock appblk(DAGORED2->getWorkspace().getAppPath());
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
  const DataBlock &ltData = *appblk.getBlockByNameEx("AssetLight");

  detectRenderDebug();
  applyRenderDebug(false);
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
}; // namespace environment