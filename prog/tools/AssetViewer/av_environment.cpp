// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_appwnd.h"
#include "av_environment.h"

#include <libTools/util/strUtil.h>
#include <3d/dag_render.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform_pc.h>
#include <3d/dag_lockTexture.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <de3_skiesService.h>
#include <de3_windService.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_rendInstGen.h>
#include <de3_baseInterfaces.h>
#include <de3_dynRenderService.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderDbg.h>

#include <EditorCore/ec_interface.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/c_indirect.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_findFiles.h>
#include <debug/dag_debug.h>

#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_direct.h>
#include <shaders/dag_overrideStates.h>
#include <textureUtil/textureUtil.h>
#include <render/wind/ambientWind.h>
#include <assets/asset.h>

using hdpi::_pxScaled;

extern ISkiesService *av_skies_srv;
extern SimpleString av_skies_preset, av_skies_env, av_skies_wtype;
static bool av_skies_grp_hidden = false, av_debug_grp_hidden = false, av_envtex_grp_hidden = true;
extern IWindService *av_wind_srv;

extern TEXTUREID load_land_micro_details(const DataBlock &micro);

static inline void updateLight(bool update_var_before = false)
{
  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();

  if (update_var_before)
    ltSrv->updateShaderVars();

  if (ltSrv->updateDirLightFromSunSky())
    ltSrv->updateShaderVars();

  ltSrv->applyLightingToGeometry();
  if (EDITORCORE->getPluginCount() > 0)
    if (ILightingChangeClient *srv = EDITORCORE->getPluginBase(0)->queryInterface<ILightingChangeClient>())
      srv->onLightingSettingsChanged();

  EDITORCORE->queryEditorInterface<IDynRenderService>()->onLightingSettingsChanged();
}

static void decode_rel_fname(String &dest, const char *src);

namespace environment
{
static BaseTexture *texture = NULL;
static TEXTUREID texId = BAD_TEXTUREID;
static int textureStretch = 0;

static BaseTexture *combinedPaintTex = NULL;
static TEXTUREID combinedPaintTexId = BAD_TEXTUREID;

static bool useSinglePaintColor = false;
static E3DCOLOR singlePaintColor = E3DCOLOR(0, 0, 0);
static BaseTexture *singlePaintColorTex = nullptr;
static TEXTUREID singlePaintColorTexId = BAD_TEXTUREID;

static IWindService::PreviewSettings windPreview;

enum
{
  PID_SETTINGS_GROUP,
  PID_RDBG_GROUP,
  PID_TEX_GROUP,

  PID_SUN_AZIMUT,
  PID_SUN_ZENITH,
  PID_SUN_ANGSIZE,

  PID_SUN_BRIGHTNESS,
  PID_SUN_COLOR,

  PID_AMB_COLOR,
  PID_AMB_BRIGHTNESS,

  PID_SUN_RESTORE,

  PID_ENV_BLK,
  PID_ENV_TEXTURE,
  PID_ENV_TEXTURE_STRETCH,
  PID_REF_TEXTURE,
  PID_ENV_LEVEL_MICRODETAIL,
  PID_ENV_LEVEL_MICRODETAIL_LIST,
  PID_PAINT_DETAILS_TEXTURE,

  PID_ENV_REND_GRID,
  PID_ENV_REND_ENTITY,
  PID_ENV_SELECT_ENTITY,
  PID_ENV_ENTITY_POS,

  PID_SHADOW_QUALITY,
  PID_EXPOSURE,
  PID_RENDER_OPT,
  PID_RENDER_OPT_LAST = PID_RENDER_OPT + IDynRenderService::ROPT_COUNT,

  PID_LTDBG_GROUP,
  PID_LTDBG_NMAP,
  PID_LTDBG_DTEX,
  PID_LTDBG_LT,
  PID_LTDBG_CHROME,

  PID_SKIES_GROUP,
  PID_WEATHER_PRESET,
  PID_WEATHER_ENV,
  PID_WEATHER_TYPE,

  PID_DBGSHOW_GRP,
  PID_DBGSHOW_MODE,

  PID_SHGLVAR_GRP = 100,
  PID_SHGLVAR_PANEL,

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


static SunLightProps savedSunProps;
static AssetLightData savedAld;
static DataBlock ShGlobVars;

static bool supportRenderDebug = false;
static int rdbgEnabledGvid = -1;
static int rdbgTypeGvid = -1, rdbgUseNmapGvid = -1, rdbgUseDtexGvid = -1, rdbgUseLtGvid = -1;
static int rdbgUseChromeGvid = -1;

static inline void clearTex()
{
  if (texId != BAD_TEXTUREID)
    release_managed_tex(texId);
  texId = BAD_TEXTUREID;

  texture = NULL;
}

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

static void applyRenderDebug(AssetLightData &ald)
{
  int en = ald.rdbgType > 0 || !ald.rdbgUseNmap || !ald.rdbgUseDtex || !ald.rdbgUseLt || !ald.rdbgUseChrome;
  ShaderGlobal::set_int_fast(rdbgEnabledGvid, en);
  ShaderGlobal::set_int_fast(rdbgTypeGvid, ald.rdbgType);
  ShaderGlobal::set_int_fast(rdbgUseNmapGvid, ald.rdbgUseNmap);
  ShaderGlobal::set_int_fast(rdbgUseDtexGvid, ald.rdbgUseDtex);
  ShaderGlobal::set_int_fast(rdbgUseLtGvid, ald.rdbgUseLt);
  ShaderGlobal::set_int_fast(rdbgUseChromeGvid, ald.rdbgUseChrome);
  enable_diffuse_mipmaps_debug(ald.rdbgType == -1);
}

void set_texture(TEXTUREID tex_id)
{
  if (tex_id == texId)
    return;
  clearTex();
  texId = tex_id;
  texture = acquire_managed_tex(tex_id);
  if (!texture)
    texId = BAD_TEXTUREID;
}

void renderEnviEntity(AssetLightData &ald)
{

  if (!ald.renderEnviEnt || ald.renderEnviEntAsset.empty())
  {
    destroy_it(ald.renderEnviEntity);
    return;
  }

  if (!ald.renderEnviEntity)
  {
    DagorAsset *a = DAEDITOR3.getGenObjAssetByName(ald.renderEnviEntAsset);
    if (!a)
    {
      DAEDITOR3.conError("cannot find entity asset: <%s>", ald.renderEnviEntAsset);
      ald.renderEnviEntAsset = NULL;
      return;
    }
    ald.renderEnviEntity = a ? DAEDITOR3.createEntity(*a) : NULL;
    if (ald.renderEnviEntity)
      ald.renderEnviEntity->setSubtype(IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom"));
    if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
      rigenSrv->discardRIGenRect(0, 0, 64, 64);
  }
  if (ald.renderEnviEntity)
  {
    TMatrix tm;
    tm.identity();
    tm.setcol(3, ald.renderEnviEntPos);
    ald.renderEnviEntity->setTm(tm);
  }
}

void renderEnvironment(bool ortho)
{
  if (!texture)
    return;

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ZERO;
  state.set(shaders::OverrideState::BLEND_SRC_DEST_A);
  state.sblenda = BLEND_ZERO;
  state.dblenda = BLEND_ZERO;
  shaders::overrides::set_master_state(state);
  d3d::setwire(false);

  if (texture->restype() == RES3D_TEX || texture->restype() == RES3D_VOLTEX)
  {
    texture->texaddr(TEXADDR_MIRROR);
    Color4 scales(0.5, -0.5, 0.5, 0.5);
    if (IGenViewportWnd *vp = EDITORCORE->getViewport(0))
    {
      TextureInfo ti;
      texture->getinfo(ti);

      int rtw = 1, rth = 1;
      vp->getViewportSize(rtw, rth);

      float sx = float(rtw) / float(ti.w), sy = float(rth) / float(ti.h);
      if (textureStretch == 1)
        sx *= 2, sy *= 2;
      else if (textureStretch == 2)
        sx /= 2, sy /= 2;
      else if (textureStretch == 3)
      {
        float stretch_s = min(sx, sy);
        sx /= stretch_s, sy /= stretch_s;
      }
      else if (textureStretch == 4)
        sx = sy = 1;
      scales.set(sx * 0.5, -sy * 0.5, 0.5, 0.5);
    }

    if (texture->restype() == RES3D_TEX)
      EDITORCORE->queryEditorInterface<IDynRenderService>()->renderEnviBkgTexture(texture, scales);
    else if (texture->restype() == RES3D_VOLTEX)
    {
      float z = 1.0f - abs(512 - (get_time_msec() / 8) % 1024) / 512.0f;
      float exposure = EDITORCORE->queryEditorInterface<IDynRenderService>()->getExposure();
      EDITORCORE->queryEditorInterface<IDynRenderService>()->renderEnviVolTexture(texture, Color4(1, 1, 1, 1) * exposure,
        Color4(0, 0, 0, 1), scales, z);
    }
  }
  else if (texture->restype() == RES3D_CUBETEX && !ortho)
  {
    float exposure = EDITORCORE->queryEditorInterface<IDynRenderService>()->getExposure();
    EDITORCORE->queryEditorInterface<IDynRenderService>()->renderEnviCubeTexture(texture, Color4(1, 1, 1, 1) * exposure,
      Color4(0, 0, 0, 1));
  }

  shaders::overrides::reset_master_state();
  d3d::setwire(::grs_draw_wire);
  ShaderElement::invalidate_cached_state_block();
}

const char *getEnviTitleStr(AssetLightData *ald)
{
  static String s;
  if (!ald)
    return NULL;

  if (EDITORCORE->queryEditorInterface<IDynRenderService>()->hasEnvironmentSnapshot())
  {
    if (ald->envTextureName.empty())
    {
      s = ald->envBlkFn;
      dd_simplify_fname_c(s);
      if (char *p = strrchr(s, '/'))
        *p = 0;
      if (char *p = strrchr(s, '/'))
      {
        *p = 0;
        if (char *p2 = strrchr(s, '/'))
        {
          *p = '/';
          return p2 + 1;
        }
      }
      return s;
    }
  }
  else if (av_skies_srv && !av_skies_preset.empty())
    return NULL;

  if (ald->envTextureName.empty())
    return NULL;

  s = ald->envTextureName;
  if (char *p = strrchr(s, '/'))
    return p + 1;
  return s;
}

void clear() { clearTex(); }

void setUseSinglePaintColor(bool use) { useSinglePaintColor = use; }

bool isUsingSinglePaintColor() { return useSinglePaintColor; }

void setSinglePaintColor(E3DCOLOR color) { singlePaintColor = color; }

E3DCOLOR getSinglePaintColor() { return singlePaintColor; }

static void createSinglePaintColorTex()
{
  if (environment::singlePaintColorTex)
    return;

  if (!environment::combinedPaintTex)
  {
    logerr("combined_paint_tex texture is not set, \"Apply color to entity mode\" will not work!");
    return;
  }

  TextureInfo texInfo;
  environment::combinedPaintTex->getinfo(texInfo);

  environment::singlePaintColorTex =
    dag::create_tex(nullptr, texInfo.w, texInfo.h, TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD, 1, "single_paint_color_tex").release();

  if (!environment::singlePaintColorTex)
  {
    logerr("Failed to create single_paint_color_tex texture, \"Apply color to entity mode\" will not work!");
    return;
  }

  environment::singlePaintColorTex->texfilter(TEXFILTER_POINT);
  environment::singlePaintColorTexId = register_managed_tex("single_paint_color_tex", environment::singlePaintColorTex);
}

static void fillTextureWithColor(BaseTexture &texture, uint32_t color)
{
  TextureInfo textureInfo;
  texture.getinfo(textureInfo);

  LockedImage2DView<uint32_t> lockedTexture = lock_texture<uint32_t>(&texture, 0, TEXLOCK_WRITE);
  if (!lockedTexture)
    return;

  for (int y = 0; y < textureInfo.h; ++y)
    for (int x = 0; x < textureInfo.w; ++x)
      lockedTexture.at(x, y) = color;
}

void updatePaintColorTexture()
{
  static int paintDetailsVarId = get_shader_variable_id("paint_details_tex", true);

  if (useSinglePaintColor)
  {
    createSinglePaintColorTex();
    if (singlePaintColorTex)
    {
      fillTextureWithColor(*singlePaintColorTex, singlePaintColor);
      ShaderGlobal::set_texture(paintDetailsVarId, singlePaintColorTexId);
    }
  }
  else
  {
    ShaderGlobal::set_texture(paintDetailsVarId, combinedPaintTexId);
  }
}

//---------------------------------------------------------------------------

class EnvSetDlg : public PropPanel::DialogWindow
{
public:
  EnvSetDlg(void *handle, SunLightProps *sun_props, AssetLightData *al_data) :
    DialogWindow(handle, _pxScaled(540), _pxScaled(760 - (supportRenderDebug ? 0 : 120)), "Environment Settings"),
    sunProps(sun_props),
    ald(al_data)
  {
    IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
    PropPanel::ContainerPropertyControl *_panel = getPanel();
    G_ASSERT(_panel && "No panel in EnvSetDlg");

    G_ASSERT(sun_props && "Sun properties is NULL in EnvSetDlg");
    G_ASSERT(al_data && "AlData is NULL in EnvSetDlg");

    PropPanel::ContainerPropertyControl *_settings = drSrv->hasEnvironmentSnapshot()
                                                       ? NULL
                                                       : (av_skies_srv ? _panel->createGroup(PID_SETTINGS_GROUP, "Settings")
                                                                       : _panel->createGroupBox(PID_SETTINGS_GROUP, "Settings"));
    if (_settings)
    {
      _settings->createTrackFloat(PID_SUN_AZIMUT, "sun azimut", RadToDeg(sunProps->azimuth), -180.f, 180.f, 0.1);
      _settings->createTrackFloat(PID_SUN_ZENITH, "sun zenith", RadToDeg(sunProps->zenith), -90.f, 90.f, 0.1);
      _settings->createEditFloat(PID_SUN_ANGSIZE, "sun angSize", sunProps->angSize, 3);
      _settings->createEditFloat(PID_SUN_BRIGHTNESS, "sun integral luminocity", sunProps->calcLuminocity(), 3);
      _settings->createSimpleColor(PID_SUN_COLOR, "sun color", sunProps->color);
      ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
      SkyLightProps &sky = ltSrv->getSky();
      _settings->createSimpleColor(PID_AMB_COLOR, "ambient color", sky.color);
      _settings->createEditFloat(PID_AMB_BRIGHTNESS, "ambient brightness", sky.brightness, 2);
    }

    PropPanel::ContainerPropertyControl *_dbgShow =
      (drSrv && drSrv->getDebugShowTypeNames().size() > 1) ? _panel->createGroup(PID_DBGSHOW_GRP, "Debug settings") : NULL;
    if (_dbgShow)
    {
      dag::ConstSpan<const char *> modes = drSrv->getDebugShowTypeNames();
      PropPanel::ContainerPropertyControl *_dbgShowRG = _dbgShow->createRadioGroup(PID_DBGSHOW_MODE, "Debug show mode");
      for (int i = 0; i < modes.size(); i++)
        _dbgShowRG->createRadio(i, modes[i]);
      _dbgShow->setInt(PID_DBGSHOW_MODE, drSrv->getDebugShowType());
      _dbgShow->setBoolValue(av_debug_grp_hidden);
    }

    if (supportRenderDebug)
    {
      PropPanel::ContainerPropertyControl &ltDbgGrp = *_panel->createGroup(PID_RDBG_GROUP, "Render debug");
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
        ltDbgGrp.createCheckBox(PID_LTDBG_NMAP, "Enable normal maps", ald->rdbgUseNmap);
      if (VariableMap::isGlobVariablePresent(rdbgUseDtexGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_DTEX, "Enable detail tex", ald->rdbgUseDtex);
      if (VariableMap::isGlobVariablePresent(rdbgUseLtGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_LT, "Enable lighting", ald->rdbgUseLt);
      if (VariableMap::isGlobVariablePresent(rdbgUseChromeGvid))
        ltDbgGrp.createCheckBox(PID_LTDBG_CHROME, "Enable chrome (envi reflection)", ald->rdbgUseChrome);
      ltDbgGrp.setInt(PID_LTDBG_GROUP, ald->rdbgType);
    }

    PropPanel::ContainerPropertyControl *_tex = _panel->createGroup(PID_TEX_GROUP, "Environment textures");
    {
      Tab<String> _masks(tmpmem);
      _masks.push_back() = "Environment BLK files|envi.blk";
      _tex->createFileButton(PID_ENV_BLK, "envi snapshot settings");
      _panel->setStrings(PID_ENV_BLK, _masks);
      if (!ald->envBlkFn.empty())
        _panel->setText(PID_ENV_BLK, ald->envBlkFn);

      _masks.clear();
      _masks.push_back() = "DDSx and image files|*.ddsx;*.tga;*.jpg;*.png;*.dds";
      _masks.push_back() = "DDS files|*.dds";
      _masks.push_back() = "TGA files|*.tga";
      _masks.push_back() = "JPEG files|*.jpg";
      _masks.push_back() = "All files|*.*";

      _tex->createFileButton(PID_ENV_TEXTURE, "environment/background texture");
      _panel->setStrings(PID_ENV_TEXTURE, _masks);
      if (!ald->envTextureName.empty())
        _panel->setText(PID_ENV_TEXTURE, ald->envTextureName);

      _tex->createStatic(-1, "paint details texture");
      _tex->createButton(PID_PAINT_DETAILS_TEXTURE,
        ald->paintDetailsTexAsset.empty() ? "-- no palette tex --" : ald->paintDetailsTexAsset.str());

      Tab<String> bkg_s;
      bkg_s.push_back() = "Tile, original size";
      bkg_s.push_back() = "Tile, 2x smaller size";
      bkg_s.push_back() = "Tile, 2x larger size";
      bkg_s.push_back() = "Stretch fit (preserve aspect ratio)";
      bkg_s.push_back() = "Full stretch";
      _tex->createCombo(PID_ENV_TEXTURE_STRETCH, "background texture stretch", bkg_s, ald->envTextureStretch);


      _tex->createFileButton(PID_REF_TEXTURE, "reflection texture");
      _panel->setStrings(PID_REF_TEXTURE, _masks);
      if (!ald->textureName.empty())
        _panel->setText(PID_REF_TEXTURE, ald->textureName);

      _masks.clear();
      _masks.push_back() = "Level BLK files|*.blk";
      _tex->createFileButton(PID_ENV_LEVEL_MICRODETAIL, "Level BLK (for microdetails texture)");
      _panel->setStrings(PID_ENV_LEVEL_MICRODETAIL, _masks);
      if (!ald->envLevelBlkFn.empty())
        _panel->setText(PID_ENV_LEVEL_MICRODETAIL, ald->envLevelBlkFn);

      DataBlock appBlk(::get_app().getWorkspace().getAppPath());
      if (const char *scan_level_blk_dir = appBlk.getStr("levelsBlkScanDir", NULL))
      {
        Tab<SimpleString> levblk;
        DataBlock blk;

        find_files_in_folder(levblk, scan_level_blk_dir, ".blk", true, false, false);
        for (int i = levblk.size() - 1; i >= 0; i--)
          if (*dd_get_fname(levblk[i]) == '_' || !blk.load(levblk[i]))
            erase_items(levblk, i, 1);

        if (levblk.size())
        {
          Tab<String> levblk_s;
          levblk_s.resize(levblk.size() + 1);
          levblk_s[0] = "--";
          for (int i = 1; i < levblk_s.size(); i++)
            levblk_s[i] = levblk[i - 1];
          _tex->createCombo(PID_ENV_LEVEL_MICRODETAIL_LIST, "", levblk_s,
            find_value_idx(levblk, ald->envLevelBlkFn) >= 0 ? ald->envLevelBlkFn : "--");
        }
      }
    }
    if (addWeatherPanel(_panel, drSrv))
      _settings->setBoolValue(true);

    addAmbientWindPanel(_panel);

    _tex->setBoolValue(av_envtex_grp_hidden);
    _panel->createCheckBox(PID_ENV_REND_GRID, "render grid", get_app().getGrid().isVisible(0));
    for (int i = 0; i < drSrv->ROPT_COUNT; i++)
      if (drSrv->getRenderOptSupported(i))
        _panel->createCheckBox(PID_RENDER_OPT + i, drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i));
    if (drSrv->getShadowQualityNames().size() > 0 && drSrv->getShadowQualityNames()[0])
    {
      Tab<String> shadowQualityStr;
      shadowQualityStr.resize(drSrv->getShadowQualityNames().size());
      for (int i = 0; i < shadowQualityStr.size(); i++)
        shadowQualityStr[i] = drSrv->getShadowQualityNames()[i];
      _panel->createCombo(PID_SHADOW_QUALITY, "shadow quality", shadowQualityStr, drSrv->getShadowQuality());
    }
    if (drSrv->hasExposure())
      _panel->createTrackFloatLogarithmic(PID_EXPOSURE, "exposure (0-10 logarithmic)", drSrv->getExposure(), 0.0f, 10.0f, 0.0f,
        sqrt(10.0f));
    _panel->createSeparator();
    _panel->createCheckBox(PID_ENV_REND_ENTITY, "render envi entity", ald->renderEnviEnt);
    _panel->createButton(PID_ENV_SELECT_ENTITY, ald->renderEnviEntAsset.empty() ? "-- no envi entity --" : ald->renderEnviEntAsset);
    _panel->createPoint3(PID_ENV_ENTITY_POS, "render envi entity at", ald->renderEnviEntPos);
  }
  void setAmbientWindPanel(PropPanel::ContainerPropertyControl *_panel)
  {
    _panel->setBool(PID_WIND_ENABLED, windPreview.enabled);
    _panel->setFloat(PID_WIND_DIR, windPreview.windAzimuth);
    _panel->setFloat(PID_WIND_STRENGTH, windPreview.windStrength);
    _panel->setFloat(PID_WIND_NOISE_STRENGTH, windPreview.ambient.windNoiseStrength);
    _panel->setFloat(PID_WIND_NOISE_SPEED, windPreview.ambient.windNoiseSpeed);
    _panel->setFloat(PID_WIND_NOISE_SCALE, windPreview.ambient.windNoiseScale);
    _panel->setFloat(PID_WIND_NOISE_PERPENDICULAR, windPreview.ambient.windNoisePerpendicular);
    _panel->setText(PID_WIND_LEVEL, windPreview.levelPath);

    enableAmbientWindPanel(_panel);
  }
  void addAmbientWindPanel(PropPanel::ContainerPropertyControl *_panel)
  {
    PropPanel::ContainerPropertyControl *_wind = _panel->createGroup(PID_WIND_GROUP, "Ambient wind settings");
    _wind->createCheckBox(PID_WIND_ENABLED, "Enabled", false);
    _wind->createTrackFloat(PID_WIND_DIR, "Direction", 0, 0, 360, 1);
    _wind->createTrackFloat(PID_WIND_STRENGTH, "Strength (Beaufort)", 0, 0, 12, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_STRENGTH, "Noise strength (Multiplier of strength)", 0, 0, 10, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_SPEED, "Noise speed (Beaufort)", 0, 0, 12, 0.1);
    _wind->createTrackFloat(PID_WIND_NOISE_SCALE, "Noise scale (Meters)", 0, 0, 200, 1);
    _wind->createTrackFloat(PID_WIND_NOISE_PERPENDICULAR, "Noise perpendicular", 0, 0, 2, 0.1);
    _wind->createFileButton(PID_WIND_LEVEL, "Level blk with wind settings");

    setAmbientWindPanel(_panel);
  }
  void enableAmbientWindPanel(PropPanel::ContainerPropertyControl *_panel)
  {
    bool enableOverride = windPreview.enabled;
    _panel->setEnabledById(PID_WIND_DIR, enableOverride);
    _panel->setEnabledById(PID_WIND_STRENGTH, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_STRENGTH, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_SPEED, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_SCALE, enableOverride);
    _panel->setEnabledById(PID_WIND_NOISE_PERPENDICULAR, enableOverride);

    _panel->setEnabledById(PID_WIND_LEVEL, enableOverride && av_wind_srv && av_wind_srv->isLevelEcsSupported());
  }
  bool addWeatherPanel(PropPanel::ContainerPropertyControl *_panel, IDynRenderService *drSrv)
  {
    if (av_skies_srv && !drSrv->hasEnvironmentSnapshot())
    {
      PropPanel::ContainerPropertyControl *_skies = _panel->createGroup(PID_SKIES_GROUP, "Sky/weather settings");
      Tab<String> preset(midmem), env(midmem), wtype(midmem);
      av_skies_srv->fillPresets(preset, env, wtype);

      insert_item_at(preset, 0, String("-- Disable daSkies --"));
      _skies->createList(PID_WEATHER_PRESET, "Weather presets", preset,
        av_skies_preset.empty() ? preset[0].str() : av_skies_preset.str());
      _skies->createList(PID_WEATHER_ENV, "Environment types", env, av_skies_env);
      _skies->createList(PID_WEATHER_TYPE, "Weather types", wtype, av_skies_wtype);

      _skies->getById(PID_WEATHER_PRESET)->setHeight(_pxScaled(80));
      _skies->getById(PID_WEATHER_ENV)->setHeight(_pxScaled(80));
      _skies->getById(PID_WEATHER_TYPE)->setHeight(_pxScaled(80));
      _skies->setBoolValue(av_skies_grp_hidden);
      return true;
    }
    return false;
  }


  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    switch (pcb_id)
    {
      case PID_ENV_SELECT_ENTITY:
        if (const char *asset = DAEDITOR3.selectAsset(ald->renderEnviEntAsset, "Select envi entity", DAEDITOR3.getGenObjAssetTypes()))
        {
          ald->renderEnviEntAsset = asset;
          panel->setCaption(pcb_id, ald->renderEnviEntAsset.empty() ? "-- no envi entity --" : ald->renderEnviEntAsset);
          destroy_it(ald->renderEnviEntity);
          environment::renderEnviEntity(*ald);
        }
        break;
      case PID_PAINT_DETAILS_TEXTURE:
        if (const char *asset = DAEDITOR3.selectAssetX(ald->paintDetailsTexAsset, "Select palette texture", "tex", "palette", true))
        {
          ald->paintDetailsTexAsset = asset;
          panel->setCaption(pcb_id, ald->paintDetailsTexAsset.empty() ? "-- no palette tex --" : ald->paintDetailsTexAsset);
          ald->setPaintDetailsTexture();
        }
        break;
    }
  }


  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
    switch (pcb_id)
    {
      case PID_ENV_TEXTURE:
      {
        const char *appDir = get_app().getWorkspace().getAppDir();
        String fn(panel->getText(pcb_id));

        if (fn.length())
        {
          ald->envTextureName = ::make_path_relative(fn, appDir);
          ald->setEnvironmentTexture();
          panel->setText(pcb_id, ald->envTextureName);
        }
        else
        {
          ald->envTextureName = NULL;
          ald->setEnvironmentTexture();
          updateLight();
        }
      }
      break;
      case PID_ENV_BLK:
      {
        const char *appDir = get_app().getWorkspace().getAppDir();
        String fn(panel->getText(pcb_id));

        if (fn.length())
        {
          ald->envBlkFn = ::make_path_relative(fn, appDir);
          panel->setText(pcb_id, ald->envBlkFn);
          ald->setEnvironmentTexture();
        }
        else
        {
          ald->envBlkFn = NULL;
          ald->setEnvironmentTexture();
          updateLight();
        }
        EDITORCORE->actObjects(1e-3); // to update use_skies flag
      }
      break;
      case PID_ENV_LEVEL_MICRODETAIL:
      case PID_ENV_LEVEL_MICRODETAIL_LIST:
      {
        const char *appDir = get_app().getWorkspace().getAppDir();
        String fn(panel->getText(pcb_id));

        if (fn.length() && strcmp(fn, "--") != 0)
        {
          if (pcb_id == PID_ENV_LEVEL_MICRODETAIL)
            ald->envLevelBlkFn = ::make_path_relative(fn, appDir);
          else
            ald->envLevelBlkFn = fn;
          panel->setText(PID_ENV_LEVEL_MICRODETAIL, ald->envLevelBlkFn);
          panel->setText(PID_ENV_LEVEL_MICRODETAIL_LIST, dd_file_exists(ald->envLevelBlkFn) ? ald->envLevelBlkFn : "--");
          ald->applyMicrodetailFromLevelBlk();
        }
        else
        {
          ald->envLevelBlkFn = NULL;
          panel->setText(PID_ENV_LEVEL_MICRODETAIL, ald->envLevelBlkFn);
          panel->setText(PID_ENV_LEVEL_MICRODETAIL_LIST, "--");
          ald->applyMicrodetailFromLevelBlk();
          updateLight();
        }
      }
      break;
      case PID_ENV_TEXTURE_STRETCH: textureStretch = ald->envTextureStretch = panel->getInt(pcb_id); break;

      case PID_REF_TEXTURE:
      {
        const char *appDir = get_app().getWorkspace().getAppDir();
        String fn(panel->getText(pcb_id));

        if (fn.length())
        {
          ald->textureName = ::make_path_relative(fn, appDir);
          ald->setReflectionTexture();
          panel->setText(pcb_id, ald->textureName);
        }
        else
        {
          ald->textureName = NULL;
          ald->setReflectionTexture();
          updateLight();
        }
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

      case PID_SHADOW_QUALITY: drSrv->setShadowQuality(panel->getInt(pcb_id)); break;

      case PID_EXPOSURE: drSrv->setExposure(panel->getFloat(pcb_id)); break;

      case PID_LTDBG_GROUP:
        ald->rdbgType = panel->getInt(pcb_id);
        applyRenderDebug(*ald);
        break;

      case PID_DBGSHOW_MODE: drSrv->setDebugShowType(panel->getInt(pcb_id)); break;


      case PID_LTDBG_NMAP:
        ald->rdbgUseNmap = panel->getBool(pcb_id);
        applyRenderDebug(*ald);
        break;

      case PID_LTDBG_DTEX:
        ald->rdbgUseDtex = panel->getBool(pcb_id);
        applyRenderDebug(*ald);
        break;

      case PID_LTDBG_LT:
        ald->rdbgUseLt = panel->getBool(pcb_id);
        applyRenderDebug(*ald);
        break;

      case PID_LTDBG_CHROME:
        ald->rdbgUseChrome = panel->getBool(pcb_id);
        applyRenderDebug(*ald);
        break;
      case PID_WIND_NOISE_STRENGTH:
      case PID_WIND_NOISE_SPEED:
      case PID_WIND_NOISE_SCALE:
      case PID_WIND_NOISE_PERPENDICULAR:
      case PID_WIND_STRENGTH:
      case PID_WIND_DIR:
      case PID_WIND_ENABLED:
        windPreview.ambient.windNoiseStrength = panel->getFloat(PID_WIND_NOISE_STRENGTH);
        windPreview.ambient.windNoiseSpeed = panel->getFloat(PID_WIND_NOISE_SPEED);
        windPreview.ambient.windNoiseScale = panel->getFloat(PID_WIND_NOISE_SCALE);
        windPreview.ambient.windNoisePerpendicular = panel->getFloat(PID_WIND_NOISE_PERPENDICULAR);
        windPreview.windStrength = panel->getFloat(PID_WIND_STRENGTH);
        windPreview.windAzimuth = panel->getFloat(PID_WIND_DIR);
        windPreview.useWindDir = false;
        windPreview.enabled = panel->getBool(PID_WIND_ENABLED);

        if (pcb_id == PID_WIND_ENABLED)
          enableAmbientWindPanel(panel);

        if (av_wind_srv)
          av_wind_srv->setPreview(windPreview);
        break;
      case PID_WIND_LEVEL:
      {
        windPreview.levelPath = panel->getText(pcb_id);

        if (av_wind_srv)
          av_wind_srv->setPreview(windPreview);
      }
      break;
      case PID_WEATHER_PRESET:
      case PID_WEATHER_TYPE:
      case PID_WEATHER_ENV:
        if (av_skies_srv)
        {
          Tab<String> preset(midmem), env(midmem), wtype(midmem);
          av_skies_srv->fillPresets(preset, env, wtype);
          if (pcb_id == PID_WEATHER_PRESET)
          {
            int idx = panel->getInt(pcb_id);
            av_skies_preset = (idx == 0) ? NULL : preset[idx - 1].str();
          }
          else if (pcb_id == PID_WEATHER_TYPE)
            av_skies_wtype = wtype[panel->getInt(pcb_id)];
          else if (pcb_id == PID_WEATHER_ENV)
            av_skies_env = env[panel->getInt(pcb_id)];

          if (!av_skies_preset.empty() && !av_skies_env.empty() && !av_skies_wtype.empty())
            av_skies_srv->setWeather(av_skies_preset, av_skies_env, av_skies_wtype);
          updateLight();
          EDITORCORE->actObjects(1e-3); // to update use_skies flag
        }
        break;

      case PID_ENV_REND_GRID: get_app().getGrid().setVisible(panel->getBool(pcb_id), 0); break;
      case PID_ENV_REND_ENTITY:
        ald->renderEnviEnt = panel->getBool(pcb_id);
        environment::renderEnviEntity(*ald);
        break;
      case PID_ENV_ENTITY_POS:
        ald->renderEnviEntPos = panel->getPoint3(pcb_id);
        environment::renderEnviEntity(*ald);
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
        else if (unsigned(pcb_id) > 0x00100000)
          if (scheme->getParams(schemeGrp, ShGlobVars))
            ShaderGlobal::set_vars_from_blk(ShGlobVars);
    }
  }

  PropPanel::PropPanelScheme *scheme;
  PropPanel::ContainerPropertyControl *schemeGrp;

protected:
  SunLightProps *sunProps;
  AssetLightData *ald;
};

//---------------------------------------------------------------------------

void show_environment_settings(void *handle, AssetLightData *ald)
{
  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();

  G_ASSERT(ltSrv && "show_environment_settings: EDITORCORE has no interface!!!");

  if (ltSrv->getSunCount() == 0)
    return;

  SunLightProps *defSun = ltSrv->getSun(0);

  savedSunProps = *defSun;
  savedAld = *ald;
  savedAld.renderEnviEntity = NULL;
  bool saved_grid = get_app().getGrid().isVisible(0);
  int saved_dstype = drSrv->getDebugShowType();
  bool saved_ropt[drSrv->ROPT_COUNT];
  for (int i = 0; i < drSrv->ROPT_COUNT; i++)
    saved_ropt[i] = drSrv->getRenderOptEnabled(i);
  bool savedShadowQ = drSrv->getShadowQuality();

  SkyLightProps &sky = ltSrv->getSky();
  SkyLightProps skyProps = sky;

  EnvSetDlg envSetDlg(handle, defSun, ald);

  // ImGui scrolls to the focused item, which in this case is at the bottom and requires scrolling. Prevent that.
  envSetDlg.setInitialFocus(PropPanel::DIALOG_ID_NONE);

  // global shader vars fill

  PropPanel::ContainerPropertyControl *grp = envSetDlg.getPanel()->createGroup(PID_SHGLVAR_GRP, "Shader global vars");
  grp->setEventHandler(&envSetDlg);

  DataBlock appBlk(::get_app().getWorkspace().getAppPath());
  DataBlock schemeBlk;
  schemeBlk.setFrom(appBlk.getBlockByNameEx("shader_glob_vars_scheme"));

  PropPanel::PropPanelScheme *scheme = grp->createSceme();
  scheme->load(schemeBlk, true);
  scheme->setParams(grp, ShGlobVars);
  grp->setBoolValue(true);
  envSetDlg.scheme = scheme;
  envSetDlg.schemeGrp = grp;
  DataBlock old_ShGlobVars;
  old_ShGlobVars = ShGlobVars;

  // show dialog

  if (envSetDlg.showDialog() == PropPanel::DIALOG_ID_CANCEL)
  {
    *defSun = savedSunProps;
    destroy_it(ald->renderEnviEntity);
    *ald = savedAld;
    sky = skyProps;
    get_app().getGrid().setVisible(saved_grid, 0);
    drSrv->setDebugShowType(saved_dstype);
    for (int i = 0; i < drSrv->ROPT_COUNT; i++)
      drSrv->setRenderOptEnabled(i, saved_ropt[i]);
    drSrv->setShadowQuality(savedShadowQ);

    ald->setEnvironmentTexture(false);
    ald->applyMicrodetailFromLevelBlk();
    applyRenderDebug(*ald);
    ShGlobVars = old_ShGlobVars;
    ShaderGlobal::set_vars_from_blk(ShGlobVars);
    drSrv->renderFramesToGetStableAdaptation(0.1, 10);
  }
  else
  {
    // global shader vars change
    if (scheme->getParams(grp, ShGlobVars))
      ShaderGlobal::set_vars_from_blk(ShGlobVars);
  }
  av_skies_grp_hidden = envSetDlg.getPanel()->getBool(PID_SKIES_GROUP);
  av_debug_grp_hidden = envSetDlg.getPanel()->getBool(PID_DBGSHOW_GRP);
  av_envtex_grp_hidden = envSetDlg.getPanel()->getBool(PID_TEX_GROUP);

  grp->setEventHandler(NULL);
  del_it(scheme);
}


void load_settings(DataBlock &blk, AssetLightData *ald, const AssetLightData *ald_def, bool &rend_grid)
{
  const DataBlock &ltData = *blk.getBlockByNameEx("AssetLight");

  ald->envTextureName = ltData.getStr("environment_texture_name", ald_def->envTextureName);
  ald->envTextureStretch = ltData.getInt("environment_texture_stretch", 0);
  ald->envBlkFn = ltData.getStr("environment_settings_fn", ald_def->envBlkFn);
  ald->envLevelBlkFn = ltData.getStr("environment_level_blk_fn", ald_def->envLevelBlkFn);

  ald->textureName = ltData.getStr("texture_name", ald_def->textureName);
  ald->shaderVar = ltData.getStr("shader_var", ald_def->shaderVar);

  Point3 ltColor = ltData.getPoint3("color", Point3(ald_def->color.r, ald_def->color.g, ald_def->color.b));
  ald->color.r = ltColor.x;
  ald->color.g = ltColor.y;
  ald->color.b = ltColor.z;

  ald->specPower = ltData.getReal("spec_power", ald_def->specPower);
  ald->specStrength = ltData.getReal("spec_strength", ald_def->specStrength);

  ald->paintDetailsTexAsset = ltData.getStr("paint_details_tex", NULL);
  ald->paintDetailsTexAsset = DagorAsset::fpath2asset(ald->paintDetailsTexAsset);
  ald->setPaintDetailsTexture();

  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  if (ltSrv->getSunCount() > 0)
  {
    SunLightProps *defSun = ltSrv->getSun(0);

    const DataBlock &sunBlk = *blk.getBlockByNameEx("first_sun");

    defSun->azimuth = DegToRad(sunBlk.getReal("azimut", RadToDeg(defSun->azimuth)));
    defSun->zenith = DegToRad(sunBlk.getReal("zenith", RadToDeg(defSun->zenith)));
    defSun->angSize = sunBlk.getReal("angSize", defSun->angSize);
    defSun->specPower = sunBlk.getReal("specPower", defSun->specPower);
    defSun->brightness = sunBlk.getReal("brightness", defSun->brightness);

    Point3 c = sunBlk.getPoint3("color", Point3(defSun->color.r, defSun->color.g, defSun->color.b));
    defSun->color.r = c.x;
    defSun->color.g = c.y;
    defSun->color.b = c.z;

    updateLight();
  }

  const DataBlock &envBlk = *blk.getBlockByNameEx("ambient");
  {
    SkyLightProps &sky = ltSrv->getSky();
    sky.brightness = envBlk.getReal("brightness", sky.brightness);
    sky.color = envBlk.getE3dcolor("color", sky.color);
  }
  rend_grid = blk.getBool("renderGrid", true);
  ald->renderEnviEnt = blk.getBool("renderEnviEnt", ald_def->renderEnviEnt);
  ald->renderEnviEntAsset = blk.getStr("renderEnviEntAsset", ald_def->renderEnviEntAsset);
  ald->renderEnviEntPos = blk.getPoint3("renderEnviEntPos", ald_def->renderEnviEntPos);

  ald->rdbgType = blk.getInt("rdbgType", ald_def->rdbgType);
  ald->rdbgUseNmap = blk.getBool("rdbgUseNmap", ald_def->rdbgUseNmap);
  ald->rdbgUseDtex = blk.getBool("rdbgUseDtex", ald_def->rdbgUseDtex);
  ald->rdbgUseLt = blk.getBool("rdbgUseLt", ald_def->rdbgUseLt);
  ald->rdbgUseChrome = blk.getBool("rdbgUseChrome", ald_def->rdbgUseChrome);

  DataBlock appBlk(::get_app().getWorkspace().getAppPath());

  updateLight();
  detectRenderDebug();
  applyRenderDebug(*ald);

  // load global shader vars

  const DataBlock *sgv_values = blk.getBlockByNameEx("shader_glob_var_values");
  if (sgv_values)
  {
    ShGlobVars.setFrom(sgv_values);
    ShaderGlobal::set_vars_from_blk(ShGlobVars);
  }

  IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
  for (int i = 0; i < drSrv->ROPT_COUNT; i++)
    if (drSrv->getRenderOptSupported(i))
      drSrv->setRenderOptEnabled(i, blk.getBool(drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i)));
  drSrv->setShadowQuality(blk.getInt("shadowQ", appBlk.getInt("AV_defaultShadowQ", 0)));
  if (drSrv->hasExposure())
    drSrv->setExposure(blk.getReal("exposure", appBlk.getReal("AV_defaultExposure", 1.0f)));

  av_skies_grp_hidden = blk.getBool("av_skies_grp_hidden", av_skies_grp_hidden);
  av_debug_grp_hidden = blk.getBool("av_debug_grp_hidden", av_debug_grp_hidden);
  av_envtex_grp_hidden = blk.getBool("av_envtex_grp_hidden", av_envtex_grp_hidden);

  IWindService::readSettingsBlk(windPreview, *blk.getBlockByNameEx("wind"));
  if (IWindService *windSrv = EDITORCORE->queryEditorInterface<IWindService>())
    windSrv->setPreview(windPreview);
}


void save_settings(DataBlock &blk, AssetLightData *ald, const AssetLightData *ald_def, bool &rend_grid)
{
  DataBlock &ltData = *blk.addBlock("AssetLight");

  if (stricmp(ald_def->envTextureName, ald->envTextureName) != 0)
    ltData.setStr("environment_texture_name", ald->envTextureName);
  if (ald_def->envTextureStretch != ald->envTextureStretch)
    ltData.setInt("environment_texture_stretch", ald->envTextureStretch);
  if (stricmp(ald_def->envBlkFn, ald->envBlkFn) != 0)
    ltData.setStr("environment_settings_fn", ald->envBlkFn);
  if (stricmp(ald_def->envLevelBlkFn, ald->envLevelBlkFn) != 0)
    ltData.setStr("environment_level_blk_fn", ald->envLevelBlkFn);
  if (stricmp(ald_def->textureName, ald->textureName) != 0)
    ltData.setStr("texture_name", ald->textureName);
  if (stricmp(ald_def->shaderVar, ald->shaderVar) != 0)
    ltData.setStr("shader_var", ald->shaderVar);

  if (ald_def->specPower != ald->specPower)
    ltData.setReal("spec_power", ald->specPower);
  if (ald_def->specStrength != ald->specStrength)
    ltData.setReal("spec_strength", ald->specStrength);

  if (!ald->paintDetailsTexAsset.empty())
    ltData.setStr("paint_details_tex", ald->paintDetailsTexAsset);

  if ((ald_def->color.r != ald->color.r) || (ald_def->color.g != ald->color.g) || (ald_def->color.b != ald->color.b))
    ltData.setPoint3("color", Point3(ald->color.r, ald->color.g, ald->color.b));


  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  if (ltSrv->getSunCount() > 0)
  {
    SunLightProps *defSun = ltSrv->getSun(0);

    DataBlock &sunBlk = *blk.addBlock("first_sun");

    sunBlk.setReal("azimut", RadToDeg(defSun->azimuth));
    sunBlk.setReal("zenith", RadToDeg(defSun->zenith));
    sunBlk.setReal("angSize", defSun->angSize);
    sunBlk.setReal("specPower", defSun->specPower);
    sunBlk.setReal("brightness", defSun->brightness);

    sunBlk.setPoint3("color", Point3(defSun->color.r, defSun->color.g, defSun->color.b));
  }


  DataBlock &envBlk = *blk.addBlock("ambient");
  {
    SkyLightProps &sky = ltSrv->getSky();
    envBlk.setReal("brightness", sky.brightness);
    envBlk.setE3dcolor("color", sky.color);
  }

  if (!rend_grid)
    blk.setBool("renderGrid", false);
  blk.setBool("renderEnviEnt", ald->renderEnviEnt);
  blk.setStr("renderEnviEntAsset", ald->renderEnviEntAsset);
  blk.setPoint3("renderEnviEntPos", ald->renderEnviEntPos);
  IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
  for (int i = 0; i < drSrv->ROPT_COUNT; i++)
    if (drSrv->getRenderOptSupported(i))
      blk.setBool(drSrv->getRenderOptName(i), drSrv->getRenderOptEnabled(i));
  blk.setInt("shadowQ", drSrv->getShadowQuality());
  if (drSrv->hasExposure())
    blk.setReal("exposure", drSrv->getExposure());

  blk.setInt("rdbgType", ald->rdbgType);
  blk.setBool("rdbgUseNmap", ald->rdbgUseNmap);
  blk.setBool("rdbgUseDtex", ald->rdbgUseDtex);
  blk.setBool("rdbgUseLt", ald->rdbgUseLt);
  blk.setBool("rdbgUseChrome", ald->rdbgUseChrome);

  DataBlock &windBlk = *blk.addBlock("wind");
  IWindService::writeSettingsBlk(windBlk, windPreview);

  // save global shader vars

  DataBlock *sgv_values = blk.addBlock("shader_glob_var_values");
  if (sgv_values)
    sgv_values->setFrom(&ShGlobVars);
  blk.setBool("av_skies_grp_hidden", av_skies_grp_hidden);
  blk.setBool("av_debug_grp_hidden", av_debug_grp_hidden);
  blk.setBool("av_envtex_grp_hidden", av_envtex_grp_hidden);

  float zn = 0.1f, zf = 10000.0f;
  get_app().getViewport(0)->getZnearZfar(zn, zf);
  blk.setReal("av_znear", zn);
  blk.setReal("av_zfar", zf);
}
}; // namespace environment

void environment::load_skies_settings(const DataBlock &blk)
{
  Tab<String> preset(midmem), env(midmem), wtype(midmem);
  int idx;

  av_skies_srv->fillPresets(preset, env, wtype);

  const char *preset_str = blk.getStr("preset", nullptr);
  if (!preset_str)
    preset_str = preset.size() > 0 ? preset[0].c_str() : "";
  String ps(::make_full_path(get_app().getWorkspace().getSdkDir(), preset_str));
  if (!dd_file_exists(ps))
  {
    ps = preset_str;
    if (!dd_file_exists(ps))
      ps = "";
  }
  if (ps.empty())
    get_app().getConsole().addMessage(ILogWriter::ERROR, ".asset-local/_av.blk: skies { preset:t=\"%s\"  not found or not set",
      blk.getStr("preset", ""));
  else
  {
    dd_strlwr(ps);
    simplify_fname(ps);
  }
  idx = find_value_idx(preset, ps);
  av_skies_preset = idx >= 0 ? preset[idx].str() : "";

  idx = find_value_idx(env, String(blk.getStr("env", "")));
  av_skies_env = env[idx >= 0 ? idx : 0];

  idx = find_value_idx(wtype, String(blk.getStr("wtype", "")));
  av_skies_wtype = wtype[idx >= 0 ? idx : 0];

  if (!av_skies_preset.empty() && !av_skies_env.empty() && !av_skies_wtype.empty())
    av_skies_srv->setWeather(av_skies_preset, av_skies_env, av_skies_wtype);
}

void environment::save_skies_settings(DataBlock &blk)
{
  blk.setStr("preset", ::make_path_relative(av_skies_preset, get_app().getWorkspace().getSdkDir()));
  blk.setStr("env", av_skies_env);
  blk.setStr("wtype", av_skies_wtype);
}

void environment::on_asset_changed(const DagorAsset &asset, AssetLightData &ald)
{
  if (strcmp(asset.getName(), ald.paintDetailsTexAsset) != 0 && strcmp(asset.getName(), "assets_color_global_tex_palette") != 0)
    return;

  logdbg("Updating paint details texture asset %s", asset.getName());
  ald.setPaintDetailsTexture();
}

void AssetLightData::setReflectionTexture()
{
  if (!shaderVar.empty() && !textureName.empty())
  {
    String texPath;
    if ((strlen(textureName) > 2) && (textureName[1] != ':'))
      texPath.printf(512, "%s/%s", get_app().getWorkspace().getAppDir(), textureName.str());
    else
      texPath = textureName;

    TEXTUREID texId = add_managed_texture(texPath);

    {
      int varId = get_shader_variable_id(shaderVar, true);
      if (!VariableMap::isGlobVariablePresent(varId))
        get_app().getConsole().addMessage(ILogWriter::WARNING, "no shader variable '%s'", shaderVar.str());
      else if (!ShaderGlobal::set_texture(varId, texId))
        get_app().getConsole().addMessage(ILogWriter::ERROR, "error on loading texture: '%s', id=%d", textureName.str(), texId);
    }
  }
}

static void decode_rel_fname(String &dest, const char *src)
{
  if (src[0] && src[1] != ':')
    dest.printf(512, "%s%s", get_app().getWorkspace().getAppDir(), src);
  else
    dest = src;
}

void AssetLightData::setEnvironmentTexture(bool require_lighting_update)
{
  if (!envTextureName.empty())
  {
    String texPath;
    decode_rel_fname(texPath, envTextureName);
    texPath += "?p1";

    environment::set_texture(add_managed_texture(texPath));
  }
  else
    environment::set_texture(BAD_TEXTUREID);
  environment::textureStretch = envTextureStretch;

  String loc_path;
  if (!envBlkFn.empty())
    decode_rel_fname(loc_path, envBlkFn);
  EDITORCORE->queryEditorInterface<IDynRenderService>()->setEnvironmentSnapshot(loc_path, envTextureName.empty());
  if (require_lighting_update)
    EDITORCORE->queryEditorInterface<IDynRenderService>()->onLightingSettingsChanged();
}

void AssetLightData::setPaintDetailsTexture()
{
  static int paintDetailsVarId = get_shader_variable_id("paint_details_tex", true);
  String localPaintTexPath;
  TEXTUREID localPaintColorsTexId, globalPaintColorsTexId;
  globalPaintColorsTexId = get_managed_texture_id("assets_color_global_tex_palette*");
  if (!paintDetailsTexAsset.empty())
    localPaintColorsTexId = get_managed_texture_id(String(0, "%s*", paintDetailsTexAsset));
  else
    localPaintColorsTexId = globalPaintColorsTexId;

  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(environment::combinedPaintTexId, environment::combinedPaintTex);
  Tab<TEXTUREID> tex_ids;
  tex_ids.push_back(globalPaintColorsTexId);
  tex_ids.push_back(localPaintColorsTexId);
  acquire_managed_tex(globalPaintColorsTexId);
  acquire_managed_tex(localPaintColorsTexId);
  prefetch_and_wait_managed_textures_loaded(tex_ids, true);
  environment::combinedPaintTex =
    texture_util::stitch_textures_horizontal(tex_ids, "combined_paint_tex", TEXFMT_DEFAULT | TEXCF_SRGBREAD).release();
  if (environment::combinedPaintTex)
  {
    environment::combinedPaintTexId = register_managed_tex("paint_details_tex", environment::combinedPaintTex);
    environment::combinedPaintTex->texfilter(TEXFILTER_POINT);
    TextureInfo texInfo;
    environment::combinedPaintTex->getinfo(texInfo);
    ShaderGlobal::set_real(get_shader_variable_id("paint_details_tex_inv_h", true), safediv(1.f, (float)texInfo.h));
  }
  else if (VariableMap::isGlobVariablePresent(paintDetailsVarId))
    DAEDITOR3.conError("failed to create combined painting texture (globTid=0x%x(%s) localTid=0x%x paintDetailsTexAsset='%s' var=%d)",
      globalPaintColorsTexId, get_managed_texture_name(globalPaintColorsTexId), localPaintColorsTexId, paintDetailsTexAsset,
      paintDetailsVarId);
  release_managed_tex(globalPaintColorsTexId);
  release_managed_tex(localPaintColorsTexId);
  ShaderGlobal::set_texture(paintDetailsVarId, environment::combinedPaintTexId);

  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(environment::singlePaintColorTexId, environment::singlePaintColorTex);
  environment::updatePaintColorTexture();
}

void AssetLightData::applyMicrodetailFromLevelBlk()
{
  static DataBlock microDetails;
  static TEXTUREID landMicrodetailsId = BAD_TEXTUREID;

  String abs_fn;
  decode_rel_fname(abs_fn, envLevelBlkFn);
  if (!dd_file_exists(abs_fn) && dd_file_exists(envLevelBlkFn))
    abs_fn = envLevelBlkFn;

  DataBlock level_blk;
  if (!abs_fn.empty())
    level_blk.load(abs_fn);

  const DataBlock &blk = *level_blk.getBlockByNameEx("micro_details");
  if (microDetails != blk)
  {
    DAEDITOR3.conNote("load micro details: %s", envLevelBlkFn);
    ShaderGlobal::reset_from_vars(landMicrodetailsId);
    release_managed_tex(landMicrodetailsId);
    landMicrodetailsId = load_land_micro_details(blk);
    microDetails = blk;
  }
}

void AssetLightData::loadDefaultSettings(DataBlock &app_blk)
{
  const DataBlock &ltData = *app_blk.getBlockByNameEx("AssetLight");
  envTextureName = ltData.getStr("environment_texture_name", NULL);

  textureName = ltData.getStr("texture_name", NULL);
  shaderVar = ltData.getStr("shader_var", NULL);

  Point3 ltColor = ltData.getPoint3("color", Point3(0.5, 0.5, 0.5));
  color.r = ltColor.x;
  color.g = ltColor.y;
  color.b = ltColor.z;

  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  SunLightProps *defSun = NULL;
  if (ltSrv->getSunCount() > 0)
    defSun = ltSrv->getSun(0);

  specPower = ltData.getReal("spec_power", defSun ? defSun->specPower : 0.f);
  specStrength = ltData.getReal("spec_strength", defSun ? defSun->specStrength : 0.f);

  rdbgType = ltData.getInt("rdbgType", 0);
  rdbgUseNmap = ltData.getBool("rdbgUseNmap", true);
  rdbgUseDtex = ltData.getBool("rdbgUseDtex", true);
  rdbgUseLt = ltData.getBool("rdbgUseLt", true);
  rdbgUseChrome = ltData.getBool("rdbgUseChrome", true);
}


static SunLightProps sunData;

SunLightProps *AssetLightData::createSun()
{
  ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>();

  if (ltSrv->getSunCount() < 1)
    ltSrv->setSunCount(1);
  SunLightProps *slp = ltSrv->getSun(0);

  G_ASSERT(slp);

  return slp;
}
