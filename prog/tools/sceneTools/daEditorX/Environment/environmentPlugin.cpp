// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <astronomy/astronomy.h>
#include "environmentCommands.h"
#include "environmentPlugin.h"
#include "postFX_panel.h"
#include "shGV_panel.h"
#include "hdrSettingsDlg.h"

#include <propPanel/commonWindow/color_correction_info.h>
#include <propPanel/commonWindow/curveColorDialog.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/panelWindow.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <gui/dag_stdGuiRenderEx.h>

#include <coolConsole/coolConsole.h>

#include <oldEditor/de_util.h>
#include <oldEditor/exportToLightTool.h>
#include <oldEditor/de_workspace.h>

#include <de3_lightService.h>
#include <de3_skiesService.h>
#include <de3_lightProps.h>
#include <de3_interface.h>
#include <de3_editorEvents.h>
#include <de3_dynRenderService.h>
#include <de3_windService.h>
#include <de3_hmapService.h>

#include <EditorCore/ec_ObjectCreator.h>
#include <EditorCore/ec_colors.h>
#include <EditorCore/ec_wndGlobal.h>

#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>

#include <scene/dag_visibility.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <math/dag_capsule.h>
#include <osApiWrappers/dag_direct.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <math/dag_colorMatrix.h>

#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>

#include <sceneBuilder/nsb_export_lt.h>
#include <sceneBuilder/nsb_LightmappedScene.h>
#include <sceneBuilder/nsb_LightingProvider.h>
#include <sceneBuilder/nsb_StdTonemapper.h>

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>

#include <libTools/staticGeom/staticGeometryContainer.h>

#include <libTools/renderViewports/cachedViewports.h>
#include "../de_appwnd.h"

#include <ioSys/dag_memIo.h>

#if _TARGET_PC_WIN
#include <winsock2.h>
#undef ERROR
#else
#include <arpa/inet.h>
#endif

#define METRIC_MAX_FACES            50
#define METRIC_ZNEAR_MIN            0.05
#define METRIC_ZNEAR_MAX            0.5
#define METRIC_ZFAR_MIN             60.0
#define METRIC_ZFAR_MAX             30000.0
#define METRIC_ZNEAR_ZFAR_MAX_RATIO 10000.0

#define MIN_TEX_GAMMA_ADAPTATION 0.01

#define HTTP_SERVER_PORT 23456


enum
{
  PID_USE_HDR,
  PID_USE_LDR,
};


enum
{
  PROPBAR_WIDTH = 280,
  PROPBAR_ENVIRONMENT_WTYPE = 182,
  POSTFX_ENVIRONMENT_WTYPE,
  SHADERGV_ENVIRONMENT_WTYPE,
};


enum
{
  COLOR_TEXT_WIDTH = 256,
  COLOR_TEXT_HEIGHT = 1,
};


#define DEF_FOG_DENSITY      0.001
#define MIN_FOG_DENSITY      0.0000001
#define MIN_FOG_DENSITY_ACES 0.0000001
#define MAX_FOG_DENSITY      100.0
#define FOG_DENSITY_STEP     0.0001
#define DEF_FOG_COLOR        E3DCOLOR(200, 200, 200, 255)

#define ENVI_DAG "single_envi.dag"


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


ISceneLightService *EnvironmentPlugin::ltService = NULL;
ISkiesService *EnvironmentPlugin::skiesSrv = NULL;
IWindService *EnvironmentPlugin::windSrv = NULL;
bool EnvironmentPlugin::needSkiesUpdate = true;
bool EnvironmentPlugin::recreatePlugin = false;

bool EnvironmentPlugin::prepareRequiredServices()
{
  ltService = DAGORED2->queryEditorInterface<ISceneLightService>();
  if (!ltService)
    return false;
  return true;
}

//==============================================================================
EnvironmentPlugin::EnvironmentPlugin() :
  isVisible(false),
  propPanel(NULL),
  toolBarId(0),
  doResetEnvi(false),
  isHdr(false),
  isLdr(true),
  skyScaleVarId(-1),
  skyWorldXVarId(VariableMap::BAD_ID),
  skyWorldYVarId(VariableMap::BAD_ID),
  skyWorldZVarId(VariableMap::BAD_ID),
  previewFog(false),
  postfxPanel(NULL),
  shgvPanel(NULL),
  sunAzimuth(0.0),
  isPPVisible(true),
  selectedWeatherPreset(-1),
  selectedWeatherType(-1),
  weatherPresetsList(midmem),
  weatherTypesList(midmem),
  selectedEnvironmentNo(-1),
  environmentNamesList(midmem),
  colorTex(NULL),
  gameHttpServerAddr(INADDR_NONE)
{
  DataBlock app_blk(DAGORED2->getWorkspace().getAppPath());
  const DataBlock &envi_def = *app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi");

  memset(&envParams, 0, sizeof(envParams));
  envParams.znzfScale = 1;
  const char *type = envi_def.getStr("type", "classic");
  skiesSrv = NULL;
  windSrv = NULL;
  if (strcmp(type, "classic") == 0)
    isAcesPlugin = false;
  else if (strcmp(type, "aces") == 0)
    isAcesPlugin = true;
  else if (strcmp(type, "skies") == 0)
  {
    isAcesPlugin = true;
    skiesSrv = DAGORED2->queryEditorInterface<ISkiesService>();
    if (skiesSrv)
    {
      skiesSrv->setup(DAGORED2->getWorkspace().getAppDir(), envi_def);
      skiesSrv->init();
    }
    windSrv = DAGORED2->queryEditorInterface<IWindService>();
    if (windSrv)
      windSrv->init(DAGORED2->getWorkspace().getAppDir(), envi_def);
  }
  else
  {
    isAcesPlugin = false;
    DAEDITOR3.conError("unsupported type <%s> in projectDefaults/envi", type);
  }

  colorTex = d3d::create_tex(NULL, COLOR_TEXT_WIDTH, COLOR_TEXT_HEIGHT, TEXCF_BEST | TEXCF_DYNAMIC | TEXCF_RGB, 1);
  d3d_err(colorTex);
  unsigned char channel[256];
  for (int i = 0; i < 256; ++i)
    channel[i] = i;
  prepareColorTex(channel, channel, channel);
  ccInfo = new PropPanel::ColorCorrectionInfo();

#if _TARGET_PC_WIN // TODO: tools Linux porting: WSAStartup
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
    debug("WSAStartup failed");
#else
  LOGERR_CTX("TODO: tools Linux porting: WSAStartup");
#endif
}


//==============================================================================
EnvironmentPlugin::~EnvironmentPlugin()
{
  if (colorTex)
  {
    del_d3dres(colorTex);
    colorTex = NULL;
  }

  del_it(ccInfo);

#if _TARGET_PC_WIN // TODO: tools Linux porting: WSACleanup
  WSACleanup();
#else
  LOGERR_CTX("TODO: tools Linux porting: WSACleanup");
#endif
}


//==============================================================================
void EnvironmentPlugin::registered()
{
  if (!isAcesPlugin)
    setFogDefaults();
  else if (!DAGORED2->getConsole().registerCommand("envi.set", this))
    DAEDITOR3.conError("[%s] Couldn't register command 'envi.set'", getMenuCommandName());
  if (!DAGORED2->getConsole().registerCommand("envi.sun_from_time", this))
    DAEDITOR3.conError("[%s] Couldn't register command 'envi.sun_from_time'", getMenuCommandName());


  isVisible = true;

  if (!isAcesPlugin)
  {
    skyScaleVarId = ::get_shader_variable_id("sky_scale", true);
    skyWorldXVarId = ::get_shader_glob_var_id("sky_world_x", true);
    skyWorldYVarId = ::get_shader_glob_var_id("sky_world_y", true);
    skyWorldZVarId = ::get_shader_glob_var_id("sky_world_z", true);
  }

  skyFogColorAndDensityVarId = ::get_shader_variable_id("sky_fog_color_and_density", true);
  sunFogColorAndDensityVarId = ::get_shader_variable_id("sun_fog_color_and_density", true);
  sunFogAzimuthSincosVarId = ::get_shader_variable_id("sun_fog_azimuth_sincos", true);
}


void EnvironmentPlugin::unregistered()
{
  if (windSrv)
    windSrv->term();
  if (isAcesPlugin)
    DAGORED2->getConsole().unregisterCommand("envi.set", this);
  DAGORED2->getConsole().unregisterCommand("envi.sun_from_time", this);
}


//==============================================================================
void *EnvironmentPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IRenderingService);
  RETURN_INTERFACE(huid, IBinaryDataBuilder);
  RETURN_INTERFACE(huid, IRenderOnCubeTex);
  RETURN_INTERFACE(huid, IPluginAutoSave);
  return NULL;
}


bool EnvironmentPlugin::catchEvent(unsigned ev_huid, void *userData)
{
  if (ev_huid == HUID_SunPropertyChanged)
    updateLight();

  return false;
}


//==============================================================================
void EnvironmentPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();

  wndManager.addViewportAccelerator(CM_SHOW_PANEL, ImGuiKey_P);
  wndManager.addViewportAccelerator(CM_RESET_GIZMO, ImGuiKey_Delete);

  if (!isAcesPlugin)
    wndManager.addViewportAccelerator(CM_IMPORT, ImGuiMod_Ctrl | ImGuiKey_O);
}


//==============================================================================
bool EnvironmentPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();

  if (!isAcesPlugin)
  {
    mainMenu->addItem(menu_id, CM_IMPORT, "Import DAG...\tCtrl+O");
    mainMenu->addSeparator(menu_id);
  }

  mainMenu->addItem(menu_id, CM_SHOW_PANEL, "Show settings\tP");
  if (!skiesSrv)
  {
    mainMenu->addItem(menu_id, CM_SHOW_POSTFX_PANEL, "Show PostFX settings");
    mainMenu->addItem(menu_id, CM_SHOW_SHGV_PANEL, "Show Shader Global Vars");
    mainMenu->addItem(menu_id, CM_HDR_VIEW_SETTINGS, "HDR settings...");
  }
  mainMenu->addItem(menu_id, CM_HTTP_GAME_SERVER, "Connect To Game HTTP Server");

  mainMenu->addItem(menu_id, CM_COLOR_CORRECTION, "Color correction..."); // debug

  if (!isAcesPlugin)
  {
    mainMenu->addSeparator(menu_id);
    mainMenu->addItem(menu_id, CM_ERASE_ENVI, "Erase geometry");
  }

  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  toolbar->setEventHandler(this);

  PropPanel::ContainerPropertyControl *tool = toolbar->createToolbarPanel(CM_TOOL, "");

  tool->createButton(CM_IMPORT, "Import DAG (Ctrl+O)");
  tool->setButtonPictures(CM_IMPORT, "import_dag");
  tool->createSeparator();

  tool->createCheckBox(CM_SHOW_PANEL, "Show settings (P)");
  tool->setButtonPictures(CM_SHOW_PANEL, "show_panel");
  if (!skiesSrv)
  {
    tool->createCheckBox(CM_SHOW_POSTFX_PANEL, "Show PostFX settings");
    tool->setButtonPictures(CM_SHOW_POSTFX_PANEL, "show_panel_postfx");
    tool->createCheckBox(CM_SHOW_SHGV_PANEL, "Show shader global variables");
    tool->setButtonPictures(CM_SHOW_SHGV_PANEL, "show_panel_postfx");
    tool->createButton(CM_HDR_VIEW_SETTINGS, "HDR view settings...");
    tool->setButtonPictures(CM_HDR_VIEW_SETTINGS, "hdr_view_settings");
  }
  tool->createSeparator();
  tool->createButton(CM_HTTP_GAME_SERVER, "Connect to game http server");
  tool->setButtonPictures(CM_HTTP_GAME_SERVER, "terr_up_vertex");

  if (!isAcesPlugin)
  {
    tool->createButton(CM_ERASE_ENVI, "Erase geometry");
    tool->setButtonPictures(CM_ERASE_ENVI, "clear");
  }

  IWndManager *manager = DAGORED2->getWndManager();
  manager->registerWindowHandler(this);

  if (isPPVisible)
    showPanel();

  return true;
}


//==============================================================================
bool EnvironmentPlugin::end()
{
  isPPVisible = (bool)propPanel;

  if (isPPVisible)
    showPanel();

  if (postfxPanel)
    recreatePostfxPanel();
  if (shgvPanel)
    recreateShGVPanel();

  IWndManager *manager = DAGORED2->getWndManager();
  manager->unregisterWindowHandler(this);

  return true;
}


//==============================================================================
void EnvironmentPlugin::importEnvi()
{
  const String openPath = ::find_in_base_smart(lastImportedEnvi, DAGORED2->getSdkDir(), ::make_full_path(DAGORED2->getSdkDir(), ""));

  String s = wingw::file_open_dlg(DAGORED2->getWndManager()->getMainWindow(), "Import environment from DAG",
    "DAG files (*.dag)|*.dag|All files (*.*)|*.*", "dag", openPath);

  if (s.length())
  {
    resetEnvi();
    const char *dag_fullpath = s;
    CoolConsole &con = DAGORED2->getConsole();
    con.startLog();
    con.startProgress();
    con.addMessage(ILogWriter::NOTE, "Import envi from dag...");

    const String dag_name(DAGORED2->getPluginFilePath(this, ENVI_DAG));
    bool success = replace_dag(dag_fullpath, dag_name, &con);

    if (success)
    {
      geom.loadFromDag(dag_name, &DAGORED2->getConsole());
      if (geom.getNearestPoint().length() < envParams.z_near)
      {
        const real coef = envParams.z_near / geom.getNearestPoint().lengthSq();
        String msg(128, "Some envi points are nearest znear, envi was scaled %f times", coef);
        debug(msg);
        con.addMessage(ILogWriter::WARNING, msg);
        geom.saveToDagAndScale(dag_name, coef);
        geom.loadFromDag(dag_name, &DAGORED2->getConsole());
      }
      lastImportedEnvi = s;
    }
    else
    {
      dd_erase(dag_name);
      wingw::message_box(wingw::MBS_EXCL, "Import DAG", "Failed to import '%s':\n%s", (const char *)dag_name,
        get_dagutil_last_error());
    }
    con.endLog();
    con.endProgress();
    DAGORED2->repaint();
  }
}


//==============================================================================
void EnvironmentPlugin::resetEnvi()
{
  clear_and_shrink(dagFileName);

  dd_erase(DAGORED2->getPluginFilePath(this, ENVI_DAG));

  for (const alefind_t &ff : dd_find_iterator(DAGORED2->getPluginFilePath(this, "*.dtx"), DA_FILE))
    ::dd_erase(DAGORED2->getPluginFilePath(this, ff.name));

  for (const alefind_t &ff : dd_find_iterator(DAGORED2->getPluginFilePath(this, "*.dds"), DA_FILE))
    ::dd_erase(DAGORED2->getPluginFilePath(this, ff.name));

  DAGORED2->repaint();
  doResetEnvi = false;
}


bool EnvironmentPlugin::onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
{
  CoolConsole &con = DAGORED2->getConsole();

  if (!stricmp(cmd, "envi.set") && params.size() == 1)
  {
    int errCnt = con.getErrorsCount() + con.getFatalsCount();
    for (int i = 0; i < environmentNamesList.size(); i++)
      if (strcmp(params[0], environmentNamesList[i]) == 0)
      {
        if (propPanel)
          propPanel->setInt(CM_PID_ENVIRONMENTS_LIST, i);
        if (selectedEnvironmentNo != -1)
          getSettingsFromPlugins();

        selectedEnvironmentNo = i;
        setSettingsToPlugins();

        updateLight();

        if (postfxPanel)
          recreatePostfxPanel();
        if (shgvPanel)
          recreateShGVPanel();
        return con.getErrorsCount() + con.getFatalsCount() == errCnt;
      }
    DAEDITOR3.conError("unrecognized envi name <%s>", params[0]);
  }
  else if (!stricmp(cmd, "envi.sun_from_time"))
  {
    if (params.size() < 6)
    {
      DAEDITOR3.conError("usage envi.sun_from_time year month day time lattitude longtitude");
      return true;
    }
    unsigned year = atoi(params[0]);
    unsigned month = atoi(params[1]);
    unsigned day = atoi(params[2]);
    double time = atof(params[3]);
    real lat = atof(params[4]);
    real lon = atof(params[5]);
    float timeDelta = lon * 24.0 / 360.0;
    double utctime = time - timeDelta;
    if (utctime < 0)
      utctime += 24.0;
    {
      double jd = julian_day(year, month, day, utctime);
      double lst = calc_lst_hours(jd, lon);

      // Get brightness of brightest star
      star_catalog::StarDesc s = get_sun(jd, s);
      radec_2_altaz(s.rightAscension, s.declination, lst, lat, s.altitude, s.azimuth);
      s.azimuth = norm_s_ang(HALFPI - s.azimuth);
      DAEDITOR3.conNote("Azimuth = %g Zenith = %f, for %d:%d:%d UTC time %d:%d lat =%g lon =%g", RadToDeg(s.azimuth),
        RadToDeg(s.altitude), year, month, day, int(floor(utctime)), int(utctime * 60) % 60, lat, lon);
      if (ltService->getSun(0))
      {
        ltService->getSun(0)->zenith = s.altitude;
        ltService->getSun(0)->azimuth = s.azimuth;
        updateLight();
        onLightingSettingsChanged();
        DAEDITOR3.conNote("Set");
      }
    }
    return true;
  }

  return false;
}


const char *EnvironmentPlugin::onConsoleCommandHelp(const char *cmd)
{
  if (!stricmp(cmd, "envi.set"))
    return "Type:\n"
           "'envi.set <envi_name>' to set current environment/daytime.\n";
  if (!stricmp(cmd, "envi.sun_from_time"))
    return "Type:\n"
           "envi.sun_from_time year month day time lattitude longtitude\n";
  return NULL;
}

bool EnvironmentPlugin::onPluginMenuClickInternal(unsigned id, PropPanel::ContainerPropertyControl *panel)
{
  switch (id)
  {
    case CM_IMPORT: importEnvi(); return true;

    case CM_SHOW_PANEL:
      showPanel();
      EDITORCORE->managePropPanels();
      return true;

    case CM_RESET_GIZMO:
      // undoBefore();
      DAGORED2->setGizmo(NULL, IDagorEd2Engine::MODE_None);
      return true;

    case CM_ERASE_ENVI:
      if (wingw::message_box(wingw::MBS_OKCANCEL | wingw::MBS_QUEST, "Erase geometry", "Erase environment and fog capsules?") ==
          wingw::MB_ID_OK)
      {
        geom.clear();
        clear_and_shrink(dagFileName);
        clearObjects();
        doResetEnvi = true;
        DAGORED2->repaint();
      }
      return true;

    case CM_SHOW_POSTFX_PANEL:
      recreatePostfxPanel();
      EDITORCORE->managePropPanels();
      return true;

    case CM_SHOW_SHGV_PANEL:
      recreateShGVPanel();
      EDITORCORE->managePropPanels();
      return true;

    case CM_HDR_VIEW_SETTINGS: hdrViewSettings(); return true;

    case CM_HTTP_GAME_SERVER: connectToGameHttpServer(); return true;

    case CM_COLOR_CORRECTION:
    {
      PropPanel::CurveColorDialog dialog(DAGORED2->getWndManager()->getMainWindow(), "Color correction");
      int rChannel[256], gChannel[256], bChannel[256];
      prepareColorStats(rChannel, gChannel, bChannel);
      dialog.setColorStats(rChannel, gChannel, bChannel);
      dialog.setCorrectionInfo(*ccInfo);
      if (dialog.showDialog() == PropPanel::DIALOG_ID_OK)
      {
        unsigned char r_tbl[256], g_tbl[256], b_tbl[256];
        dialog.getColorTables(r_tbl, g_tbl, b_tbl);
        prepareColorTex(r_tbl, g_tbl, b_tbl);
        dialog.getCorrectionInfo(*ccInfo);
      }
    }
      return true;
  }

  return false;
}


//==============================================================================
bool EnvironmentPlugin::onPluginMenuClick(unsigned id) { return onPluginMenuClickInternal(id, nullptr); }


//==============================================================================
void EnvironmentPlugin::clearObjects()
{
  if (recreatePlugin)
  {
    unregistered();
    this->~EnvironmentPlugin();
    new (this) EnvironmentPlugin;
    registered();
  }

  needSkiesUpdate = true;

  if (!isAcesPlugin)
  {
    envParams.exportEnviScene = true;
    previewFog = true;
  }

  if (skiesSrv)
  {
    skiesSrv->term();
    skiesSrv->init();
  }
  if (windSrv)
    windSrv->term();

  recreatePlugin = true;
}


//==============================================================================
void EnvironmentPlugin::autoSaveObjects(DataBlock &local_data)
{
  DataBlock &autoBlk = *local_data.addBlock("panel_state");
  autoBlk.setFrom(&mainPanelState);

  PropPanel::CurveColorDialog::savePresets(::make_full_path(sgg::get_exe_path(), "../.local/color_presets.blk"));
}


//==============================================================================
void EnvironmentPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  if (!isAcesPlugin)
  {
    String postFxPath(DAGORED2->getPluginFilePath(this, "gameParams.blk"));
    postfxBlk.saveToTextFile(postFxPath);

    String shgvPath(DAGORED2->getPluginFilePath(this, "shGlobVars.blk"));
    shgvBlk.saveToTextFile(shgvPath);

    local_data.setStr("lastImported", lastImportedEnvi);

    // blk.setBool( "hdrScene", isHdr );
    blk.setBool("ldrScene", isLdr);
    blk.setReal("sun_azimuth", sunAzimuth);

    DataBlock *cb = blk.addBlock("fog");
    if (cb)
    {
      cb->setBool("used", envParams.fog.used);
      cb->setBool("exp2", envParams.fog.exp2);
      cb->setReal("sun_density", envParams.fog.sunDensity);
      cb->setReal("sky_density", envParams.fog.skyDensity);
      cb->setE3dcolor("sun_color", envParams.fog.sunColor);
      cb->setE3dcolor("sky_color", envParams.fog.skyColor);
      cb->setBool("use_sun_fog", envParams.fog.useSunFog);

      cb->setE3dcolor("lfog_sky_color", envParams.fog.lfogSkyColor);
      cb->setE3dcolor("lfog_sun_color", envParams.fog.lfogSunColor);
      cb->setReal("lfog_density", envParams.fog.lfogDensity);
      cb->setReal("lfog_min_height", envParams.fog.lfogMinHeight);
      cb->setReal("lfog_max_height", envParams.fog.lfogMaxHeight);
    }

    cb = blk.addBlock("scene");
    if (cb)
    {
      cb->setReal("z_near", envParams.z_near);
      cb->setReal("z_far", envParams.z_far);
      cb->setReal("rotY", envParams.rotY);
      cb->setReal("znzfScale", envParams.znzfScale);

      cb->setReal("skyHdrMultiplier", envParams.skyHdrMultiplier);
      cb->setReal("exportEnviScene", envParams.exportEnviScene);
    }

    cb = blk.addBlock("preview");
    if (cb)
      cb->setBool("previeiw_fog", previewFog);

    // when there is no dedicated lighting plugin
    if (!DAGORED2->getPluginByName("SceneLighter") && !skiesSrv)
      ltService->saveSunsAndSky(*blk.addNewBlock("envi"));


    if (doResetEnvi)
      resetEnvi();
  }
  else
  {
    getSettingsFromPlugins();

    if (skiesSrv)
    {
      blk.setStr("weatherPreset", ::make_path_relative(weatherPresetsList[selectedWeatherPreset], DAGORED2->getSdkDir()));
      blk.setStr("weatherType", weatherTypesList[selectedWeatherType]);

#define SAVE_ETS(NM, TYPE) local_data.set##TYPE(#NM, ets.NM)
      SAVE_ETS(forceWeather, Bool);
      SAVE_ETS(forceGeoDate, Bool);
      SAVE_ETS(forceTime, Bool);
      SAVE_ETS(forceSeed, Bool);
      SAVE_ETS(latitude, Real);
      SAVE_ETS(yyyy, Int);
      SAVE_ETS(mm, Int);
      SAVE_ETS(dd, Int);
      SAVE_ETS(timeOfDay, Real);
      SAVE_ETS(rndSeed, Int);
#undef SAVE_ETS
    }
    local_data.setReal("znzfScale", envParams.znzfScale);
    blk.setStr("enviName", environmentNamesList[selectedEnvironmentNo]);
    blk.setStr("commonFile", commonFileName);
    blk.setStr("levelFile", levelFileName);

    if (!skiesSrv)
    {
      String commonFilePath(DAGORED2->getSdkDir());
      commonFilePath.append("/" + commonFileName);
      commonBlk.saveToTextFile(commonFilePath);

      String levelFilePath(DAGORED2->getSdkDir());
      levelFilePath.append("/" + levelFileName);
      levelBlk.saveToTextFile(levelFilePath);
    }
  }
}


void EnvironmentPlugin::getSettingsFromPlugins()
{
  G_ASSERT(!skiesSrv || selectedWeatherPreset != -1);
  G_ASSERT(!skiesSrv || selectedWeatherType != -1);
  G_ASSERT(selectedEnvironmentNo != -1);

  // when there is no dedicated lighting plugin
  if (!DAGORED2->getPluginByName("SceneLighter"))
  {
    currentEnvironmentAces.enviBlk.clearData();
    ltService->saveSunsAndSky(currentEnvironmentAces.enviBlk);
  }

  IGenEditorPlugin *hmapPlugin = DAGORED2->getPluginByName("HeightmapLand");
  if (hmapPlugin)
  {
    IEnvironmentSettings *settings = hmapPlugin->queryInterface<IEnvironmentSettings>();
    if (settings)
      settings->getEnvironmentSettings(currentEnvironmentAces.hmapBlk);
  }

  DataBlock *timeBlk = levelBlk.addBlock(environmentNamesList[selectedEnvironmentNo]);
  timeBlk->clearData();

  commonBlk.clearData();
  timeBlk->clearData();

  commonBlk.addBlock("shader_vars")->setFrom(currentEnvironmentAces.shaderVarsBlk.getBlockByNameEx("common"));

  levelBlk.addBlock("shader_vars")->setFrom(currentEnvironmentAces.shaderVarsBlk.getBlockByNameEx("level"));

  timeBlk->addBlock("shader_vars")->setFrom(currentEnvironmentAces.shaderVarsBlk.getBlockByNameEx("time"));

  commonBlk.addBlock("postfx")->addBlock("hdr")->setFrom(currentEnvironmentAces.postfxBlk.getBlockByNameEx("hdr"));

  commonBlk.addBlock("postfx")->addBlock("hdr")->removeBlock("fake");

  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->setFrom(currentEnvironmentAces.postfxBlk.getBlockByNameEx("DemonPostFx"));

  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeBlock("colorMatrix");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("sunAzimuth");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("sunZenith");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("hdrStarThreshold");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("hdrStarPowerMul");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("hdrDarkThreshold");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("hdrGlowPower");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("volfogFade");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("volfogRange");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("volfogMul");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("volfogColor");
  commonBlk.addBlock("postfx")->addBlock("DemonPostFx")->removeParam("volfogMaxAngle");


  timeBlk->addBlock("postfx")->addBlock("hdr")->addBlock("fake")->setFrom(
    currentEnvironmentAces.postfxBlk.getBlockByNameEx("hdr")->getBlockByNameEx("fake"));

  timeBlk->addBlock("postfx")
    ->addBlock("DemonPostFx")
    ->addBlock("colorMatrix")
    ->setFrom(currentEnvironmentAces.postfxBlk.getBlockByNameEx("DemonPostFx")->getBlockByNameEx("colorMatrix"));

  const DataBlock *enviDemonBlk = currentEnvironmentAces.postfxBlk.getBlockByNameEx("DemonPostFx");
  DataBlock *timeDemonBlk = timeBlk->addBlock("postfx")->addBlock("DemonPostFx");
  timeDemonBlk->setReal("sunAzimuth", enviDemonBlk->getReal("sunAzimuth", 0.f));
  timeDemonBlk->setReal("sunZenith", enviDemonBlk->getReal("sunZenith", 0.f));
  timeDemonBlk->setReal("hdrStarThreshold", enviDemonBlk->getReal("hdrStarThreshold", 0.f));
  timeDemonBlk->setReal("hdrStarPowerMul", enviDemonBlk->getReal("hdrStarPowerMul", 0.f));
  timeDemonBlk->setReal("hdrDarkThreshold", enviDemonBlk->getReal("hdrDarkThreshold", 0.f));
  timeDemonBlk->setReal("hdrGlowPower", enviDemonBlk->getReal("hdrGlowPower", 0.f));
  timeDemonBlk->setReal("volfogFade", enviDemonBlk->getReal("volfogFade", 0.f));
  timeDemonBlk->setReal("volfogRange", enviDemonBlk->getReal("volfogRange", 0.f));
  timeDemonBlk->setReal("volfogMul", enviDemonBlk->getReal("volfogMul", 0.f));
  timeDemonBlk->setE3dcolor("volfogColor", enviDemonBlk->getE3dcolor("volfogColor", 0xFFFFFFFF));
  timeDemonBlk->setReal("volfogMaxAngle", enviDemonBlk->getReal("volfogMaxAngle", 0.f));

  timeBlk->addBlock("envi")->setFrom(&currentEnvironmentAces.enviBlk);
  timeBlk->addBlock("hmap")->setFrom(&currentEnvironmentAces.hmapBlk);

  DataBlock *fogBlk = timeBlk->addBlock("fog");
  fogBlk->setE3dcolor("sun_color", currentEnvironmentAces.envParams.fog.sunColor);
  fogBlk->setReal("sun_density", currentEnvironmentAces.envParams.fog.sunDensity);
  fogBlk->setE3dcolor("sky_color", currentEnvironmentAces.envParams.fog.skyColor);
  fogBlk->setReal("sky_density", currentEnvironmentAces.envParams.fog.skyDensity);

  DataBlock *sceneBlk = commonBlk.addBlock("scene");
  if (skiesSrv)
    skiesSrv->getZRange(currentEnvironmentAces.envParams.z_near, currentEnvironmentAces.envParams.z_far);

  sceneBlk->setReal("z_near", currentEnvironmentAces.envParams.z_near);
  sceneBlk->setReal("z_far", currentEnvironmentAces.envParams.z_far);
}


void EnvironmentPlugin::setSettingsToPlugins()
{
  G_ASSERT(!skiesSrv || selectedWeatherPreset != -1);
  G_ASSERT(!skiesSrv || selectedWeatherType != -1);
  G_ASSERT(selectedEnvironmentNo != -1);

  const DataBlock *timeBlk = levelBlk.getBlockByNameEx(environmentNamesList[selectedEnvironmentNo]);

  currentEnvironmentAces.shaderVarsBlk.clearData();
  currentEnvironmentAces.shaderVarsBlk.addNewBlock(commonBlk.getBlockByNameEx("shader_vars"), "common");
  currentEnvironmentAces.shaderVarsBlk.addNewBlock(levelBlk.getBlockByNameEx("shader_vars"), "level");
  currentEnvironmentAces.shaderVarsBlk.addNewBlock(timeBlk->getBlockByNameEx("shader_vars"), "time");

  currentEnvironmentAces.postfxBlk.setFrom(commonBlk.getBlockByNameEx("postfx"));
  currentEnvironmentAces.postfxBlk.addBlock("hdr")->addNewBlock(
    timeBlk->getBlockByNameEx("postfx")->getBlockByNameEx("hdr")->getBlockByNameEx("fake"), "fake");

  currentEnvironmentAces.postfxBlk.addBlock("DemonPostFx")
    ->addNewBlock(timeBlk->getBlockByNameEx("postfx")->getBlockByNameEx("DemonPostFx")->getBlockByNameEx("colorMatrix"),
      "colorMatrix");

  DataBlock *enviDemonBlk = currentEnvironmentAces.postfxBlk.addBlock("DemonPostFx");
  const DataBlock *timeDemonBlk = timeBlk->getBlockByNameEx("postfx")->getBlockByNameEx("DemonPostFx");
  enviDemonBlk->setReal("sunAzimuth", timeDemonBlk->getReal("sunAzimuth", 0.f));
  enviDemonBlk->setReal("sunZenith", timeDemonBlk->getReal("sunZenith", 0.f));
  enviDemonBlk->setReal("hdrStarThreshold", timeDemonBlk->getReal("hdrStarThreshold", 0.f));
  enviDemonBlk->setReal("hdrStarPowerMul", timeDemonBlk->getReal("hdrStarPowerMul", 0.f));
  enviDemonBlk->setReal("hdrDarkThreshold", timeDemonBlk->getReal("hdrDarkThreshold", 0.f));
  enviDemonBlk->setReal("hdrGlowPower", timeDemonBlk->getReal("hdrGlowPower", 0.f));
  enviDemonBlk->setReal("volfogFade", timeDemonBlk->getReal("volfogFade", 0.f));
  enviDemonBlk->setReal("volfogRange", timeDemonBlk->getReal("volfogRange", 0.f));
  enviDemonBlk->setReal("volfogMul", timeDemonBlk->getReal("volfogMul", 0.f));
  enviDemonBlk->setE3dcolor("volfogColor", timeDemonBlk->getE3dcolor("volfogColor", 0xFFFFFFFF));
  enviDemonBlk->setReal("volfogMaxAngle", timeDemonBlk->getReal("volfogMaxAngle", 0.f));

  currentEnvironmentAces.enviBlk.setFrom(timeBlk->getBlockByNameEx("envi"));
  currentEnvironmentAces.hmapBlk.setFrom(timeBlk->getBlockByNameEx("hmap"));

  const DataBlock *fogBlk = timeBlk->getBlockByNameEx("fog");
  currentEnvironmentAces.envParams.fog.sunColor = fogBlk->getE3dcolor("sun_color", 0);
  currentEnvironmentAces.envParams.fog.sunDensity = fogBlk->getReal("sun_density", 0.f);
  currentEnvironmentAces.envParams.fog.skyColor = fogBlk->getE3dcolor("sky_color", 0);
  currentEnvironmentAces.envParams.fog.skyDensity = fogBlk->getReal("sky_density", 0.f);

  const DataBlock *sceneBlk = commonBlk.getBlockByNameEx("scene");
  currentEnvironmentAces.envParams.z_near = sceneBlk->getReal("z_near", 1.f);
  currentEnvironmentAces.envParams.z_far = sceneBlk->getReal("z_far", 10000.f);


  if (skiesSrv)
  {
    skiesSrv->setWeather(weatherPresetsList[selectedWeatherPreset], environmentNamesList[selectedEnvironmentNo],
      weatherTypesList[selectedWeatherType]);
    updateSunSkyEnvi();
  }

  // when there is no dedicated lighting plugin
  if (!DAGORED2->getPluginByName("SceneLighter") && !skiesSrv)
  {
    ltService->loadSunsAndSky(currentEnvironmentAces.enviBlk);
    if (ltService->getSunCount() < 2)
      ltService->setSunCount(2);
    ltService->setSyncLtColors(false);
  }

  IGenEditorPlugin *hmapPlugin = DAGORED2->getPluginByName("HeightmapLand");
  if (hmapPlugin)
  {
    IEnvironmentSettings *settings = hmapPlugin->queryInterface<IEnvironmentSettings>();
    if (settings)
    {
      if (skiesSrv)
        settings->getEnvironmentSettings(currentEnvironmentAces.hmapBlk);
      else
        settings->setEnvironmentSettings(currentEnvironmentAces.hmapBlk);
    }
  }


  if (!skiesSrv)
  {
    ShaderGlobal::set_vars_from_blk(currentEnvironmentAces.shaderVarsBlk);
    EDITORCORE->queryEditorInterface<IDynRenderService>()->restartPostfx(currentEnvironmentAces.postfxBlk);
  }

  onLightingSettingsChanged();
  ltService->updateShaderVars();

  IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
  if (skiesSrv && hmlService)
  {
    static int puddles_powerVarId = ::get_shader_glob_var_id("puddles_power", true);
    static int puddles_seedVarId = ::get_shader_glob_var_id("puddles_seed", true);
    static int puddles_noise_influenceVarId = ::get_shader_glob_var_id("puddles_noise_influence", true);

    float powerScale, seed, noiseInfluence;
    hmlService->getPuddlesParams(powerScale, seed, noiseInfluence);

    float puddlesPower = get_point2lerp_or_real(*skiesSrv->getWeatherTypeBlk(), "puddlesPower", 0.0f, ets.rndSeed);
    puddlesPower *= powerScale;

    ShaderGlobal::set_real(puddles_powerVarId, puddlesPower);

    ShaderGlobal::set_real(puddles_noise_influenceVarId, noiseInfluence);
    ShaderGlobal::set_real(puddles_seedVarId, seed);

    hmlService->invalidateClipmap(false, false);
  }

  if (skiesSrv && windSrv)
  {
    Point2 windDir = levelBlk.getPoint2("windDir", Point2(0.6, 0.8));
    windSrv->setWeather(*skiesSrv->getWeatherTypeBlk(), windDir);
  }
}

void EnvironmentPlugin::updateSunSkyEnvi()
{
  if (!skiesSrv)
    return;

  DataBlock stars;
  DataBlock customW;
  if (ets.forceWeather)
  {
    customW.setBool("#exclusive", true);
    if (!ets.forceTime)
      stars.setBool("#del:localTime", true);
  }
  if (ets.forceGeoDate)
  {
    stars.setReal("latitude", ets.latitude);
    stars.setInt("year", ets.yyyy);
    stars.setInt("month", ets.mm);
    stars.setInt("day", ets.dd);
  }
  if (ets.forceTime)
    stars.setReal("localTime", ets.timeOfDay);

  skiesSrv->overrideWeather(3,
    (ets.forceWeather && selectedEnvironmentNo >= 0) ? environmentNamesList[selectedEnvironmentNo].str() : NULL,
    (ets.forceWeather && selectedEnvironmentNo >= 0) ? weatherTypesList[selectedWeatherType].str() : NULL,
    ets.forceSeed ? ets.rndSeed : -1, ets.forceWeather ? &customW : NULL,
    (ets.forceWeather | ets.forceGeoDate || ets.forceTime) ? &stars : NULL);
}


//==============================================================================
void EnvironmentPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  if (!isAcesPlugin)
  {
    String shgvPath(DAGORED2->getPluginFilePath(this, "shGlobVars.blk"));
    shgvBlk.load(shgvPath);
    ShaderGlobal::set_vars_from_blk(shgvBlk, true);

    String postFxPath(DAGORED2->getPluginFilePath(this, "gameParams.blk"));
    postfxBlk.load(postFxPath);

    DataBlock app_blk;
    if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
      DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());

    const char *hdr_mode = postfxBlk.getBlockByNameEx("hdr")->getStr("mode", "none");
    if (strcmp(hdr_mode, "none") != 0 && !app_blk.getBlockByNameEx("hdr_mode")->getBool(hdr_mode, false))
    {
      DAEDITOR3.conWarning("changed unsupported hdrMode=<%s> to <none>", hdr_mode);
      postfxBlk.addBlock("hdr")->setStr("mode", "none");
    }

    EDITORCORE->queryEditorInterface<IDynRenderService>()->restartPostfx(postfxBlk);

    lastImportedEnvi = local_data.getStr("lastImported", lastImportedEnvi);

    // isHdr = blk.getBool("hdrScene", true);
    isLdr = blk.getBool("ldrScene", true);

    sunAzimuth = blk.getReal("sun_azimuth", sunAzimuth);

    String fileName(::make_full_path(base_path, ENVI_DAG));
    simplify_fname(fileName);
    dagFileName = fileName;
    geom.loadFromDag(dagFileName, &DAGORED2->getConsole());

    const DataBlock *cb = blk.getBlockByName("fog");
    if (cb)
    {
      envParams.fog.used = cb->getBool("used", envParams.fog.used);
      envParams.fog.exp2 = cb->getBool("exp2", envParams.fog.exp2);

      if (cb->paramExists("density"))
        envParams.fog.skyDensity = cb->getReal("density", envParams.fog.skyDensity);
      else
        envParams.fog.skyDensity = cb->getReal("sky_density", envParams.fog.skyDensity);

      envParams.fog.sunDensity = cb->getReal("sun_density", envParams.fog.sunDensity);

      if (cb->paramExists("color"))
        envParams.fog.skyColor = cb->getE3dcolor("color", envParams.fog.skyColor);
      else
        envParams.fog.skyColor = cb->getE3dcolor("sky_color", envParams.fog.skyColor);

      envParams.fog.sunColor = cb->getE3dcolor("sun_color", envParams.fog.sunColor);
      envParams.fog.useSunFog = cb->getBool("use_sun_fog", envParams.fog.useSunFog);

      envParams.fog.lfogSkyColor = cb->getE3dcolor("lfog_sky_color", envParams.fog.lfogSkyColor);
      envParams.fog.lfogSunColor = cb->getE3dcolor("lfog_sun_color", envParams.fog.lfogSunColor);
      envParams.fog.lfogDensity = cb->getReal("lfog_density", envParams.fog.lfogDensity);
      envParams.fog.lfogMinHeight = cb->getReal("lfog_min_height", envParams.fog.lfogMinHeight);
      envParams.fog.lfogMaxHeight = cb->getReal("lfog_max_height", envParams.fog.lfogMaxHeight);
    }

    cb = blk.getBlockByName("scene");
    if (cb)
    {
      envParams.z_near = cb->getReal("z_near", envParams.z_near);
      envParams.z_far = cb->getReal("z_far", envParams.z_far);
      envParams.znzfScale = cb->getReal("znzfScale", 1.0f);
      envParams.rotY = cb->getReal("rotY", 0.0);
      envParams.skyHdrMultiplier = cb->getReal("skyHdrMultiplier", envParams.skyHdrMultiplier);
      envParams.exportEnviScene = cb->getBool("exportEnviScene", true);
    }

    cb = blk.getBlockByName("preview");
    if (cb)
      previewFog = cb->getBool("previeiw_fog", previewFog);


    // when there is no dedicated lighting plugin
    if (!DAGORED2->getPluginByName("SceneLighter") && !skiesSrv)
    {
      ltService->loadSunsAndSky(*blk.getBlockByNameEx("envi"));
      if (ltService->getSunCount() < 2)
        ltService->setSunCount(2);
      ltService->setSyncLtColors(false);
    }

    onLightingSettingsChanged();
  }
  else
  {
    selectedEnvironmentNo = -1;
    clear_and_shrink(environmentNamesList);
    selectedWeatherPreset = selectedWeatherType = -1;
    clear_and_shrink(weatherPresetsList);
    clear_and_shrink(weatherTypesList);

    geom.loadFromDag(DAGORED2->getPluginFilePath(this, "single.dag"), &DAGORED2->getConsole());

    commonFileName = commonFileNamePcXbox = blk.getStr("commonFile", NULL);
    levelFileName = levelFileNamePcXbox = blk.getStr("levelFile", NULL);
    commonFileNamePs3 = blk.getStr("commonFilePs3", NULL);
    levelFileNamePs3 = blk.getStr("levelFilePs3", NULL);

    if (skiesSrv)
    {
      skiesSrv->fillPresets(weatherPresetsList, environmentNamesList, weatherTypesList);

      String preset = ::make_full_path(DAGORED2->getSdkDir(), String(blk.getStr("weatherPreset", "")));
      dd_strlwr(preset);
      selectedWeatherPreset = find_value_idx(weatherPresetsList, preset);
      if (selectedWeatherPreset < 0 && weatherPresetsList.size())
        selectedWeatherPreset = 0;

      selectedWeatherType = find_value_idx(weatherTypesList, String(blk.getStr("weatherType", "")));
      if (selectedWeatherType < 0 && weatherTypesList.size())
        selectedWeatherType = 0;

      selectedEnvironmentNo = find_value_idx(environmentNamesList, String(blk.getStr("enviName", "")));
      if (selectedEnvironmentNo < 0 && environmentNamesList.size())
        selectedEnvironmentNo = 0;

#define LOAD_ETS(NM, TYPE) ets.NM = local_data.get##TYPE(#NM, ets.NM)
      // LOAD_ETS(forceWeather, Bool);
      // LOAD_ETS(forceGeoDate, Bool);
      // LOAD_ETS(forceTime, Bool);
      // LOAD_ETS(forceSeed, Bool);
      LOAD_ETS(latitude, Real);
      LOAD_ETS(yyyy, Int);
      LOAD_ETS(mm, Int);
      LOAD_ETS(dd, Int);
      LOAD_ETS(timeOfDay, Real);
      LOAD_ETS(rndSeed, Int);
#undef LOAD_ETS
    }
    else
      loadBlk(false, blk.getStr("enviName", blk.getStr("selected", "day")));
    envParams.znzfScale = local_data.getReal("znzfScale", 1.0);
    setSettingsToPlugins();
  }

  const DataBlock *panelStateBlk = local_data.getBlockByName("panel_state");
  if (panelStateBlk)
    mainPanelState.setFrom(panelStateBlk);

  fillPanel();

  PropPanel::CurveColorDialog::loadPresets(::make_full_path(sgg::get_exe_path(), "../.local/color_presets.blk"));
}


//==============================================================================
void EnvironmentPlugin::loadBlk(bool ps3, const char *selectedName)
{
  clear_and_shrink(environmentNamesList);
  selectedEnvironmentNo = -1;

  if (ps3)
  {
    commonFileName = commonFileNamePs3;
    levelFileName = levelFileNamePs3;
  }
  else
  {
    commonFileName = commonFileNamePcXbox;
    levelFileName = levelFileNamePcXbox;
  }

  G_ASSERT(!commonFileName.empty());
  String commonFilePath(DAGORED2->getSdkDir());
  commonFilePath.append("/" + commonFileName);
  commonBlk.load(commonFilePath);

  G_ASSERT(!levelFileName.empty());
  String levelFilePath(DAGORED2->getSdkDir());
  levelFilePath.append("/" + levelFileName);
  levelBlk.load(levelFilePath);

  int selectedNo = -1;

  for (unsigned int blockNo = 0; blockNo < levelBlk.blockCount(); blockNo++)
  {
    const char *blockName = levelBlk.getBlock(blockNo)->getBlockName();

    if (!stricmp(blockName, "shader_vars"))
      continue;

    if (!stricmp(blockName, selectedName))
      selectedNo = environmentNamesList.size();

    environmentNamesList.push_back() = blockName;
  }

  if (selectedNo != -1)
    selectedEnvironmentNo = selectedNo;
}


void EnvironmentPlugin::actObjects(float dt)
{
  if (skiesSrv)
  {
    skiesSrv->updateSkies(dt);
    if (needSkiesUpdate && selectedWeatherPreset >= 0 && selectedWeatherType >= 0 && selectedEnvironmentNo >= 0)
    {
      setSettingsToPlugins();
      needSkiesUpdate = false;
    }
  }

  if (windSrv)
    windSrv->update();
}
void EnvironmentPlugin::beforeRenderObjects(IGenViewportWnd *renderVp)
{
  if (skiesSrv)
  {
    skiesSrv->setZnZfScale(envParams.znzfScale);
    float zn = currentEnvironmentAces.envParams.z_near, zf = currentEnvironmentAces.envParams.z_far;
    skiesSrv->getZRange(currentEnvironmentAces.envParams.z_near, currentEnvironmentAces.envParams.z_far);
    if (propPanel && (zn != currentEnvironmentAces.envParams.z_near || zf != currentEnvironmentAces.envParams.z_far))
    {
      propPanel->setCaption(CM_PID_SCENE_ZNEAR, String(0, "Z near:  %.3f", currentEnvironmentAces.envParams.z_near));
      propPanel->setCaption(CM_PID_SCENE_ZFAR, String(0, "Z far:    %.0f", currentEnvironmentAces.envParams.z_far));
    }
  }

  if (renderVp && !renderVp->isOrthogonal())
  {
    Driver3dPerspective p;
    d3d::getpersp(p);
    d3d::setpersp(Driver3dPerspective(p.wk, p.hk, !isAcesPlugin ? envParams.z_near : currentEnvironmentAces.envParams.z_near,
      !isAcesPlugin ? envParams.z_far : currentEnvironmentAces.envParams.z_far, 0, 0));
  }

  static float old_lfd = 0, old_bfd = 0;
  int lfd_id, bfd_id;
  lfd_id = get_shader_variable_id("layered_fog_density", true);
  bfd_id = get_shader_variable_id("blue_fog_density", true);

  if (lfd_id != -1 && bfd_id != -1)
  {
    lfd_id = ShaderGlobal::get_glob_var_id(lfd_id);
    bfd_id = ShaderGlobal::get_glob_var_id(bfd_id);

    if (renderVp && renderVp->isOrthogonal())
    {
      if (old_lfd == 0 && old_bfd == 0)
      {
        old_lfd = ShaderGlobal::get_real_fast(lfd_id);
        old_bfd = ShaderGlobal::get_real_fast(bfd_id);
      }

      ShaderGlobal::set_real_fast(lfd_id, 0.f);
      ShaderGlobal::set_real_fast(bfd_id, 0.f);
    }
    else if (old_lfd != 0 || old_bfd != 0)
    {
      ShaderGlobal::set_real_fast(lfd_id, old_lfd);
      ShaderGlobal::set_real_fast(bfd_id, old_bfd);
      old_lfd = old_bfd = 0;
    }
  }
}


void EnvironmentPlugin::prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit)
{
  if (!renderEnvi)
    return;
  renderGeometry(STG_RENDER_ENVI);
  renderGeometry(STG_RENDER_CLOUDS);
}


void EnvironmentPlugin::renderGeometry(Stage stage)
{
  if (isAcesPlugin && selectedEnvironmentNo == -1)
    return;

  if (IGenViewportWnd *vp = DAGORED2->getRenderViewport())
    if (vp->isOrthogonal())
    {
      if (isAcesPlugin && skiesSrv && stage == STG_BEFORE_RENDER)
        skiesSrv->beforeRenderSkyOrtho();
      return;
    }

  switch (stage)
  {
    case STG_BEFORE_RENDER:
      if (skiesSrv)
        skiesSrv->beforeRenderSky();

      if (ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>())
        ltSrv->updateShaderVars();
      break;

    case STG_RENDER_CLOUDS:
      if (!isAcesPlugin || !skiesSrv)
        break;
      // intended fallthrough
    case STG_RENDER_ENVI:
      int l, t, w, h;
      float minZ, maxZ;

      d3d::getview(l, t, w, h, minZ, maxZ);
      d3d::setview(l, t, w, h, 1, 1);

      TMatrix vtm;
      d3d::gettm(TM_VIEW, vtm);
      TMatrix tm = vtm;

      if (!isAcesPlugin)
      {
        if (VariableMap::isGlobVariablePresent(skyWorldXVarId))
        {
          TMatrix worldMat = rotyTM(DegToRad(envParams.rotY));

          ShaderGlobal::set_color4_fast(skyWorldXVarId, worldMat[0][0], worldMat[0][1], worldMat[0][2], 0.f);

          ShaderGlobal::set_color4_fast(skyWorldYVarId, worldMat[1][0], worldMat[1][1], worldMat[1][2], 0.f);

          ShaderGlobal::set_color4_fast(skyWorldZVarId, worldMat[2][0], worldMat[2][1], worldMat[2][2], 0.f);
        }
        else
        {
          tm = tm * rotyTM(DegToRad(envParams.rotY));
        }
      }

      tm.setcol(3, Point3(0.f, 0.f, 0.f));
      d3d::settm(TM_VIEW, tm);

      if (!isAcesPlugin)
        ShaderGlobal::set_real(skyScaleVarId, envParams.skyHdrMultiplier);

      geom.render();
      d3d::settm(TM_VIEW, vtm);
      d3d::setview(l, t, w, h, minZ, maxZ);

      if (skiesSrv)
      {
        if (stage == STG_RENDER_ENVI)
          skiesSrv->renderSky();
        else if (stage == STG_RENDER_CLOUDS)
          skiesSrv->renderClouds();
      }
      break;
  }
}

void EnvironmentPlugin::renderObjectsToViewport(IGenViewportWnd *vp)
{
  if (isAcesPlugin && selectedEnvironmentNo == -1)
    return;
  if (vp->isOrthogonal())
    return;

  real sunAzimuth = ltService->getSunEx(0).azimuth;
  real sunZenith = ltService->getSunEx(0).zenith;

  Point3 sunDir = -Point3(cosf(HALFPI - sunAzimuth) * cosf(sunZenith), sinf(sunZenith), sinf(HALFPI - sunAzimuth) * cosf(sunZenith));

  Point3 pos = ::grs_cur_view.pos - sunDir * 50;


  Point2 cp;
  if (vp->worldToClient(pos, cp))
  {
    StdGuiRender::set_color(COLOR_WHITE);
    StdGuiRender::draw_line(cp.x, cp.y - 20, cp.x, cp.y + 20);
    StdGuiRender::draw_line(cp.x - 20, cp.y, cp.x + 20, cp.y);

    cp += Point2(1, 1);
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_line(cp.x, cp.y - 20, cp.x, cp.y + 20);
    StdGuiRender::draw_line(cp.x - 20, cp.y, cp.x + 20, cp.y);
  }

  sunAzimuth = ltService->getSunEx(1).azimuth;
  sunZenith = ltService->getSunEx(1).zenith;
  if (isAcesPlugin)
    return;

  sunDir = -Point3(cosf(HALFPI - sunAzimuth) * cosf(sunZenith), sinf(sunZenith), sinf(HALFPI - sunAzimuth) * cosf(sunZenith));

  pos = ::grs_cur_view.pos - sunDir * 50;

  if (vp->worldToClient(pos, cp))
  {
    StdGuiRender::set_color(COLOR_WHITE);
    StdGuiRender::draw_line(cp.x - 20, cp.y - 20, cp.x + 20, cp.y + 20);
    StdGuiRender::draw_line(cp.x + 20, cp.y - 20, cp.x - 20, cp.y + 20);

    cp += Point2(2, 1);
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_line(cp.x - 20, cp.y - 20, cp.x + 20, cp.y + 20);
    cp.y -= 1;
    StdGuiRender::draw_line(cp.x + 20, cp.y - 20, cp.x - 20, cp.y + 20);
  }
}
void EnvironmentPlugin::renderTransObjects()
{
  if (isAcesPlugin && selectedEnvironmentNo == -1)
    return;
  // renderColorTex(); // DEBUG
}

void EnvironmentPlugin::updateImgui()
{
  if (DAGORED2->curPlugin() == this)
  {
    if (propPanel)
    {
      bool open = true;
      DAEDITOR3.imguiBegin(*propPanel, &open);
      propPanel->updateImgui();
      DAEDITOR3.imguiEnd();

      if (!open && propPanel)
      {
        showPanel();
        EDITORCORE->managePropPanels();
      }
    }
  }
}

bool EnvironmentPlugin::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

bool EnvironmentPlugin::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}

bool EnvironmentPlugin::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}

bool EnvironmentPlugin::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}

bool EnvironmentPlugin::buildLmEnviScene(ILogWriter &rep, PropPanel::ContainerPropertyControl *panel, unsigned target)
{
  if (isAcesPlugin)
    return true;

  String prj;
  DAGORED2->getProjectFolderPath(prj);

  const String lmsEnviDat(DAGORED2->getPluginFilePath(this, "envi.lms.dat"));
  const String lmEnviDir = ::make_full_path(prj, "builtScene/envi/");
  const String enviDag(DAGORED2->getPluginFilePath(this, ENVI_DAG));

  CoolConsole &con = DAGORED2->getConsole();
  con.startProgress();

  if (exportLms(enviDag, lmsEnviDat, con))
  {
    StaticSceneBuilder::LightmappedScene scene;

    if (scene.load(lmsEnviDat, con))
    {
      if (!dd_dir_exist(lmEnviDir))
      {
        ::dd_mkdir(lmEnviDir);
        con.addMessage(ILogWriter::WARNING, "'%s' folder was created", ::make_full_path(prj, "builtScene/envi"));
      }

      StaticSceneBuilder::LightingProvider lighting;
      StaticSceneBuilder::StdTonemapper toneMapper;
      StaticSceneBuilder::Splitter splitter;
      splitter.combineObjects = false;
      splitter.splitObjects = false;

      if (!build_and_save_scene1(scene, lighting, toneMapper, splitter, lmEnviDir, StaticSceneBuilder::SCENE_FORMAT_LdrTgaDds, target,
            con, con, true))
      {
        con.endProgress();
        con.endLog();
        if (wingw::message_box(wingw::MBS_YESNO, "Build scene report", "Build scene with errors.\nSee console for details.") ==
            wingw::MB_ID_YES)
          con.showConsole(true);
        return false;
      }
    }
    con.endProgress();
    return true;
  }
  return false;
}

bool EnvironmentPlugin::exportLms(const char *dag_file, const char *lms_file, CoolConsole &con)
{
  if (isAcesPlugin)
    return true;

  debug(">> exporting LMS to %s", lms_file);

  StaticGeometryContainer envi_geom;

  if (!dd_file_exist(dag_file))
  {
    con.addMessage(ILogWriter::WARNING, "Environment scene '%s' not specified", dag_file);
    return false;
  }

  if (!envi_geom.loadFromDag(dag_file, &con))
  {
    con.addMessage(ILogWriter::ERROR, "Can't load environment from dag");
    return false;
  }

  QuietProgressIndicator qpi;
  if (!StaticSceneBuilder::export_envi_to_LMS1(envi_geom, lms_file, qpi, con))
  {
    con.addMessage(ILogWriter::ERROR, "Error saving lightmapped envi");
    return false;
  }

  return true;
}


void EnvironmentPlugin::fillExportPanel(PropPanel::ContainerPropertyControl &params) {}


bool EnvironmentPlugin::validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params)
{
  if (isAcesPlugin)
    return true;

  String prj;
  DAGORED2->getProjectFolderPath(prj);
  if (envParams.exportEnviScene && buildLmEnviScene(rep, params, target))
    rep.addMessage(ILogWriter::NOTE, prj + " environment was built automatically.");
  return true;
}


bool EnvironmentPlugin::addUsedTextures(ITextureNumerator &tn)
{
  if (isAcesPlugin)
    return true;

  String prj;
  DAGORED2->getProjectFolderPath(prj);
  if (envParams.exportEnviScene && check_built_scene_data_exist(prj + "/builtScene/envi", _MAKE4C('PC')))
  {
    add_built_scene_textures(prj + "/builtScene/envi", tn);
    dd_erase(prj + "/builtScene/envi");
  }
  return true;
}


bool EnvironmentPlugin::buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *params)
{
  if (isAcesPlugin)
    return true;

  String prj;
  DAGORED2->getProjectFolderPath(prj);

  // general scene settings
  cwr.beginTaggedBlock(_MAKE4C('ENVP'));
  cwr.writeInt32e(envParams.fog.used);
  cwr.writeInt32e(envParams.fog.exp2);
  cwr.writeReal(envParams.fog.skyDensity);
  cwr.writeE3dColorE(envParams.fog.skyColor);
  cwr.writeReal(envParams.z_near);
  cwr.writeReal(envParams.z_far);
  cwr.writeReal(envParams.skyHdrMultiplier);

  if (envParams.fog.useSunFog)
  {
    cwr.writeReal(envParams.fog.sunDensity);
    cwr.writeE3dColorE(envParams.fog.sunColor);
    cwr.writeReal(sunAzimuth);
  }
  else
  {
    cwr.writeReal(envParams.fog.skyDensity);
    cwr.writeE3dColorE(envParams.fog.skyColor);
    cwr.writeReal(0);
  }

  cwr.writeReal(DegToRad(envParams.rotY));

  cwr.writeE3dColorE(envParams.fog.lfogSkyColor);
  cwr.writeE3dColorE(envParams.fog.lfogSunColor);
  cwr.writeReal(envParams.fog.lfogDensity);
  cwr.writeReal(envParams.fog.lfogMinHeight);
  cwr.writeReal(envParams.fog.lfogMaxHeight);

  cwr.align8();
  cwr.endBlock();

  if (envParams.exportEnviScene && check_built_scene_data_exist(prj + "/builtScene/envi", cwr.getTarget()))
  {
    cwr.beginTaggedBlock(_MAKE4C('ENVI'));
    store_built_scene_data(prj + "/builtScene/envi", cwr, tn);
    cwr.align8();
    cwr.endBlock();
  }

  cwr.beginTaggedBlock(_MAKE4C('POFX'));
  mkbindump::write_ro_datablock(cwr, postfxBlk);
  cwr.align8();
  cwr.endBlock();

  cwr.beginTaggedBlock(_MAKE4C('ShGV'));
  mkbindump::write_ro_datablock(cwr, shgvBlk);
  cwr.align8();
  cwr.endBlock();

  cwr.beginTaggedBlock(_MAKE4C('DirL'));

  // Sky
  cwr.write32ex(&ltService->getSky().ambCol, sizeof(Color3));

  // Sun 0.
  Point3 ltDir = -ltService->getSunEx(0).ltDir;
  cwr.write32ex(&ltDir, sizeof(Point3));
  cwr.write32ex(&ltService->getSunEx(0).ltCol, sizeof(Color3));


  // Sun 1.
  ltDir = -ltService->getSunEx(1).ltDir;
  cwr.write32ex(&ltDir, sizeof(Point3));
  cwr.write32ex(&ltService->getSunEx(1).ltCol, sizeof(Color3));


  cwr.endBlock();

  return true;
}


bool EnvironmentPlugin::checkMetrics(const DataBlock &metrics_blk)
{
  String proj;
  DAGORED2->getProjectFolderPath(proj);
  CoolConsole &con = DAGORED2->getConsole();

  String path = ::make_full_path(proj, "builtScene/envi/scene-PC.scn");
  String lmDir = ::make_full_path(proj, "builtScene/envi");

  const real zNearMin = metrics_blk.getReal("znear_min", METRIC_ZNEAR_MIN);
  const real zNearMax = metrics_blk.getReal("znear_max", METRIC_ZNEAR_MAX);
  const real zFarMin = metrics_blk.getReal("zfar_min", METRIC_ZFAR_MIN);
  const real zFarMax = metrics_blk.getReal("zfar_max", METRIC_ZFAR_MAX);
  const real zMinFarRatioMax = metrics_blk.getReal("z_ratio_max", METRIC_ZNEAR_ZFAR_MAX_RATIO);

  real zNear = !isAcesPlugin ? envParams.z_near : currentEnvironmentAces.envParams.z_near;
  real zFar = !isAcesPlugin ? envParams.z_far : currentEnvironmentAces.envParams.z_far;
  if (zNear < zNearMin)
  {
    con.addMessage(ILogWriter::ERROR, "Z-near value (%g) less then minimum allowed (%g)", zNear, zNearMin);
    return false;
  }

  if (zNear > zNearMax)
  {
    con.addMessage(ILogWriter::ERROR, "Z-near value (%g) more then maximum allowed (%g)", zNear, zNearMax);
    return false;
  }

  if (zFar < zFarMin)
  {
    con.addMessage(ILogWriter::ERROR, "Z-far value (%g) less then maximum allowed (%g)", zFar, zFarMin);
    return false;
  }

  if (zFar > zFarMax)
  {
    con.addMessage(ILogWriter::ERROR, "Z-far value (%g) more then maximum allowed (%g)", zFar, zFarMax);
    return false;
  }

  const real zRatio = (zFar - zNear) / zNear;

  if (zRatio > zMinFarRatioMax)
  {
    con.addMessage(ILogWriter::ERROR, "Z-far to Z-near ratio (%g) more then maximum allowed (%g)", zRatio, zMinFarRatioMax);
    return false;
  }

  const int maxFaces = metrics_blk.getInt("max_faces", METRIC_MAX_FACES);

  return true;
}


// property panel handling
void EnvironmentPlugin::showPanel() { recreatePanel(propPanel, PROPBAR_ENVIRONMENT_WTYPE); }

void EnvironmentPlugin::fillPanel()
{
  if (!propPanel)
    return;
  propPanel->clear();
  propPanel->disableFillAutoResize();

  PropPanel::ContainerPropertyControl *grp = NULL;

  if (isAcesPlugin)
    grp = propPanel->createGroup(CM_PID_ENVIRONMENTS, "Environments");

  if (isAcesPlugin && skiesSrv)
  {
    grp->createCheckBox(CM_PID_ENV_FORCE_WEATHER, "Force selected weather", ets.forceWeather);
    Tab<String> relWeatherPresetsList(tmpmem);
    for (int i = 0; i < weatherPresetsList.size(); i++)
      relWeatherPresetsList.push_back() = dd_get_fname(weatherPresetsList[i]);

    grp->createList(CM_PID_WEATHER_PRESETS_LIST, "Weather presets", relWeatherPresetsList, selectedWeatherPreset);
    grp->createList(CM_PID_WEATHER_TYPES_LIST, "Weather type", weatherTypesList, selectedWeatherType);
  }

  if (isAcesPlugin)
  {
    grp->createList(CM_PID_ENVIRONMENTS_LIST, "Environments list", environmentNamesList, selectedEnvironmentNo);
    if (selectedEnvironmentNo != -1)
    {
      selectedEnvironmentNo = -1;
      onChange(CM_PID_ENVIRONMENTS_LIST, propPanel);
    }
  }
  if (isAcesPlugin && skiesSrv)
  {
    grp->createSeparator(0);
    grp->createCheckBox(CM_PID_ENV_FORCE_GEO_DATE, "Force latitude/date", ets.forceGeoDate);
    grp->createTrackFloat(CM_PID_ENV_LATITUDE, "Geo latitude", ets.latitude, -90, +90, 1, ets.forceGeoDate);
    grp->createEditInt(CM_PID_ENV_YEAR, "Year", ets.yyyy, ets.forceGeoDate);
    grp->setMinMaxStep(CM_PID_ENV_YEAR, 1900, 2100, 1);
    grp->createEditInt(CM_PID_ENV_MONTH, "Month", ets.mm, ets.forceGeoDate);
    grp->setMinMaxStep(CM_PID_ENV_MONTH, 1, 12, 1);
    grp->createEditInt(CM_PID_ENV_DAY, "Day of month", ets.dd, ets.forceGeoDate);
    grp->setMinMaxStep(CM_PID_ENV_DAY, 1, 31, 1);
    grp->createSeparator(0);
    grp->createCheckBox(CM_PID_ENV_FORCE_TIME, "Force time of day", ets.forceTime);
    grp->createTrackFloat(CM_PID_ENV_TIME_OF_DAY, "Time of day", ets.timeOfDay, 0, 24, 0.25, ets.forceTime);
    grp->createSeparator(0);
    grp->createCheckBox(CM_PID_ENV_FORCE_SEED, "Force randomization seed", ets.forceSeed);
    grp->createTrackInt(CM_PID_ENV_RND_SEED, "Seed", ets.rndSeed, 0, 1 << 20, 1, ets.rndSeed);
  }

  // Fog
  if (!isAcesPlugin)
  {
    grp = propPanel->createGroup(CM_PID_FOG_GRP, "Fog");
    G_ASSERT(grp);

    grp->createCheckBox(CM_PID_FOG_USE, "Use fog", envParams.fog.used);
    grp->createCheckBox(CM_PID_FOG_EXP2, "Exp2-type fog", envParams.fog.exp2);

    grp->createEditFloat(CM_PID_SKY_FOG_DENSITY,
      "Sky fog density:", !isAcesPlugin ? envParams.fog.skyDensity : currentEnvironmentAces.envParams.fog.skyDensity, 5);
    if (!isAcesPlugin)
      grp->setMinMaxStep(CM_PID_SKY_FOG_DENSITY, MIN_FOG_DENSITY, MAX_FOG_DENSITY, FOG_DENSITY_STEP);
    grp->createColorBox(CM_PID_SKY_FOG_COLOR,
      "Sky fog color:", !isAcesPlugin ? envParams.fog.skyColor : currentEnvironmentAces.envParams.fog.skyColor);

    if (!isAcesPlugin)
      grp->createCheckBox(CM_PID_USE_SUN_FOG, "Use sun fog", envParams.fog.useSunFog);

    grp->createEditFloat(CM_PID_SUN_FOG_DENSITY,
      "Sun fog density:", !isAcesPlugin ? envParams.fog.sunDensity : currentEnvironmentAces.envParams.fog.sunDensity, 5,
      !isAcesPlugin ? envParams.fog.useSunFog : true);
    if (!isAcesPlugin)
      grp->setMinMaxStep(CM_PID_SUN_FOG_DENSITY, MIN_FOG_DENSITY, MAX_FOG_DENSITY, FOG_DENSITY_STEP);
    grp->createColorBox(CM_PID_SUN_FOG_COLOR,
      "Sun fog color:", !isAcesPlugin ? envParams.fog.sunColor : currentEnvironmentAces.envParams.fog.sunColor,
      !isAcesPlugin ? envParams.fog.useSunFog : true);

    grp->createTrackFloat(CM_PID_SUN_AZIMUTH, "Sun azimuth:", RadToDeg(HALFPI - sunAzimuth), -180, 180, 0.01f);
    grp->createButton(CM_PID_GET_AZIMUTH_FROM_LIGHT, "Get aizmuth from \"Light\" plugin");

    grp->createIndent();
    grp->createCheckBox(CM_PID_PREVIEW_FOG, "Preview fog settings", previewFog);
    grp->createIndent();

    grp->createColorBox(CM_PID_LFOG_COL1, "Layered fog sky color:", envParams.fog.lfogSkyColor);
    grp->createColorBox(CM_PID_LFOG_COL2, "Layered fog sun color:", envParams.fog.lfogSunColor);
    grp->createEditFloat(CM_PID_LFOG_DENSITY, "Layered fog density:", envParams.fog.lfogDensity, 4);
    grp->createEditFloat(CM_PID_LFOG_MIN_HEIGHT, "Layer fog min height:", envParams.fog.lfogMinHeight, 4);
    grp->createEditFloat(CM_PID_LFOG_MAX_HEIGHT, "Layer fog max height:", envParams.fog.lfogMaxHeight, 4);

    propPanel->setBool(CM_PID_FOG_GRP, true);
  }

  // Scene
  grp = propPanel->createGroup(CM_PID_SCENE_GRP, "Scene");
  G_ASSERT(grp);

  if (isAcesPlugin && skiesSrv)
  {
    grp->createStatic(CM_PID_SCENE_ZNEAR, String(0, "Z near:  %.3f", currentEnvironmentAces.envParams.z_near));
    grp->createStatic(CM_PID_SCENE_ZFAR, String(0, "Z far:    %.0f", currentEnvironmentAces.envParams.z_far));
  }
  else
  {
    grp->createEditFloat(CM_PID_SCENE_ZNEAR, "Z near:", !isAcesPlugin ? envParams.z_near : currentEnvironmentAces.envParams.z_near, 3,
      !isAcesPlugin);
    grp->createEditFloat(CM_PID_SCENE_ZFAR, "Z far:", !isAcesPlugin ? envParams.z_far : currentEnvironmentAces.envParams.z_far, 3,
      !isAcesPlugin);
  }

  if (!isAcesPlugin)
  {
    grp->createTrackFloat(CM_PID_SCENE_ROTY, "Rotate:", envParams.rotY, 0, 360, 0.01f);
    grp->createCheckBox(CM_PID_SCENE_EXPORT, "Export ENVI scene", envParams.exportEnviScene);
  }
  else
  {
    grp->createTrackFloat(CM_PID_ZNZF_SCALE, "Z near/Z far range scale", envParams.znzfScale, 0.1, 20, 0.05);
  }

  propPanel->setBool(CM_PID_SCENE_GRP, true);

  // Sun
  grp = propPanel->createGroup(CM_PID_SUN_GRP, "Sun");
  G_ASSERT(grp);

  grp->createTrackFloat(CM_PID_SUN_GRP_AZIMUTH, "Azimuth:", RadToDeg(ltService->getSunEx(0).azimuth), -360, 360, 0.01, !isAcesPlugin);
  grp->createTrackFloat(CM_PID_SUN_GRP_ZENITH, "Zenith:", RadToDeg(ltService->getSunEx(0).zenith), -90, 90, 0.01, !isAcesPlugin);

  propPanel->setBool(CM_PID_SUN_GRP, true);

  if (!isAcesPlugin)
  {
    // Backlight
    grp = propPanel->createGroup(CM_PID_SUN2_GRP, "Backlight");
    G_ASSERT(grp);

    grp->createTrackFloat(CM_PID_SUN2_GRP_AZIMUTH, "Azimuth:", RadToDeg(ltService->getSunEx(1).azimuth), -180, 180, 0.01);
    grp->createTrackFloat(CM_PID_SUN2_GRP_ZENITH, "Zenith:", RadToDeg(ltService->getSunEx(1).zenith), -90, 90, 0.01);

    propPanel->setBool(CM_PID_SUN2_GRP, true);

    // Sky
    grp = propPanel->createGroup(CM_PID_SKY_GRP, "Sky");
    G_ASSERT(grp);

    grp->createEditFloat(CM_PID_SKY_HDR_MULTIPLIER, "HDR multiplier:", envParams.skyHdrMultiplier, 4);

    propPanel->setBool(CM_PID_SKY_GRP, true);

    // Direct lighting
    grp = propPanel->createGroup(PID_DIRECT_LIGHTING, "Direct lighting");
    G_ASSERT(grp);

    bool syncDir = ltService->getSyncLtDirs();
    bool syncCol = ltService->getSyncLtColors();

    grp->createCheckBox(PID_SYNC_DIR_LIGHT_DIR, "Automatic sync directions", syncDir);

    if (!isAcesPlugin)
      grp->createCheckBox(PID_SYNC_DIR_LIGHT_COLOR, "Automatic sync colors", syncCol);

    E3DCOLOR col;
    real mul;

    col = !isAcesPlugin ? color4ToE3dcolor(::color4(ltService->getSunEx(0).ltCol, 1.0), mul) : ltService->getSunEx(0).ltColUser;
    mul = !isAcesPlugin ? mul : ltService->getSunEx(0).ltColMulUser;

    grp->createSimpleColor(PID_DIR_SUN1_COLOR, "Sun #1 color:", col, !syncCol);
    grp->createEditFloat(PID_DIR_SUN1_COLOR_MUL, "Sun #1 color multiplier:", mul, 4, !syncCol);

    col = !isAcesPlugin ? color4ToE3dcolor(::color4(ltService->getSunEx(1).ltCol, 1.0), mul) : ltService->getSunEx(1).ltColUser;
    mul = !isAcesPlugin ? mul : ltService->getSunEx(1).ltColMulUser;

    grp->createSimpleColor(PID_DIR_SUN2_COLOR, "Sun #2 color:", col, !syncCol);
    grp->createEditFloat(PID_DIR_SUN2_COLOR_MUL, "Sun #2 color multiplier:", mul, 4, !syncCol);

    col = color4ToE3dcolor(::color4(ltService->getSky().ambCol, 1.0), mul);

    grp->createSimpleColor(PID_DIR_SKY_COLOR, "Sky color:", col, !syncCol);
    grp->createEditFloat(PID_DIR_SKY_COLOR_MUL, "Sky color multiplier:", mul, 4, !syncCol);

    grp->createPoint3(PID_DIR_SUN1_DIR, "Sun #1 direction:", ltService->getSunEx(0).ltDir, 4, !syncDir);
    grp->createPoint3(PID_DIR_SUN2_DIR, "Sun #2 direction:", ltService->getSunEx(1).ltDir, 4, !syncDir);

    propPanel->setBool(PID_DIRECT_LIGHTING, true);
  }

  propPanel->restoreFillAutoResize();
  propPanel->loadState(mainPanelState);
  // propPanel->createButton(CM_PID_DEFAULTS, "Reset to defaults");
}


void EnvironmentPlugin::updateLight()
{
  if (!propPanel)
    return;

  // sun 1

  propPanel->setFloat(CM_PID_SUN_GRP_AZIMUTH, RadToDeg(ltService->getSunEx(0).azimuth));

  propPanel->setFloat(CM_PID_SUN_GRP_ZENITH, RadToDeg(ltService->getSunEx(0).zenith));

  // direct lighting

  E3DCOLOR col;
  real mul;

  if (ltService->getSyncLtColors())
  {
    col = color4ToE3dcolor(::color4(ltService->getSunEx(0).ltCol, 1.0), mul);
    propPanel->setColor(PID_DIR_SUN1_COLOR, col);
    propPanel->setFloat(PID_DIR_SUN1_COLOR_MUL, mul);

    col = color4ToE3dcolor(::color4(ltService->getSky().ambCol, 1.0), mul);
    propPanel->setColor(PID_DIR_SKY_COLOR, col);
    propPanel->setFloat(PID_DIR_SKY_COLOR_MUL, mul);
  }

  if (ltService->getSyncLtDirs())
  {
    propPanel->setPoint3(PID_DIR_SUN1_DIR, ltService->getSunEx(0).ltDir);
  }

  if (isAcesPlugin)
  {
    propPanel->setColor(CM_PID_SUN_FOG_COLOR, currentEnvironmentAces.envParams.fog.sunColor);
    propPanel->setFloat(CM_PID_SUN_FOG_DENSITY, currentEnvironmentAces.envParams.fog.sunDensity);
    propPanel->setColor(CM_PID_SKY_FOG_COLOR, currentEnvironmentAces.envParams.fog.skyColor);
    propPanel->setFloat(CM_PID_SKY_FOG_DENSITY, currentEnvironmentAces.envParams.fog.skyDensity);

    propPanel->setFloat(CM_PID_SCENE_ZNEAR, currentEnvironmentAces.envParams.z_near);
    propPanel->setFloat(CM_PID_SCENE_ZFAR, currentEnvironmentAces.envParams.z_far);

    propPanel->setFloat(CM_PID_SUN2_GRP_AZIMUTH, RadToDeg(ltService->getSunEx(1).azimuth));
    propPanel->setFloat(CM_PID_SUN2_GRP_ZENITH, RadToDeg(ltService->getSunEx(1).zenith));


    propPanel->setBool(PID_SYNC_DIR_LIGHT_DIR, ltService->getSyncLtDirs());
    propPanel->setColor(PID_DIR_SUN1_COLOR, ltService->getSunEx(0).ltColUser);
    propPanel->setFloat(PID_DIR_SUN1_COLOR_MUL, ltService->getSunEx(0).ltColMulUser);
    propPanel->setColor(PID_DIR_SUN2_COLOR, ltService->getSunEx(1).ltColUser);
    propPanel->setFloat(PID_DIR_SUN2_COLOR_MUL, ltService->getSunEx(1).ltColMulUser);
    propPanel->setColor(PID_DIR_SKY_COLOR, ltService->getSky().ambColUser);
    propPanel->setFloat(PID_DIR_SKY_COLOR_MUL, ltService->getSky().ambColMulUser);
    propPanel->setPoint3(PID_DIR_SUN1_DIR, ltService->getSunEx(0).ltDir);
    propPanel->setPoint3(PID_DIR_SUN2_DIR, ltService->getSunEx(1).ltDir);
  }
}


void *EnvironmentPlugin::onWmCreateWindow(int type)
{
  switch (type)
  {
    case PROPBAR_ENVIRONMENT_WTYPE:
    {
      if (propPanel)
        return nullptr;

      propPanel = IEditorCoreEngine::get()->createPropPanel(this, "Properties");
      fillPanel();

      PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
      if (toolbar)
        toolbar->setBool(CM_SHOW_PANEL, true);

      return propPanel;
    }
    break;

    case POSTFX_ENVIRONMENT_WTYPE:
    {
      if (postfxPanel)
        return nullptr;

      postfxPanel = new PostfxPanel(*this, nullptr);
      postfxPanel->fillPanel();

      PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
      if (toolbar)
        toolbar->setBool(CM_SHOW_POSTFX_PANEL, true);

      return postfxPanel;
    }
    break;

    case SHADERGV_ENVIRONMENT_WTYPE:
    {
      if (shgvPanel)
        return nullptr;

      shgvPanel = new ShaderGlobVarsPanel(*this, nullptr);
      shgvPanel->fillPanel();

      PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
      if (toolbar)
        toolbar->setBool(CM_SHOW_SHGV_PANEL, true);

      return shgvPanel;
    }
    break;
  }

  return nullptr;
}


bool EnvironmentPlugin::onWmDestroyWindow(void *window)
{
  if (window == propPanel)
  {
    mainPanelState.reset();
    propPanel->saveState(mainPanelState);

    del_it(propPanel);
    PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
    if (toolbar)
      toolbar->setBool(CM_SHOW_PANEL, false);
    return true;
  }

  if (window == postfxPanel)
  {
    del_it(postfxPanel);
    PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
    if (toolbar)
      toolbar->setBool(CM_SHOW_POSTFX_PANEL, false);
    return true;
  }

  if (window == shgvPanel)
  {
    del_it(shgvPanel);
    PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
    if (toolbar)
      toolbar->setBool(CM_SHOW_SHGV_PANEL, false);
    return true;
  }

  return false;
}


void EnvironmentPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (onPluginMenuClickInternal(pcb_id, panel))
    return;

  switch (pcb_id)
  {
      // case CM_PID_DEFAULTS:
      //{
      //   setFogDefaults();
      //   fillPanel();
      // }
      // break;

    case CM_PID_GET_AZIMUTH_FROM_LIGHT:
    {
      const real sunAzimuthDeg = ltService->getSunEx(0).azimuth * RAD_TO_DEG;

      if (propPanel)
        propPanel->setFloat(CM_PID_SUN_AZIMUTH, sunAzimuthDeg);
    }
    break;
  }
}

void EnvironmentPlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
#define CASE_SET_VAL(id, val, func) \
  case id: val = panel->func(id); break;

  switch (pcb_id)
  {
    case CM_PID_WEATHER_PRESETS_LIST:
    case CM_PID_WEATHER_TYPES_LIST:
    case CM_PID_ENVIRONMENTS_LIST:
    {
      if (selectedEnvironmentNo >= 0 && selectedWeatherPreset >= 0 && selectedWeatherPreset >= 0)
        getSettingsFromPlugins();

      if (pcb_id == CM_PID_WEATHER_PRESETS_LIST)
        selectedWeatherPreset = propPanel->getInt(pcb_id);
      else if (pcb_id == CM_PID_WEATHER_TYPES_LIST)
        selectedWeatherType = propPanel->getInt(pcb_id);
      else if (pcb_id == CM_PID_ENVIRONMENTS_LIST)
        selectedEnvironmentNo = propPanel->getInt(pcb_id);
      setSettingsToPlugins();

      updateLight();

      if (postfxPanel)
        recreatePostfxPanel();
      if (shgvPanel)
        recreateShGVPanel();
      break;
    }

      CASE_SET_VAL(CM_PID_SCENE_ROTY, envParams.rotY, getFloat)
      CASE_SET_VAL(CM_PID_SCENE_EXPORT, envParams.exportEnviScene, getBool)
      CASE_SET_VAL(CM_PID_PREVIEW_FOG, previewFog, getBool)

    case CM_PID_SUN_AZIMUTH: sunAzimuth = HALFPI - DegToRad(panel->getFloat(pcb_id)); break;

    case CM_PID_SUN_GRP_AZIMUTH:
      if (!ltService->getSun(0))
        break;
      ltService->getSun(0)->azimuth = DegToRad(panel->getFloat(pcb_id));
      onLightingSettingsChanged(false);
      break;

    case CM_PID_SUN_GRP_ZENITH:
      if (!ltService->getSun(0))
        break;
      ltService->getSun(0)->zenith = DegToRad(panel->getFloat(pcb_id));
      onLightingSettingsChanged(false);
      break;

    case CM_PID_SUN2_GRP_AZIMUTH:
      if (!ltService->getSun(1))
        break;
      ltService->getSun(1)->azimuth = DegToRad(panel->getFloat(pcb_id));
      onLightingSettingsChanged(false);
      break;

    case CM_PID_SUN2_GRP_ZENITH:
      if (!ltService->getSun(1))
        break;
      ltService->getSun(1)->zenith = DegToRad(panel->getFloat(pcb_id));
      onLightingSettingsChanged(false);
      break;

    case PID_SYNC_DIR_LIGHT_DIR:
    {
      bool syncDir = panel->getBool(pcb_id);
      panel->setEnabledById(PID_DIR_SUN1_DIR, !syncDir);
      panel->setEnabledById(PID_DIR_SUN2_DIR, !syncDir);

      ltService->setSyncLtDirs(syncDir);
      if (syncDir)
      {
        ltService->updateShaderVars();
        // fillPanel();
      }
      onLightingSettingsChanged(false);
    }
    break;

    case PID_SYNC_DIR_LIGHT_COLOR:
    {
      bool sync = panel->getBool(pcb_id);
      panel->setEnabledById(PID_DIR_SUN1_COLOR, !sync);
      panel->setEnabledById(PID_DIR_SUN1_COLOR_MUL, !sync);
      panel->setEnabledById(PID_DIR_SUN2_COLOR, !sync);
      panel->setEnabledById(PID_DIR_SUN2_COLOR_MUL, !sync);
      panel->setEnabledById(PID_DIR_SKY_COLOR, !sync);
      panel->setEnabledById(PID_DIR_SKY_COLOR_MUL, !sync);

      ltService->setSyncLtColors(sync);
      if (sync)
      {
        ltService->updateShaderVars();
        // fillPanel();
      }
      onLightingSettingsChanged(false);
    }
    break;

    case PID_DIR_SUN1_DIR:
      if (!ltService->getSun(0))
        break;
      ltService->getSun(0)->ltDir = panel->getPoint3(pcb_id);
      ltService->updateShaderVars();
      onLightingSettingsChanged(false);
      break;

    case PID_DIR_SUN2_DIR:
      if (!ltService->getSun(1))
        break;
      ltService->getSun(1)->ltDir = panel->getPoint3(pcb_id);
      ltService->updateShaderVars();
      onLightingSettingsChanged(false);
      break;

    case PID_DIR_SUN1_COLOR:
    case PID_DIR_SUN1_COLOR_MUL:
    {
      if (!ltService->getSun(0))
        break;

      E3DCOLOR col = panel->getColor(PID_DIR_SUN1_COLOR);
      float mul = panel->getFloat(PID_DIR_SUN1_COLOR_MUL);

      ltService->getSun(0)->ltCol = ::color3(col) * mul;

      if (isAcesPlugin)
      {
        ltService->getSun(0)->ltColUser = col;
        ltService->getSun(0)->ltColMulUser = mul;
      }

      ltService->updateShaderVars();
      onLightingSettingsChanged(false);
    }
    break;

    case PID_DIR_SUN2_COLOR:
    case PID_DIR_SUN2_COLOR_MUL:
    {
      if (!ltService->getSun(1))
        break;

      E3DCOLOR col = panel->getColor(PID_DIR_SUN2_COLOR);
      float mul = panel->getFloat(PID_DIR_SUN2_COLOR_MUL);

      ltService->getSun(1)->ltCol = ::color3(col) * mul;

      if (isAcesPlugin)
      {
        ltService->getSun(1)->ltColUser = col;
        ltService->getSun(1)->ltColMulUser = mul;
      }

      ltService->updateShaderVars();
      onLightingSettingsChanged(false);
    }
    break;

    case PID_DIR_SKY_COLOR:
    case PID_DIR_SKY_COLOR_MUL:
    {
      E3DCOLOR col = panel->getColor(PID_DIR_SKY_COLOR);
      float mul = panel->getFloat(PID_DIR_SKY_COLOR_MUL);

      DEBUG_CP();
      ltService->getSky().ambCol = ::color3(col) * mul;

      ltService->updateShaderVars();
      onLightingSettingsChanged(false);
    }
    break;

      //

      CASE_SET_VAL(CM_PID_FOG_USE, envParams.fog.used, getBool)
      CASE_SET_VAL(CM_PID_FOG_EXP2, envParams.fog.exp2, getBool)
      CASE_SET_VAL(CM_PID_SUN_FOG_DENSITY,
        (!isAcesPlugin ? envParams.fog.sunDensity : currentEnvironmentAces.envParams.fog.sunDensity), getFloat)
      CASE_SET_VAL(CM_PID_SKY_FOG_DENSITY,
        (!isAcesPlugin ? envParams.fog.skyDensity : currentEnvironmentAces.envParams.fog.skyDensity), getFloat)
      CASE_SET_VAL(CM_PID_SUN_FOG_COLOR, (!isAcesPlugin ? envParams.fog.sunColor : currentEnvironmentAces.envParams.fog.sunColor),
        getColor)
      CASE_SET_VAL(CM_PID_SKY_FOG_COLOR, (!isAcesPlugin ? envParams.fog.skyColor : currentEnvironmentAces.envParams.fog.skyColor),
        getColor)
      CASE_SET_VAL(CM_PID_LFOG_COL1, envParams.fog.lfogSkyColor, getColor)
      CASE_SET_VAL(CM_PID_LFOG_COL2, envParams.fog.lfogSunColor, getColor)
      CASE_SET_VAL(CM_PID_LFOG_DENSITY, envParams.fog.lfogDensity, getFloat)
      CASE_SET_VAL(CM_PID_LFOG_MIN_HEIGHT, envParams.fog.lfogMinHeight, getFloat)
      CASE_SET_VAL(CM_PID_LFOG_MAX_HEIGHT, envParams.fog.lfogMaxHeight, getFloat)
      CASE_SET_VAL(CM_PID_SCENE_ZNEAR, (!isAcesPlugin ? envParams.z_near : currentEnvironmentAces.envParams.z_near), getFloat)
      CASE_SET_VAL(CM_PID_SCENE_ZFAR, (!isAcesPlugin ? envParams.z_far : currentEnvironmentAces.envParams.z_far), getFloat)
    case CM_PID_ZNZF_SCALE:
      envParams.znzfScale = panel->getFloat(pcb_id);
      if (skiesSrv)
        skiesSrv->setZnZfScale(envParams.znzfScale);
      break;

      CASE_SET_VAL(CM_PID_SKY_HDR_MULTIPLIER, envParams.skyHdrMultiplier, getFloat)

    case CM_PID_USE_SUN_FOG:
      envParams.fog.useSunFog = panel->getBool(pcb_id);
      panel->setEnabledById(CM_PID_SUN_FOG_DENSITY, envParams.fog.useSunFog);
      panel->setEnabledById(CM_PID_SUN_FOG_COLOR, envParams.fog.useSunFog);
      break;

      CASE_SET_VAL(CM_PID_ENV_FORCE_WEATHER, ets.forceWeather, getBool);
    case CM_PID_ENV_FORCE_GEO_DATE:
      ets.forceGeoDate = panel->getBool(pcb_id);
      panel->setEnabledById(CM_PID_ENV_LATITUDE, ets.forceGeoDate);
      panel->setEnabledById(CM_PID_ENV_YEAR, ets.forceGeoDate);
      panel->setEnabledById(CM_PID_ENV_MONTH, ets.forceGeoDate);
      panel->setEnabledById(CM_PID_ENV_DAY, ets.forceGeoDate);
      break;
    case CM_PID_ENV_FORCE_TIME:
      ets.forceTime = panel->getBool(pcb_id);
      panel->setEnabledById(CM_PID_ENV_TIME_OF_DAY, ets.forceTime);
      break;
    case CM_PID_ENV_FORCE_SEED:
      ets.forceSeed = panel->getBool(pcb_id);
      panel->setEnabledById(CM_PID_ENV_RND_SEED, ets.forceSeed);
      break;
      CASE_SET_VAL(CM_PID_ENV_LATITUDE, ets.latitude, getFloat);
      CASE_SET_VAL(CM_PID_ENV_YEAR, ets.yyyy, getInt);
      CASE_SET_VAL(CM_PID_ENV_MONTH, ets.mm, getInt);
      CASE_SET_VAL(CM_PID_ENV_DAY, ets.dd, getInt);
      CASE_SET_VAL(CM_PID_ENV_TIME_OF_DAY, ets.timeOfDay, getFloat);
      CASE_SET_VAL(CM_PID_ENV_RND_SEED, ets.rndSeed, getInt);
  }

  switch (pcb_id)
  {
    case CM_PID_ENV_FORCE_WEATHER:
    case CM_PID_ENV_FORCE_GEO_DATE:
    case CM_PID_ENV_FORCE_TIME:
    case CM_PID_ENV_FORCE_SEED:
    case CM_PID_ENV_LATITUDE:
    case CM_PID_ENV_YEAR:
    case CM_PID_ENV_MONTH:
    case CM_PID_ENV_DAY:
    case CM_PID_ENV_TIME_OF_DAY:
    case CM_PID_ENV_RND_SEED: updateSunSkyEnvi(); break;
  }

  flushDataBlocksToGame();

#undef CASE_SET_VAL
}


void EnvironmentPlugin::setFogDefaults()
{
  envParams.fog.used = true;
  envParams.fog.exp2 = false;
  envParams.fog.sunDensity = DEF_FOG_DENSITY;
  envParams.fog.skyDensity = DEF_FOG_DENSITY;
  envParams.fog.sunColor = DEF_FOG_COLOR;
  envParams.fog.skyColor = DEF_FOG_COLOR;
  envParams.fog.useSunFog = false;

  envParams.fog.lfogSkyColor = envParams.fog.lfogSunColor = DEF_FOG_COLOR;
  envParams.fog.lfogDensity = 0;
  envParams.fog.lfogMinHeight = 0;
  envParams.fog.lfogMaxHeight = 1;

  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
  const DataBlock &envi_def = *app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi");

  envParams.z_near = envi_def.getReal("znear", 0.10);
  envParams.z_far = envi_def.getReal("zfar", 1000.0);
  envParams.rotY = 0.0;

  envParams.skyHdrMultiplier = 1.;

  if (!DAGORED2->getPluginByName("SceneLighter") && !skiesSrv)
  {
    ltService->setSunCount(0);
    ltService->setSunCount(2);
    ltService->setSyncLtDirs(true);
    ltService->setSyncLtColors(false);
  }
  DAGORED2->repaint();
}


//==================================================================================================
void EnvironmentPlugin::recreatePanel(PropPanel::PanelWindowPropertyControl *panel, int wtype_id)
{
  if (!panel)
    EDITORCORE->addPropPanel(wtype_id, hdpi::_pxScaled(PROPBAR_WIDTH));
  else
    EDITORCORE->removePropPanel(panel);
}


void EnvironmentPlugin::recreatePostfxPanel()
{
  recreatePanel(postfxPanel ? postfxPanel->getPanel() : NULL, POSTFX_ENVIRONMENT_WTYPE);
}


void EnvironmentPlugin::recreateShGVPanel() { recreatePanel(shgvPanel ? shgvPanel->getPanel() : NULL, SHADERGV_ENVIRONMENT_WTYPE); }


//==================================================================================================
void EnvironmentPlugin::hdrViewSettings()
{
  if (skiesSrv)
    return;

  DataBlock *hdrBlk = !isAcesPlugin ? postfxBlk.addBlock("hdr") : currentEnvironmentAces.postfxBlk.addBlock("hdr");

  if (!hdrBlk)
    return;

  HdrViewSettingsDialog dlg(hdrBlk);

  if (dlg.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    const char *hdr_mode = hdrBlk->getStr("mode", "none");
    if (isAcesPlugin)
      loadBlk(strcmp(hdr_mode, "ps3") == 0, "day");

    // Restart HDR
    EDITORCORE->queryEditorInterface<IDynRenderService>()->restartPostfx(!isAcesPlugin ? postfxBlk : currentEnvironmentAces.postfxBlk);

    if (dlg.isLevelReloadRequired())
    {
      Tab<IHDRChangeSettingsClient *> interfaces(tmpmem);
      DAGORED2->getInterfacesEx<IHDRChangeSettingsClient>(interfaces);

      for (int i = 0; i < interfaces.size(); ++i)
        interfaces[i]->onHDRSettingsChanged();
    }

    DAGORED2->repaint();

    // Turn on animate viewports for hdrMode != node
    if (!dlg.isHdrModeNone())
      DAGORED2->setAnimateViewports(true);
  }
}

void EnvironmentPlugin::flushDataBlocksToGame()
{
#if _TARGET_PC_WIN                       // TODO: tools Linux porting: EnvironmentPlugin::flushDataBlocksToGame()
  if (gameHttpServerAddr == INADDR_NONE) // not connected
    return;

  String req;
  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
  int ret;

  SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == INVALID_SOCKET)
    goto update_envi_fail;

  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(HTTP_SERVER_PORT);
  sin.sin_addr.s_addr = gameHttpServerAddr;

  ret = connect(s, (sockaddr *)&sin, sizeof(sin));
  if (ret != 0)
    goto update_envi_fail;

  getSettingsFromPlugins();
  levelBlk.saveToTextStream(cwr);
  cwr.write(ZERO_PTR<char>(), 1);
  commonBlk.saveToTextStream(cwr);

  req.printf(64,
    "POST /envi_blk HTTP/1.0\r\n"
    "Content-Length: %d\r\n"
    "\r\n",
    cwr.size());
  ret = send(s, req.str(), req.length(), 0);
  if (ret == SOCKET_ERROR)
    goto update_envi_fail;
  ret = send(s, (const char *)cwr.data(), cwr.size(), 0);
  if (ret == SOCKET_ERROR)
    goto update_envi_fail;

  closesocket(s);
  return;

update_envi_fail:

  if (s != INVALID_SOCKET)
    closesocket(s);
  DAGORED2->getConsole().addMessage(ILogWriter::ERROR, "update envi on game http server failed...");
  gameHttpServerAddr = INADDR_NONE;
#else
  LOGERR_CTX("TODO: tools Linux porting: EnvironmentPlugin::flushDataBlocksToGame()");
#endif
}

void EnvironmentPlugin::connectToGameHttpServer()
{
#if _TARGET_PC_WIN // TODO: tools Linux porting: EnvironmentPlugin::connectToGameHttpServer()
  eastl::unique_ptr<PropPanel::DialogWindow> dlg(
    DAGORED2->createDialog(hdpi::_pxScaled(250), hdpi::_pxScaled(120), "Connect To Game Http Server"));
  PropPanel::ContainerPropertyControl *panel = dlg->getPanel();
  panel->createEditBox(337, "Address Connect To:", "");
  if (dlg->showDialog() != PropPanel::DIALOG_ID_OK)
    return;

  SOCKET s = INVALID_SOCKET;
  SimpleString txtAddr = panel->getText(337);
  gameHttpServerAddr = inet_addr(txtAddr.str());
  auto fail = [&] {
    if (s != INVALID_SOCKET)
      closesocket(s);
    DAGORED2->getConsole().addMessage(ILogWriter::ERROR, "connect to http server at '%s' failed...", txtAddr.str());
    gameHttpServerAddr = INADDR_NONE;
    ::MessageBox(0, "Connect to Game Http Server Failed", "Connect Failed", MB_OK | MB_ICONERROR);
  };
  if (gameHttpServerAddr == INADDR_NONE || gameHttpServerAddr == INADDR_ANY)
    return fail();

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == INVALID_SOCKET)
    return fail();

  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(HTTP_SERVER_PORT);
  sin.sin_addr.s_addr = gameHttpServerAddr;

  int ret = connect(s, (sockaddr *)&sin, sizeof(sin));
  if (ret != 0)
    return fail();

  const char *req = "GET / HTTP/1.0\r\n\r\n";
  ret = send(s, req, (int)strlen(req), 0);
  if (ret == SOCKET_ERROR)
    return fail();

  char reply[21];
  memset(reply, 0, sizeof(reply));

  ret = recv(s, reply, sizeof(reply) - 1, 0);
  if (ret == SOCKET_ERROR || ret == 0)
    return fail();

  if (!strstr(reply, "200 OK"))
    return fail();

  DAGORED2->getConsole().addMessage(ILogWriter::NOTE, "successfully connected to %s:%d", txtAddr.str(), HTTP_SERVER_PORT);

  closesocket(s);
#else
  LOGERR_CTX("TODO: tools Linux porting: EnvironmentPlugin::connectToGameHttpServer()");
#endif
}


//==================================================================================================
void EnvironmentPlugin::onLightingSettingsChanged(bool rest_postfx)
{
  if (rest_postfx && !skiesSrv)
  {
    DataBlock &pfx = !isAcesPlugin ? postfxBlk : currentEnvironmentAces.postfxBlk;
    DataBlock *demonBlk = pfx.getBlockByName("DemonPostFx");

    if (demonBlk && demonBlk->getBool("autosyncWithLightPlugin", true))
    {
      demonBlk->setReal("sunAzimuth", ltService->getSunEx(0).azimuth * RAD_TO_DEG);
      demonBlk->setReal("sunZenith", ltService->getSunEx(0).zenith * RAD_TO_DEG);

      EDITORCORE->queryEditorInterface<IDynRenderService>()->restartPostfx(pfx);
    }
  }

  onDirLightChanged();

  Tab<ILightingChangeClient *> interfaces(tmpmem);
  DAGORED2->getInterfacesEx<ILightingChangeClient>(interfaces);

  for (int i = 0; i < interfaces.size(); ++i)
    interfaces[i]->onLightingSettingsChanged();
  EDITORCORE->queryEditorInterface<IDynRenderService>()->onLightingSettingsChanged();
}

//==================================================================================================
void EnvironmentPlugin::onDirLightChanged()
{
  if (ltService->updateDirLightFromSunSky())
    ltService->updateShaderVars();
}


//==================================================================================================
void EnvironmentPlugin::handleViewportPaint(IGenViewportWnd *wnd)
{
  renderObjectsToViewport(wnd);
  if (!isAcesPlugin)
    return;

  if (skiesSrv)
  {
    using hdpi::_pxS;

    int w, h;
    wnd->getViewportSize(w, h);
    const DataBlock &b = skiesSrv->getAppliedWeatherDesc();
    float t = b.getReal("locTime");
    String desc(0, "Location: Lon=%.2f Lat=%.2f,    Local time: %2.0f:%02.0f,    Weather: %s/%s%s, seed=%d,    Date: %04d/%02d/%02d",
      b.getReal("lon"), b.getReal("lat"), floorf(t), fmodf(t * 60.0f, 60.0f), b.getStr("weather"), b.getStr("env"),
      b.getBool("customWeather", false) ? " (custom)" : "", b.getInt("seed"), b.getInt("year"), b.getInt("month"), b.getInt("day"));

    StdGuiRender::set_font(0);

    StdGuiRender::set_color(COLOR_YELLOW);
    StdGuiRender::render_box(_pxS(0), h - _pxS(30), w, h);
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(_pxS(10), h - _pxS(10), desc);
  }

  if (selectedEnvironmentNo == -1)
    return;

  if (!postfxPanel)
    return;

  const DataBlock *hdrBlk = currentEnvironmentAces.postfxBlk.getBlockByNameEx("hdr");
  const char *hdrMode = hdrBlk->getStr("mode", "none");
  if (!stricmp(hdrMode, "none"))
    return;

  const DataBlock *demonBlk = currentEnvironmentAces.postfxBlk.getBlockByName("DemonPostFx");
  const DataBlock *cmBlk = demonBlk->getBlockByNameEx("colorMatrix");
  if (!cmBlk->getBool("view_levels", true))
    return;

  const int levelsWidth = 200;
  const int levelsHeight = 200;
  const int marginWidth = 30;
  const int marginHeight = 30;

  int viewportWidth, viewportHeight;
  wnd->getViewportSize(viewportWidth, viewportHeight);

  StdGuiRender::set_color(COLOR_WHITE);
  StdGuiRender::render_box(viewportWidth - levelsWidth - marginWidth, viewportHeight - levelsHeight - marginHeight,
    viewportWidth - marginWidth, viewportHeight - marginHeight);

  Color3 saturationColor = color3(cmBlk->getE3dcolor("saturationColor", E3DCOLOR(255, 255, 255)));
  float saturation = cmBlk->getReal("saturation", 100.0f);
  Color3 grayColor = color3(cmBlk->getE3dcolor("grayColor", E3DCOLOR(255, 255, 255)));
  Color3 contrastColor = color3(cmBlk->getE3dcolor("contrastColor", E3DCOLOR(255, 255, 255)));
  float contrast = cmBlk->getReal("contrast", 100.0f);
  Color3 contrastPivotColor = color3(cmBlk->getE3dcolor("contrastPivotColor", E3DCOLOR(255, 255, 255)));
  float contrastPivot = cmBlk->getReal("contrastPivot", 50.0f);
  Color3 brightnessColor = color3(cmBlk->getE3dcolor("brightnessColor", E3DCOLOR(255, 255, 255)));
  float brightness = cmBlk->getReal("brightness", 100.0f);

  TMatrix paramColorMatrix = ::make_saturation_color_matrix(saturationColor * (saturation / 100.f), grayColor);

  paramColorMatrix *= ::make_contrast_color_matrix(contrastColor * (contrast / 100.f), contrastPivotColor * (contrastPivot / 100.f));

  paramColorMatrix *= ::make_brightness_color_matrix(brightnessColor * (brightness / 100.f));

  unsigned int colors[3] = {COLOR_LTRED, COLOR_LTGREEN, COLOR_LTBLUE};
  for (int colorNo = 0; colorNo < 3; colorNo++)
  {
    StdGuiRender::set_color(colors[colorNo]);
    for (int pointNo = 0; pointNo < levelsWidth; pointNo++)
    {
      float valueIn = pointNo / (float)levelsWidth;
      Point3 colorIn(0.5f, 0.5f, 0.5f);
      colorIn[colorNo] = valueIn;
      Point3 colorOut = paramColorMatrix * colorIn;
      float valueOut = colorOut[colorNo];

      float prevValueIn = (pointNo == 0 ? pointNo : pointNo - 1) / (float)levelsWidth;
      colorIn[colorNo] = prevValueIn;
      colorOut = paramColorMatrix * colorIn;
      float prevValueOut = colorOut[colorNo];

      StdGuiRender::draw_line((int)(viewportWidth - levelsWidth - marginWidth + (pointNo == 0 ? pointNo : pointNo - 1)),
        (int)(viewportHeight - marginHeight - prevValueOut * levelsHeight), (int)(viewportWidth - levelsWidth - marginWidth + pointNo),
        (int)(viewportHeight - marginHeight - valueOut * levelsHeight));
    }
  }
}


void EnvironmentPlugin::prepareColorStats(int r_channel[256], int g_channel[256], int b_channel[256])
{
  memset(r_channel, 0, 256 * sizeof(int));
  memset(g_channel, 0, 256 * sizeof(int));
  memset(b_channel, 0, 256 * sizeof(int));

  IGenViewportWnd *viewport = DAGORED2->getCurrentViewport();
  if (!viewport)
    return;
  int w, h;
  viewport->getViewportSize(w, h);
  Texture *renderTex = ((DagorEdAppWindow *)DAGORED2)->renderInTex(w, h, NULL);

  if (renderTex)
  {
    int stride = 0;
    TextureInfo texInfo;
    renderTex->getinfo(texInfo);
    E3DCOLOR *img = NULL;
    renderTex->lockimg((void **)&img, stride, 0, TEXLOCK_DEFAULT);

    if (img)
    {
      for (int x = 0; x < texInfo.w; ++x)
        for (int y = 0; y < texInfo.h; ++y)
        {
          E3DCOLOR c = img[y * stride / 4 + x];
          ++r_channel[c.r];
          ++g_channel[c.g];
          ++b_channel[c.b];
        }
    }

    renderTex->unlockimg();
    del_d3dres(renderTex);
  }
}


void EnvironmentPlugin::prepareColorTex(unsigned char r_channel[256], unsigned char g_channel[256], unsigned char b_channel[256])
{
  if (!colorTex)
    return;

  E3DCOLOR *img = NULL;
  int stride = 0;
  colorTex->lockimg((void **)&img, stride, 0, TEXLOCK_WRITE);
  if (img && stride >= COLOR_TEXT_WIDTH * 4)
    for (int x = 0; x < COLOR_TEXT_WIDTH; ++x)
      for (int y = 0; y < COLOR_TEXT_HEIGHT; ++y)
        img[y * stride / 4 + x] = E3DCOLOR(r_channel[x], g_channel[x], b_channel[x]);
  colorTex->unlockimg();
}
