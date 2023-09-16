#include "plugin_scn.h"
#include "strmlevel.h"
#include <de3_interface.h>
#include <de3_lightService.h>
#include <de3_hmapService.h>
#include <de3_rendInstGen.h>

#include <shaders/dag_rendInstRes.h>
#include <scene/dag_objsToPlace.h>
#include <scene/dag_physMat.h>
#include <sceneRay/dag_sceneRay.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_direct.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/clipmap.h>
#include <landMesh/lastClip.h>
#include <rendInst/rendInstGen.h>
#include <de3_entityFilter.h>
#include <oldEditor/de_workspace.h>
#include <sepGui/wndGlobal.h>

#include <gameRes/dag_gameResSystem.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <3d/dag_materialData.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <libTools/util/strUtil.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <landMesh/lmeshRenderer.h>
#include <heightmap/heightmapHandler.h>
#include <pathFinder/pathFinder.h>
#include <de3_skiesService.h>
#include <de3_waterSrv.h>
#include <debug/dag_debug3d.h>
#include <render/toroidalHeightmap.h>


static const int WATER_NORMALS_MIPS = 8;

#define MIN_CLIPMAP_SCALE 0.5f
#define MAX_CLIPMAP_SCALE 4.f
#define MIN_TEXEL_SIZE    0.008f
#define MAX_TEXEL_SIZE    2.0f

static ISkiesService *skiesSrv = NULL;
static int landmesh_shaderlodVarid = -1;
static int waterPhaseVarId = -1;
static int normalMapFrameTexVarId = -1;
static int farNormalMapFrameTexVarId = -1;
static int skySphereFogDistVarId = -1;
static int skySphereFogHeightVarId = -1;
static int fromSunDirectionVarId = -1;
static int cloudsSunLightDir0VarId = -1;
static int sunColor0VarId = -1;
static int sunLightDir1VarId = -1;
static int sunColor1VarId = -1;
static int skyColorVarId = -1;
static int worldViewPosVarId = -1;
static int packedZVarId = -1;
static int waterSurfSubtypeMask = -1;
static int decal_ent_mask = -1, decal3d_ent_mask = -1;
static int rend_ent_geom_mask = -1, land_mesh_mask = -1, hmap_mask = -1;
static IGenEditorPlugin *riPlugin = NULL;

enum
{
  SHADERLOD_CLIPMAP = 0,
  SHADERLOD_CLIPSHADOWS = 1,
  SHADERLOD_SIMPLEST = 2,
  MAX_SHADERLOD_COUNT
};

extern bool(__stdcall *external_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm);
extern void generic_rendinstgen_service_set_callbacks(bool(__stdcall *get_height)(Point3 &pos, Point3 *out_norm),
  bool(__stdcall *trace_ray)(const Point3 &pos, const Point3 &dir, real &t, Point3 *out_norm),
  vec3f(__stdcall *update_pregen_pos_y)(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy));
static Tab<char> pregenEntStor(midmem);

enum
{
  RENDERING_WITH_SPLATTING = -1,
  RENDERING_LANDMESH = 0,
  RENDERING_CLIPMAP = 1,
  OBSOLETE_RENDERING_SPOT,
  GRASS_COLOR_OBSOLETE = 3,
  GRASS_MASK = 4,
  OBSOLETE_SPOT_TO_GRASS_MASK,
  RENDERING_HEIGHTMAP = 6,
  RENDERING_DEPTH = 7,
  RENDERING_VSM = 8,
  RENDERING_REFLECTION = 9,
  RENDERING_FEEDBACK = 10,
  LMESH_MAX
};

static int lmesh_rendering_mode_glob_varId = -1;
static int lmesh_rendering_mode = RENDERING_LANDMESH;
int set_lmesh_rendering_mode(int mode)
{
  G_ASSERT(mode < LMESH_MAX);
  if (lmesh_rendering_mode == mode)
    return lmesh_rendering_mode;
  int old = lmesh_rendering_mode;
  lmesh_rendering_mode = mode;
  ShaderGlobal::set_int_fast(lmesh_rendering_mode_glob_varId, mode);
  return old;
}

static String levelFileName;
static int inEditorGvId = -1;

static int frameBlkId = -1;
static int globalConstBlkId = -1;

static FastNameMap req_res_list;
static void add_resource_cb(const char *resname) { req_res_list.addNameId(resname); }

AcesScene *AcesScene::self = NULL;

namespace splinepoint
{
static TEXTUREID texPt = BAD_TEXTUREID;
static float ptScreenRad = 7.0;
static float ptScreenRadSm = 4.0;
static int ptRenderPassId = -1;
static DynRenderBuffer *ptDynBuf = NULL;

static void init()
{
  if (texPt == BAD_TEXTUREID)
  {
    texPt = add_managed_texture(::make_full_path(sgg::get_exe_path_full(), "../commonData/tex/point.tga"));
    acquire_managed_tex(texPt);
  }
  if (ptRenderPassId == -1)
    ptRenderPassId = get_shader_variable_id("editor_rhv_tex_pass", true);
  ptDynBuf = new DynRenderBuffer("editor_rhv_tex");
}
static void term()
{
  del_it(ptDynBuf);
  if (texPt != BAD_TEXTUREID)
  {
    release_managed_tex(texPt);
    texPt = BAD_TEXTUREID;
  }
}
} // namespace splinepoint

AcesScene::AcesScene() :
  hmapEnviBlk(NULL),
  bindumpHandle(NULL),
  lightmapTexId(BAD_TEXTUREID),
  levelMapTexId(BAD_TEXTUREID),
  objectsToPlace(midmem),
  skipEnviData(false),
  pendingBuildDf(true),
  waterService(NULL),
  lastClipTexSz(8192),
  rebuildLastClip(true),
  loadingAllRequiredResources(false),
  hasWater(false),
  dynamicZNear(1),
  dynamicZFar(10000),
  intited(false),
  cameraHeight(0.f)
{
  self = this;

  skiesSrv = DAGORED2->queryEditorInterface<ISkiesService>();
  if (waterSurfSubtypeMask == -1)
    waterSurfSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("water_surf");
  if (decal_ent_mask == -1)
    decal_ent_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal_geom");
  if (decal3d_ent_mask == -1)
    decal3d_ent_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal3d_geom");
  if (rend_ent_geom_mask == -1)
    rend_ent_geom_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
  if (land_mesh_mask == -1)
    land_mesh_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lmesh_obj");
  if (hmap_mask == -1)
    hmap_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("hmap_obj");

  zRange[0] = 0.1f;
  zRange[1] = 20000.f;

  lmeshMgr = NULL;
  lmeshRenderer = NULL;
  clipmap = NULL;
  toroidalHeightmap = NULL;

  fromSunDirectionVarId = sunColor0VarId = skyColorVarId = -1;
  sunLightDir1VarId = sunColor1VarId = -1;
  cloudsSunLightDir0VarId = -1;
  landMeshOffset = -1;

  intited = true;

  inEditorGvId = ::get_shader_glob_var_id("in_editor");
  frameBlkId = ShaderGlobal::getBlockId("global_frame");
  globalConstBlkId = ShaderGlobal::getBlockId("global_const_block");
  lmesh_rendering_mode_glob_varId = ::get_shader_glob_var_id("lmesh_rendering_mode");
  landmesh_shaderlodVarid = ::get_shader_glob_var_id("landmesh_shaderlod", true);

  generic_rendinstgen_service_set_callbacks(&custom_get_height, NULL, &custom_update_pregen_pos_y);

  dynamicZNear = ::dgs_get_game_params()->getReal("dynamicZNear", 3.f);
  dynamicZFar = ::dgs_get_game_params()->getReal("dynamicZFar", 10000.f);
  splinepoint::init();

  shaders::OverrideState flipCullState;
  flipCullState.set(shaders::OverrideState::FLIP_CULL);
  flipCullStateId = shaders::overrides::create(flipCullState);

  shaders::OverrideState blendOpMaxState;
  blendOpMaxState.set(shaders::OverrideState::BLEND_OP);
  blendOpMaxState.blendOp = BLENDOP_MAX;
  blendOpMaxStateId = shaders::overrides::create(blendOpMaxState);

  shaders::OverrideState zFuncLessState;
  zFuncLessState.set(shaders::OverrideState::Z_FUNC);
  zFuncLessState.zFunc = CMPF_LESS;
  zFuncLessStateId = shaders::overrides::create(zFuncLessState);
}


AcesScene::~AcesScene()
{
  self = NULL;
  clear();
  splinepoint::term();

  generic_rendinstgen_service_set_callbacks(NULL, NULL, NULL);

  ShaderGlobal::reset_textures(true);
}

void AcesScene::setEnvironmentSettings(DataBlock &enviBlk)
{
  G_ASSERT(enviBlk.isValid());

  const DataBlock *sceneBlk = enviBlk.getBlockByNameEx("scene");
  zRange[0] = sceneBlk->getReal("z_near", 0.1f);
  zRange[1] = sceneBlk->getReal("z_far", 20000.f);
  float skyScale = sceneBlk->getReal("skyHdrMultiplier", 4.f);

  Point3 skyColorPoint3 = enviBlk.getPoint3("sky_dl_color", Point3(0.5f, 0.5f, 0.5f));
  skyColor = Color3(skyColorPoint3.x, skyColorPoint3.y, skyColorPoint3.z);

  skyColorInitial = skyColor;
  ShaderGlobal::set_color4_fast(skyColorVarId, skyColor.r, skyColor.g, skyColor.b, 0.f);

  Point3 sunColorPoint3 = enviBlk.getPoint3("sun_dl_color", Point3(0.5f, 0.5f, 0.5f));
  sunColor0 = Color3(sunColorPoint3.x, sunColorPoint3.y, sunColorPoint3.z);
  ShaderGlobal::set_color4_fast(sunColor0VarId, sunColor0.r, sunColor0.g, sunColor0.b, 0.f);
  sunDir0 = enviBlk.getPoint3("sun_dl_dir", Point3(0.f, 1.f, 0.f));
  ShaderGlobal::set_color4_fast(fromSunDirectionVarId, sunDir0.x, sunDir0.y, sunDir0.z, 0.f);
  ShaderGlobal::set_color4_fast(cloudsSunLightDir0VarId, sunDir0.x, sunDir0.y, sunDir0.z, 0.f);

  sunColor0Initial = sunColor0;
  sunDir0Initial = sunDir0;

  hmapEnviBlk = new DataBlock;
  hmapEnviBlk->setFrom(enviBlk.getBlockByNameEx("hmap"));

  Color3 avg = 0.5 * (sunColor0Initial + skyColorInitial);

  if (lmeshMgr && !lmeshRenderer)
    lmeshRenderer = lmeshMgr->createRenderer();

  delete hmapEnviBlk;
  hmapEnviBlk = NULL;
}

static int znZfarVarId = -1, transformZVarId = -1;

void AcesScene::setZTransformPersp() { setZTransformPersp(zRange[0], zRange[1]); }

void AcesScene::setZTransformPersp(float zn, float zf)
{
  float q = zf / (zf - zn);
  ShaderGlobal::set_color4_fast(transformZVarId, q, 1, -zn * q, 0);
  ShaderGlobal::set_color4_fast(znZfarVarId, zn, zf, 0, 0);
}

void AcesScene::loadLevel(const char *bindump)
{
  if (!bindump || !*bindump)
    return;

  debug("[load]bindump begin");
  if (!dd_file_exist(bindump))
  {
    DAEDITOR3.conError("Level binary file not found: %s", bindump);
    return;
  }
  rebuildLastClip = true;
  lmeshRenderer = NULL;
  hasWater = false;
  waterService = DAGORED2->queryEditorInterface<IWaterService>();

  // init global shader variables
  fromSunDirectionVarId = ::get_shader_glob_var_id("from_sun_direction");
  sunColor0VarId = ::get_shader_glob_var_id("sun_color_0");
  skyColorVarId = ::get_shader_glob_var_id("sky_color");

  worldViewPosVarId = get_shader_glob_var_id("world_view_pos");
  znZfarVarId = get_shader_glob_var_id("zn_zfar", true);
  transformZVarId = get_shader_glob_var_id("transform_z", true);


  skySphereFogDistVarId = ::get_shader_glob_var_id("envi_fog_horizon_radius", true);
  skySphereFogHeightVarId = ::get_shader_glob_var_id("envi_fog_height_at_horizon", true);

  levelFileName = bindump;
  clear();

  // prepare clipmap
  del_it(clipmap);
  del_it(toroidalHeightmap);

  DataBlock appblk(String(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir()));
  const DataBlock &clipmapBlk = *appblk.getBlockByNameEx("clipmap");
  float clipmapScale = safediv(1.0f, sqrtf(clamp(clipmapBlk.getReal("clipmapScale", 1.f), MIN_CLIPMAP_SCALE, MAX_CLIPMAP_SCALE)));
  float texelSize = clamp(clipmapBlk.getReal("texelSize", 0.5) * clipmapScale, MIN_TEXEL_SIZE, MAX_TEXEL_SIZE);

  int prevAniso = ::dgs_tex_anisotropy;
  if (1)
    ::dgs_tex_anisotropy = 1;

  bool useToroidalHeightmap = clipmapBlk.getBool("useToroidalHeightmap", false);

  clipmap = new Clipmap;

  clipmap->init(texelSize, Clipmap::CPU_HW_FEEDBACK);
  clipmap->initVirtualTexture(4096, 8192);

  carray<uint32_t, 4> bufFormats;
  int bufCnt = min<int>(bufFormats.size(), clipmapBlk.getInt("bufferCnt", 0));
  carray<uint32_t, 4> formats;
  int cacheCnt = min<int>(formats.size(), clipmapBlk.getInt("cacheCnt", 0));
  for (int i = 0; i < bufCnt; ++i)
    bufFormats[i] = parse_tex_format(clipmapBlk.getStr(String(32, "buf_tex%d", i), "NONE"), TEXFMT_DEFAULT);
  for (int i = 0; i < cacheCnt; ++i)
    formats[i] = parse_tex_format(clipmapBlk.getStr(String(32, "cache_tex%d", i), "NONE"), TEXFMT_DEFAULT);
  clipmap->createCaches(formats.data(), cacheCnt, bufFormats.data(), bufCnt);

  if (useToroidalHeightmap)
  {
    toroidalHeightmap = new ToroidalHeightmap;
    toroidalHeightmap->init(2048, 32.0f, 96.0f, TEXFMT_L8, 128);
  }

  ::dgs_tex_anisotropy = prevAniso;

  const Driver3dDesc &desc = d3d::get_driver_desc();
  lastClipTexSz = min(clipmapBlk.getInt("lastClipTexSz", 4096), min(desc.maxtexw, desc.maxtexh));

  //
  // Load .bin.
  //
  disable_auto_load_tex_after_binary_dump_loaded();

  bindumpHandle = load_binary_dump(bindump, *this);

  if (!bindumpHandle)
    fatal("can't load binary level dump: %s", bindump);

  if (lmeshMgr && !lmeshRenderer)
    lmeshRenderer = lmeshMgr->createRenderer();

  // place resource objects
  placeResObjects();

  // Load lightmap.
  char levelFileNameWithoutExt[1000];
  const char *ext = dd_get_fname_ext(levelFileName);
  G_ASSERT(ext);
  strncpy(levelFileNameWithoutExt, levelFileName, ext - levelFileName);
  levelFileNameWithoutExt[ext - levelFileName] = 0;

  String lightmapFileName(256, "%s_lightmap", dd_get_fname(levelFileNameWithoutExt));

  lightmapTexId = ::get_tex_gameres(lightmapFileName);
  TextureInfo tinfo;
  BaseTexture *tex = acquire_managed_tex(lightmapTexId);
  if (tex)
  {
    tex->getinfo(tinfo);
    release_managed_tex(lightmapTexId);
  }
  else
  {
    tinfo.w = tinfo.h = 128;
    DAEDITOR3.conWarning("cannot get texture gameres <%s>", lightmapFileName.str());
  }

  if (lmeshMgr)
  {
    ShaderGlobal::set_texture(get_shader_variable_id("lightmap_tex"), lightmapTexId);
    Color4 world_to_lightmap(1.f / (lmeshMgr->getNumCellsX() * lmeshMgr->getLandCellSize()),
      1.f / (lmeshMgr->getNumCellsY() * lmeshMgr->getLandCellSize()), -(float)lmeshMgr->getCellOrigin().x / lmeshMgr->getNumCellsX(),
      -(float)lmeshMgr->getCellOrigin().y / lmeshMgr->getNumCellsY());

    world_to_lightmap.b += 0.5 * tinfo.w;
    world_to_lightmap.a += 0.5 * tinfo.h;
    ShaderGlobal::set_color4(get_shader_variable_id("world_to_lightmap"), world_to_lightmap);
  }


  // Load Level Map:
  String levelMapFileName(1000, "%s_map", dd_get_fname(levelFileNameWithoutExt));
  levelMapTexId = ::get_tex_gameres(levelMapFileName);
  tex = acquire_managed_tex(levelMapTexId);
  release_managed_tex(levelMapTexId);

  String fn(0, "levels/%s.blk", ::dd_get_fname(levelFileNameWithoutExt));
  String app_root(DAGORED2->getWorkspace().getAppDir());

  class LevelsFolderIncludeFileResolver : public DataBlock::IIncludeFileResolver
  {
  public:
    LevelsFolderIncludeFileResolver() : prefix(tmpmem), appDir(NULL) {}
    virtual bool resolveIncludeFile(String &inout_fname)
    {
      String fn;
      for (int i = 0; i < prefix.size(); i++)
      {
        fn.printf(0, "%s/%s/levels/%s", appDir, prefix[i], inout_fname);
        if (dd_file_exists(fn))
        {
          inout_fname = fn;
          return true;
        }
      }
      return false;
    }
    void preparePrefixes(const DataBlock *b, const char *app_dir)
    {
      prefix.clear();
      appDir = app_dir;
      if (!b)
        return;
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == DataBlock::TYPE_STRING)
          prefix.push_back(b->getStr(i));
    }
    Tab<const char *> prefix;
    const char *appDir;
  };
  static LevelsFolderIncludeFileResolver inc_resv;

  inc_resv.preparePrefixes(appblk.getBlockByName("levelsBlkPrefix"), app_root);
  DataBlock::setIncludeResolver(&inc_resv);

  debug("Loading level settings from \"%s\"", fn);
  bool loaded = false;
  int prefix_tried = 0;
  for (int i = 0; i < inc_resv.prefix.size(); i++)
  {
    String fpath(0, "%s/%s/%s", app_root, inc_resv.prefix[i], fn);
    if (dd_file_exists(fpath) && levelSettingsBlk.load(fpath))
    {
      loaded = true;
      break;
    }
    else
    {
      debug("%s is %s", fpath, dd_file_exists(fpath) ? "CORRUPT" : "MISSING");
      prefix_tried++;
    }
  }

  if (!loaded)
    loaded = levelSettingsBlk.load(fn);

  if (!loaded)
    DAEDITOR3.conWarning("cannot load %s (tried also with %d prefixes)", fn, prefix_tried);
  if (IHmapService *hmlService = DAGORED2->queryEditorInterface<IHmapService>())
    hmlService->onLevelBlkLoaded(loaded ? levelSettingsBlk : DataBlock());
  if (IRendInstGenService *rendInstGenService = DAGORED2->queryEditorInterface<IRendInstGenService>())
    rendInstGenService->onLevelBlkLoaded(loaded ? levelSettingsBlk : DataBlock());

  DataBlock::setRootIncludeResolver(app_root);
  if (ISkiesService *skiesSrv = DAGORED2->queryEditorInterface<ISkiesService>())
  {
    skiesSrv->overrideWeather(0, NULL, NULL, -1, NULL, levelSettingsBlk.getBlockByNameEx("stars"));
    //== double update to get proper sun position
    skiesSrv->overrideWeather(2, NULL, NULL, 1234, NULL, NULL);
    skiesSrv->overrideWeather(2, NULL, NULL, -1, NULL, NULL);
  }

  if (lmeshMgr && lmeshRenderer)
  {
    lmeshRenderer->setMirroring(*lmeshMgr, levelSettingsBlk.getInt("numBorderCellsXPos", 2),
      levelSettingsBlk.getInt("numBorderCellsXNeg", 2), levelSettingsBlk.getInt("numBorderCellsZPos", 2),
      levelSettingsBlk.getInt("numBorderCellsZNeg", 2), levelSettingsBlk.getReal("mirrorShrinkXPos", 0.f),
      levelSettingsBlk.getReal("mirrorShrinkXNeg", 0.f), levelSettingsBlk.getReal("mirrorShrinkZPos", 0.f),
      levelSettingsBlk.getReal("mirrorShrinkZNeg", 0.f));
  }

  // Make sure FRT is not missing and loaded properly
  if (!frtDump.isDataValid())
    DAEDITOR3.conWarning("Static collision (FRT) is absent!");

  // Make sure there is no lmesh in static scene collision.

  float sceneDist = 2000.f;
  if (tracerayNormalizedStaticScene(Point3(0.f, 1000.f, 0.f), Point3(0.f, -1.f, 0.f), sceneDist, NULL, NULL))
  {
    float heightmapDist = 2000.f;
    tracerayNormalizedHeightmap(Point3(0.f, 1000.f, 0.f), Point3(0.f, -1.f, 0.f), heightmapDist, NULL);
  }

  waterLevel = levelSettingsBlk.getReal("water_level", 0.f);

  // Set zbiases.

  const DataBlock *zbiasBlk = ::dgs_get_game_params()->getBlockByNameEx("zbias");

  ShaderGlobal::set_real(::get_shader_variable_id("static_zbias", true), zbiasBlk->getReal("staticZbias", 0.f));

  float water_zbias = zbiasBlk->getReal("waterZbias", 0.f);
  ShaderGlobal::set_real(::get_shader_variable_id("water_zbias", true), water_zbias);
  ShaderGlobal::set_real(::get_shader_variable_id("bottom_zbias", true), water_zbias * 1.1);

  ShaderGlobal::set_vars_from_blk(*levelSettingsBlk.getBlockByNameEx("shader_vars"));

  waterService->init();
  waterService->loadSettings(DataBlock());
  if (!hasWater && lmeshMgr && lmeshMgr->getBBox()[0].y < waterLevel)
    hasWater = true;
  if (hasWater && waterService)
  {
    waterService->set_level(waterLevel);
    Point2 windDir(0.6, 0.8);
    windDir = levelSettingsBlk.getPoint2("windDir", windDir);
    const DataBlock *weatherTypeBlk = NULL;
    if (ISkiesService *skiesSrv = DAGORED2->queryEditorInterface<ISkiesService>())
      weatherTypeBlk = skiesSrv->getWeatherTypeBlk();

    float stormStrength =
      (weatherTypeBlk ? weatherTypeBlk->getReal("wind_strength", 4) : 4) * levelSettingsBlk.getReal("wind_scale", 1);
    stormStrength = min(stormStrength, levelSettingsBlk.getReal("max_wind_strength", 6));
    stormStrength = max(stormStrength, 1.0f);
    waterService->set_wind(stormStrength, windDir);

    BBox2 water3DQuad;
    water3DQuad[0] = levelSettingsBlk.getPoint2("waterCoord0", Point2(-256000, -256000));
    water3DQuad[1] = levelSettingsBlk.getPoint2("waterCoord1", Point2(+256000, +256000));
    waterService->set_render_quad(water3DQuad.width().x < 128000 ? water3DQuad : BBox2());
    pendingBuildDf = true;
  }
  else if (!hasWater)
    if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_waterSurf"))
      p->setVisible(false);
  riPlugin = DAGORED2->getPluginByName("_riEntMgr");
  if (lmeshMgr)
    collisionpreview::addLandRtMeshCollision(lrtCollision, lmeshMgr->getLandTracer());
}


void AcesScene::clear()
{
  if (waterService)
    waterService->term();
  ShaderGlobal::reset_textures(true);

  del_it(lmeshMgr);
  del_it(lmeshRenderer);

  BaseStreamingSceneHolder::clear();

  release_managed_tex(lightmapTexId);
  lightmapTexId = BAD_TEXTUREID;

  unload_binary_dump(bindumpHandle, false);
  bindumpHandle = NULL;
  delayed_binary_dumps_unload();

  release_managed_tex(levelMapTexId);
  levelMapTexId = BAD_TEXTUREID;

  frtDump.unloadData();
  lrtCollision.clear();

  rendinst::clearRIGen();
  clear_and_shrink(pregenEntStor);
  pathfinder::clear();
  del_it(clipmap);
  closeFixedClip();
  hasWater = false;

  clear_and_shrink(splines);
}


bool AcesScene::bdlCustomLoad(unsigned bindump_id, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> texId)
{
  BaseStreamingSceneHolder::bdlCustomLoad(bindump_id, tag, crd, texId);


  if (tag == _MAKE4C('RqRL'))
  {
    return true;
  }


  if (tag == _MAKE4C('lmap'))
  {
    landMeshOffset = crd.tell();
    if (landMeshOffset >= 0)
    {
      debug("[load]landmesh");
      lmeshMgr = new LandMeshManager();
      debug("[load]landmesh data");

      if (!lmeshMgr->loadDump(crd))
      {
        delete lmeshMgr;
        lmeshMgr = NULL;
        debug("can't load lmesh");
      }
      ::dgs_tex_anisotropy = 1;
    }

    crd.seekrel(crd.getBlockRest());
    debug("lmesh=%d", landMeshOffset);

    return true;
  }

  if (tag == _MAKE4C('HM2'))
  {
    G_ASSERT(lmeshMgr);
    if (!lmeshMgr->loadHeightmapDump(crd, true))
      return false;
    return true;
  }

  if (tag == _MAKE4C('FRT'))
  {
    debug("[load]frt");
    bool res = true;
    try
    {
      frtDump.loadData(crd);
      res = frtDump.isDataValid();
    }
    catch (...)
    {
      debug_ctx("failed to load FRT");
      frtDump.unloadData();
      res = false;
    }
    return res;
  }
  if (tag == _MAKE4C('RIGz'))
  {
    debug("[load] RI generator");
    rendinst::RIGenLoadingAutoLock riGenLd;
    req_res_list.reset();
    rendinst::loadRIGen(crd, add_resource_cb, false, &pregenEntStor);
    ::set_required_res_list_restriction(req_res_list);
    rendinst::prepareRIGen();
    ::reset_required_res_list_restriction();
  }

  if (tag == _MAKE4C('Lnav'))
    return pathfinder::loadNavMesh(crd);

  if (tag == _MAKE4C('Obj'))
  {
    objectsToPlace.push_back(ObjectsToPlace::make(crd, crd.getBlockRest()));
    return true;
  }

  if (tag == _MAKE4C('wt3d'))
  {
    hasWater = true;
    return true;
  }
  if (tag == _MAKE4C('hspl'))
  {
    splines.resize(crd.readInt());
    SmallTab<Point3, TmpmemAlloc> pts;

    for (int i = 0; i < splines.size(); ++i)
    {
      crd.readString(splines[i].name);

      clear_and_resize(pts, crd.readInt() * 3);
      crd.readTabData(pts);
      splines[i].spl.calculate(pts.data(), pts.size(), false);
      // DAEDITOR3.conNote("spline %d, %s,  %d points", i, splines[i].name, pts.size()/3);
    }
    DAEDITOR3.conNote("loaded %d splines", splines.size());
    return true;
  }

  debug("[load]bindump end");
  return true;
}

void AcesScene::delBinDump(unsigned bindump_id) { BaseStreamingSceneHolder::delBinDump(bindump_id); }

void AcesScene::unloadAllBinDumps() { BaseStreamingSceneHolder::unloadAllBinDumps(); }

void AcesScene::update(const Point3 &op, float dt)
{
  if (!bindumpHandle)
    return;

  BaseStreamingSceneHolder::act(op);
  if (!riPlugin || riPlugin->getVisible())
    rendinst::scheduleRIGenPrepare(make_span(&grs_cur_view.pos, 1));
  if (waterService)
    waterService->act(dt);
}


void AcesScene::setupShaderVars()
{
  ShaderGlobal::set_color4_fast(skyColorVarId, skyColor.r, skyColor.g, skyColor.b, 0.f);
  ShaderGlobal::set_color4_fast(sunColor0VarId, sunColor0.r, sunColor0.g, sunColor0.b, 0.f);
  ShaderGlobal::set_color4_fast(fromSunDirectionVarId, sunDir0.x, sunDir0.y, sunDir0.z, 0.f);

  ShaderGlobal::set_real_fast(skySphereFogHeightVarId, skySphereFogHeight);
  ShaderGlobal::set_real_fast(skySphereFogDistVarId, skySphereFogDist);
  if (skipEnviData)
    if (ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>())
      ltSrv->updateShaderVars();
}

bool AcesScene::getLandscapeVis()
{
  return IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & land_mesh_mask;
}
void AcesScene::setLandscapeVis(bool vis)
{
  if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_lmeshObj"))
    p->setVisible(vis);
}


void AcesScene::beforeRender()
{
  if (!bindumpHandle)
    return;

  ShaderGlobal::set_int_fast(inEditorGvId, 0);
  int st_mask = IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

  if (lmeshMgr)
    lmeshMgr->mayRenderHmap = (st_mask & hmap_mask) ? true : false;

  Driver3dPerspective p;
  if (d3d::getpersp(p))
  {
    zRange[0] = p.zn;
    zRange[1] = p.zf;
  }

  setupShaderVars();
  ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(globalConstBlkId, ShaderGlobal::LAYER_GLOBAL_CONST);

  if (lmeshRenderer)
  {
    cameraHeight = 15000;
    if (!tracerayNormalizedHeightmap(::grs_cur_view.pos, Point3(0, -1, 0), cameraHeight, NULL) && ::grs_cur_view.pos.y < 14000)
      cameraHeight = 20;

    renderClipmaps();
    lmeshRenderer->prepare(*lmeshMgr, ::grs_cur_view.pos, cameraHeight);
  }

  ShaderGlobal::set_int_fast(inEditorGvId, 1);
  rendinst::updateRIGenImpostors(10000, sunDir0, ::grs_cur_view.itm);

  if (lmeshMgr && pendingBuildDf)
  {
    BBox2 hmap_area;
    if (HeightmapHandler *h = lmeshMgr->getHmapHandler())
    {
      hmap_area[0].set_xz(h->getWorldBox()[0]);
      hmap_area[1].set_xz(h->getWorldBox()[1]);
    }
    else
    {
      hmap_area[0].set_xz(lmeshMgr->getBBox()[0]);
      hmap_area[1].set_xz(lmeshMgr->getBBox()[1]);
    }
    ShaderGlobal::set_int_fast(inEditorGvId, 0);
    waterService->buildDistanceField(hmap_area.center(), max(hmap_area.width().x, hmap_area.width().y) / 2, -1);
    ShaderGlobal::set_int_fast(inEditorGvId, 1);
    pendingBuildDf = false;
  }
}

void AcesScene::renderClipmaps()
{
  class LandmeshCMRenderer : public ClipmapRenderer, public ToroidalHeightmapRenderer
  {
  public:
    LandMeshRenderer &renderer;
    LandMeshManager &provider;
    float minZ, maxZ;
    int omode;
    TMatrix4 ovtm, oproj;
    bool perspvalid;
    Driver3dPerspective p;
    Tab<IRenderingService *> rendSrv;
    shaders::OverrideStateId flipCullStateId;
    LandmeshCMRenderer(LandMeshRenderer &r, LandMeshManager &p) : renderer(r), provider(p), omode(0)
    {
      shaders::OverrideState flipCullState;
      flipCullState.set(shaders::OverrideState::FLIP_CULL);
      flipCullStateId = shaders::overrides::create(flipCullState);
    }

    virtual void startRenderTiles(const Point2 &center)
    {
      Point3 pos(center.x, ::grs_cur_view.pos.y, center.y);
      BBox3 landBox = provider.getBBox();
      minZ = landBox[0].y - 10;
      maxZ = landBox[1].y + 10;

      omode = ::set_lmesh_rendering_mode(RENDERING_CLIPMAP);
      static int land_mesh_prepare_clipmap_blockid = ShaderGlobal::getBlockId("land_mesh_prepare_clipmap");
      ShaderGlobal::setBlock(land_mesh_prepare_clipmap_blockid, ShaderGlobal::LAYER_SCENE);
      renderer.prepare(provider, pos, ::grs_cur_view.pos.y);
      TMatrix vtm = TMatrix::IDENT;
      d3d::gettm(TM_VIEW, &ovtm);
      d3d::gettm(TM_PROJ, &oproj);
      perspvalid = d3d::getpersp(p);
      vtm.setcol(0, 1, 0, 0);
      vtm.setcol(1, 0, 0, 1);
      vtm.setcol(2, 0, 1, 0);
      shaders::overrides::set(flipCullStateId);
      d3d::settm(TM_VIEW, vtm);

      if (::grs_draw_wire)
        d3d::setwire(0);

      rendSrv.clear();
      for (int i = 0, ie = IEditorCoreEngine::get()->getPluginCount(); i < ie; ++i)
      {
        IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
        if (p->getVisible())
          if (IRenderingService *iface = p->queryInterface<IRenderingService>())
            rendSrv.push_back(iface);
      }
    }
    virtual void endRenderTiles()
    {
      renderer.setRenderInBBox(BBox3());
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      ::set_lmesh_rendering_mode(omode);
      d3d::settm(TM_VIEW, &ovtm);
      d3d::settm(TM_PROJ, &oproj);
      if (perspvalid)
        d3d::setpersp(p);
      if (::grs_draw_wire)
        d3d::setwire(1);
      shaders::overrides::reset();
    }

    virtual void renderTile(const BBox2 &region)
    {
      TMatrix4 proj;
      proj = matrix_ortho_off_center_lh(region[0].x, region[1].x, region[1].y, region[0].y, minZ, maxZ);
      d3d::settm(TM_PROJ, &proj);
      BBox3 region3(Point3(region[0].x, minZ, region[0].y), Point3(region[1].x, maxZ + 500, region[1].y));
      renderer.setRenderInBBox(region3);
      renderer.render(provider, renderer.RENDER_CLIPMAP);

      int old_st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, decal_ent_mask | decal3d_ent_mask);
      for (int i = 0; i < rendSrv.size(); i++)
        rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, old_st_mask);
    }
    virtual void renderFeedback(const TMatrix4 &globtm)
    {
      int omesh = ::set_lmesh_rendering_mode(RENDERING_FEEDBACK);
      renderer.render(provider, renderer.RENDER_DEPTH);
      static int land_mesh_render_depth_blockid = ShaderGlobal::getBlockId("land_mesh_render_depth");
      ShaderGlobal::setBlock(land_mesh_render_depth_blockid, ShaderGlobal::LAYER_SCENE);

      static int decal3d_ent_mask = -1;
      if (decal3d_ent_mask == -1)
        decal3d_ent_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal3d_geom");

      int old_st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, decal3d_ent_mask);
      for (int i = 0; i < rendSrv.size(); i++)
        rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, old_st_mask);

      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      ::set_lmesh_rendering_mode(omesh);
    }
  };

  if (rebuildLastClip)
  {
    rebuildLastClip = false;
    prepareFixedClip(lastClipTexSz);
    setFixedClipToShader();
  }

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  static int render_with_normalmapVarId = ::get_shader_glob_var_id("render_with_normalmap", true);
  ShaderGlobal::set_int_fast(render_with_normalmapVarId, 1);

  int w, h;
  d3d::get_target_size(w, h);
  clipmap->setTargetSize(w, h);

  TMatrix4 clipmapFrustum;
  d3d::getglobtm(clipmapFrustum);
  LandmeshCMRenderer landmeshCMRenderer(*lmeshRenderer, *lmeshMgr);
  clipmap->prepareFeedback(landmeshCMRenderer, ::grs_cur_view.pos, clipmapFrustum, cameraHeight + cameraHeight, 0.f);
  clipmap->finalizeFeedback();
  clipmap->prepareRender(landmeshCMRenderer);

  setFixedClipToShader();

  ShaderGlobal::set_int_fast(render_with_normalmapVarId, 2);
  if (toroidalHeightmap != NULL)
  {
    // render with parallax
    toroidalHeightmap->updateHeightmap(landmeshCMRenderer, ::grs_cur_view.pos, 0.0f, 0.5f);
  }
}

static void start_rendering_clipmap_cb()
{
  ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
  ::set_lmesh_rendering_mode(RENDERING_CLIPMAP);
}
static void render_decals_cb(const BBox3 &landBoxPart)
{
  Tab<IRenderingService *> rendSrv(tmpmem);
  for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
  {
    IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
    if (p->getVisible())
      if (IRenderingService *iface = p->queryInterface<IRenderingService>())
        rendSrv.push_back(iface);
  }

  unsigned old_st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, decal_ent_mask | decal3d_ent_mask);
  for (int i = 0; i < rendSrv.size(); i++)
    rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);
  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, old_st_mask);
}

void AcesScene::prepareFixedClip(int texture_size)
{
  closeFixedClip();
  if (!lmeshMgr || !lmeshRenderer)
    return;

  LandMeshData data;
  data.lmeshMgr = lmeshMgr;
  data.lmeshRenderer = lmeshRenderer;
  data.texture_size = texture_size;
  data.use_dxt = false; // true;
  data.start_render = start_rendering_clipmap_cb;
  data.decals_cb = render_decals_cb;
  data.global_frame_id = frameBlkId;
  data.flipCullStateId = flipCullStateId.get();
  int oldm = ShaderGlobal::get_int_fast(lmesh_rendering_mode_glob_varId);
  ShaderGlobal::enableAutoBlockChange(false);

  prepare_fixed_clip(last_clip, data, false);
  ::set_lmesh_rendering_mode(oldm);
  ShaderGlobal::enableAutoBlockChange(true);
}

void AcesScene::setFixedClipToShader() { last_clip.setVar(); }

void AcesScene::closeFixedClip() { last_clip.close(); }

void AcesScene::render(bool render_rs)
{
  if (!bindumpHandle)
    return;

  int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

  static int water_level_gvid = get_shader_variable_id("water_level", true);
  ShaderGlobal::set_real(water_level_gvid, waterLevel);
  ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(globalConstBlkId, ShaderGlobal::LAYER_GLOBAL_CONST);

  clipmap->startUAVFeedback();
  ShaderGlobal::set_int_fast(inEditorGvId, 0);
  if (lmeshRenderer && (st_mask & land_mesh_mask))
  {
    set_lmesh_rendering_mode(RENDERING_LANDMESH);
    lmeshRenderer->render(*lmeshMgr, lmeshRenderer->RENDER_WITH_CLIPMAP);
  }
  clipmap->endUAVFeedback();
  clipmap->copyUAVFeedback();

  if (render_rs)
    BaseStreamingSceneHolder::render();
  if ((st_mask & rend_ent_geom_mask) && (!riPlugin || riPlugin->getVisible()))
    rendinst::renderRIGen(rendinst::RenderPass::Normal, ::grs_cur_view.pos, ::grs_cur_view.itm,
      rendinst::LAYER_OPAQUE | rendinst::LAYER_NOT_EXTRA);
  ShaderGlobal::set_int_fast(inEditorGvId, 1);
}

void AcesScene::renderTrans(bool render_rs)
{
  if (!bindumpHandle)
    return;

  ShaderGlobal::set_int_fast(inEditorGvId, 0);
  if (render_rs)
    BaseStreamingSceneHolder::renderTrans();
  ShaderGlobal::set_int_fast(inEditorGvId, 1);
}

void AcesScene::renderWater(IRenderingService::Stage stage)
{
  if (waterService == NULL)
    return;
  int st_mask = IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (!(st_mask & waterSurfSubtypeMask))
  {
    waterService->hide_water();
    return;
  }
  waterService->beforeRender(stage);
  waterService->renderGeometry(stage);
}
void AcesScene::renderHeight()
{
  if (!lmeshRenderer)
    return;

  DagorCurView savedView = ::grs_cur_view;
  lmeshRenderer->prepare(*lmeshMgr, lmeshMgr->getBBox().center(), cameraHeight);

  ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
  int oldMode = set_lmesh_rendering_mode(RENDERING_HEIGHTMAP);

  float oldInvGeomDist = lmeshRenderer->getInvGeomLodDist();
  lmeshRenderer->setRenderInBBox(lmeshMgr->getBBox());
  lmeshRenderer->setInvGeomLodDist(0.5 / lmeshMgr->getBBox().width().length());

  shaders::overrides::set(blendOpMaxStateId);
  lmeshRenderer->render(*lmeshMgr, lmeshRenderer->RENDER_ONE_SHADER);
  set_lmesh_rendering_mode(oldMode);
  shaders::overrides::reset();

  ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
  lmeshRenderer->setRenderInBBox(BBox3());
  lmeshRenderer->setInvGeomLodDist(oldInvGeomDist);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  ::grs_cur_view = savedView;
}

void AcesScene::renderShadows()
{
  if (!bindumpHandle)
    return;

  static int water_level_gvid = get_shader_variable_id("water_level", true);
  ShaderGlobal::set_real(water_level_gvid, waterLevel);

  ShaderGlobal::set_int_fast(inEditorGvId, 0);
  BaseStreamingSceneHolder::render();
  ShaderGlobal::set_int_fast(inEditorGvId, 1);
}
void AcesScene::renderShadowsVsm()
{
  if (!lmeshRenderer)
    return;

  ShaderGlobal::set_int_fast(inEditorGvId, 0);
  ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);

  DagorCurView savedView = ::grs_cur_view;
  lmeshRenderer->forceLowQuality(true);
  set_lmesh_rendering_mode(RENDERING_VSM);
  lmeshRenderer->render(*lmeshMgr, lmeshRenderer->RENDER_ONE_SHADER);
  lmeshRenderer->forceLowQuality(false);
  ::grs_cur_view = savedView;

  ShaderGlobal::set_int_fast(inEditorGvId, 1);
}

void AcesScene::renderSplineCurve(const BezierSpline3d &spl, bool opaque_pass)
{
  static const int RENDER_SPLINE_POINTS = 1000;
  real splineLen = spl.getLength();
  E3DCOLOR col(70, 170, 255, 255);
  int i = 0;
  if (!opaque_pass)
    col = E3DCOLOR(col.r * 3 / 5, col.g * 3 / 5, col.b * 4 / 5, 144);

  Tab<Point3> splinePoly(tmpmem);
  splinePoly.resize(RENDER_SPLINE_POINTS + 1);
  for (real t = 0.0; t < splineLen; t += splineLen / RENDER_SPLINE_POINTS, ++i)
    splinePoly[i] = spl.get_pt(t);

  draw_cached_debug_line(splinePoly.data(), i, col);
}
void AcesScene::renderSplines()
{
  ::begin_draw_cached_debug_lines(true, false, true);
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);
  for (int i = 0; i < splines.size(); ++i)
    renderSplineCurve(splines[i].spl, false);
  ::end_draw_cached_debug_lines();

  ::begin_draw_cached_debug_lines();
  for (int i = 0; i < splines.size(); ++i)
    renderSplineCurve(splines[i].spl, true);
  ::end_draw_cached_debug_lines();
}
void AcesScene::renderSplinePoints(float max_dist)
{
  bool ortho = DAGORED2->getRenderViewport()->isOrthogonal();
  Point3 cpos = grs_cur_view.pos;
  float dist2 = max_dist * max_dist;
  E3DCOLOR rendCol(70, 170, 255, 255);

  TMatrix4 gtm;
  Point2 scale;
  int w, h;

  d3d::get_target_size(w, h);
  d3d::getglobtm(gtm);
  scale.x = 2.0 / w;
  scale.y = 2.0 / h;

  splinepoint::ptDynBuf->clearBuf();
  for (int i = 0, cnt = 0; i < splines.size(); ++i)
    for (int j = 0; j < splines[i].spl.segs.size(); j++)
    {
      Point3 pt = splines[i].spl.segs[j].point(0);
      if (!ortho && lengthSq(pt - cpos) > dist2)
        continue;

      Point4 p = Point4::xyz1(pt) * gtm;
      Point2 s = scale * splinepoint::ptScreenRad;
      if (p.w > 0.0)
      {
        Point3 dx(s.x, 0, 0);
        Point3 dy(0, s.y, 0);
        Point3 p3(p.x / p.w, p.y / p.w, p.z / p.w);
        if (p3.z > 0 && p3.z < 1)
          splinepoint::ptDynBuf->drawQuad(p3 - dx - dy, p3 - dx + dy, p3 + dx + dy, p3 + dx - dy, rendCol, 1, 1);
      }

      cnt++;
      if (cnt >= 4096)
      {
        shaders::overrides::set(zFuncLessStateId);
        ShaderGlobal::set_int(splinepoint::ptRenderPassId, 0);
        splinepoint::ptDynBuf->flushToBuffer(splinepoint::texPt);
        shaders::overrides::reset();

        ShaderGlobal::set_int(splinepoint::ptRenderPassId, 1);
        splinepoint::ptDynBuf->flushToBuffer(splinepoint::texPt);
        splinepoint::ptDynBuf->clearBuf();
        cnt = 0;
      }
    }

  shaders::overrides::set(zFuncLessStateId);
  ShaderGlobal::set_int(splinepoint::ptRenderPassId, 0);
  splinepoint::ptDynBuf->flushToBuffer(splinepoint::texPt);
  shaders::overrides::reset();

  ShaderGlobal::set_int(splinepoint::ptRenderPassId, 1);
  splinepoint::ptDynBuf->flushToBuffer(splinepoint::texPt);
  splinepoint::ptDynBuf->flush();
}

void AcesScene::invalidateClipmap(bool force_redraw)
{
  if (clipmap)
    clipmap->invalidate(force_redraw);
  if (toroidalHeightmap)
    toroidalHeightmap->invalidate();
  rebuildLastClip = true;
}

void AcesScene::placeResObjects()
{
  for (int i = 0; i < objectsToPlace.size(); i++)
  {
    ObjectsToPlace &o = *objectsToPlace[i];
    switch (o.typeFourCc)
    {
      case _MAKE4C('FX'):
        // Do nothing.
        break;

      default: debug("cannot place unknown object type: %c%c%c%c", _DUMP4C(o.typeFourCc));
    }
    objectsToPlace[i]->destroy();
  }
  clear_and_shrink(objectsToPlace);
}


bool AcesScene::tracerayNormalizedHeightmap(const Point3 &p0, const Point3 &dir, float &t, Point3 *norm)
{
  if (!lmeshMgr)
    return false;

  return lmeshMgr->traceray(p0, dir, t, norm);
}


bool AcesScene::tracerayNormalizedStaticScene(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm)
{
  if (!frtDump.isDataValid())
    return false;
  int pmid = -1, &dest_pmid = out_pmid ? *out_pmid : pmid;
  return (out_norm ? frtDump.tracerayNormalized(p, dir, t, dest_pmid, *out_norm) : frtDump.tracerayNormalized(p, dir, t, dest_pmid)) >=
         0;
}

bool AcesScene::getHeightmapAtPoint(float x, float z, float &out_height, Point3 *out_normal)
{
  bool res = false;

  if (lmeshMgr)
  {
    float xk = 1, zk = 1;

    if (lmeshMgr->getHeight(Point2(x, z), out_height, out_normal))
    {
      if (out_normal)
      {
        out_normal->x *= xk;
        out_normal->z *= zk;
      }
      return true;
    }
    else
    {
      out_height = 0.f;
      if (out_normal)
        *out_normal = Point3(0.f, 1.f, 0.f);
      return false;
    }
  }

  if (!res)
  {
    out_height = 0.f;
    if (out_normal)
      *out_normal = Point3(0.f, 1.f, 0.f);
  }

  return res;
}

bool __stdcall AcesScene::custom_get_height(Point3 &p, Point3 *n)
{
  if (!self->getHeightmapAtPoint(p.x, p.z, p.y, n))
    return false;

  if (external_traceRay_for_rigen)
  {
    const float aboveHt = 50;
    float t = aboveHt;
    if (external_traceRay_for_rigen(p + Point3(0, t, 0), Point3(0, -1, 0), t, NULL))
      p.y += aboveHt - t;
  }
  return true;
}
vec3f __stdcall AcesScene::custom_update_pregen_pos_y(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy)
{
  if (!external_traceRay_for_rigen)
    return pos;

  Point3_vec4 p;
  v_st(&p.x, pos);

  const float aboveHt = 50;
  float t = aboveHt;
  if (!external_traceRay_for_rigen(p + Point3(0, t, 0), Point3(0, -1, 0), t, NULL))
    return pos;

  p.y += aboveHt - t;
  *dest_packed_y = short(clamp((p.y - oy) / csz_y, -1.0f, 1.0f) * 32767.0);
  return v_ld(&p.x);
}
