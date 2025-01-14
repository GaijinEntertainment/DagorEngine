// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "level.h"
#include <streaming/dag_streamingBase.h>
#include <shaders/dag_renderScene.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <gamePhys/collision/collisionLib.h>
#include <physMap/physMapLoad.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_roNameMap.h>
#include <util/dag_convar.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_collisionResourceClassId.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/lmeshMirroring.h>
#include <heightmap/heightmapHandler.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/utility/ecsBlkUtils.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <daECS/core/updateStage.h>
#include <ecs/rendInst/riExtra.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_delayedAction.h>
#include "game/gameEvents.h"
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_lock.h>
#include <EASTL/unique_ptr.h>
#include <webui/editVarPlugin.h>
#include <scene/dag_objsToPlace.h>
#include <math/random/dag_random.h>
#include "gameObjects.h"
#include <ecs/game/zones/levelRegions.h>
#include "main/levelRoads.h"
#include "main/main.h"
#include "game/riDestr.h"
#include "net/dedicated.h"

#if _TARGET_C3

#endif

#include <render/renderSettings.h>
#include <render/cables.h>
#include <3d/dag_gpuConfig.h>
#include <levelSplines/levelSplines.h>
#include <levelSplines/splineRegions.h>
#include <levelSplines/splineRoads.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/rendinstFloating.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>

#include "render/rendererFeatures.h"
#include "render/renderer.h"
#include <render/renderEvent.h>
#include "sound/dngSound.h"
#include "skies.h"
#include "render/fx/fx.h"
#include "net/net.h"
#include "main/ecsUtils.h"
#include "main/appProfile.h"
#include "main/gameLoad.h"
#include "render/animatedSplashScreen.h"
#include <main/weatherPreset.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_strUtil.h>
#include <util/dag_console.h>
#include <util/dag_delayedAction.h>
#include <fftWater/fftWater.h>
#include <daECS/core/sharedComponent.h>
#include "game/dasEvents.h"
#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
static eastl::unique_ptr<crashlytics::AppState> androidLevelStatus;
#endif

#define LEVEL_BIN_NAME "levelBin"
extern const char *const EMPTY_LEVEL_NAME = "__empty__";

ECS_REGISTER_EVENT(EventLevelLoaded)
ECS_REGISTER_EVENT(EventGameObjectsCreated)
ECS_REGISTER_EVENT(EventGameObjectsEntitiesScheduled)
ECS_REGISTER_EVENT(EventGameObjectsOptimize)
ECS_REGISTER_EVENT(EventRIGenExtraRequested)

static enum LevelStatus { LEVEL_NOT_LOADED, LEVEL_LOADING, LEVEL_LOADED, LEVEL_EMPTY } level_status = LEVEL_NOT_LOADED;
static ecs::EntityId level_eid = ecs::INVALID_ENTITY_ID;

static void add_load_level_action(class StrmSceneHolder *scn, const char *bin_name);

template <typename Callable>
static void level_is_loading_ecs_query(Callable c);

extern void wait_start_fx_job_done(bool finish_async_update = false);
static void ecs_tick()
{
  wait_start_fx_job_done(true); // because tick() can create/destroy rain/snow fx
  g_entity_mgr->tick();
}

static void set_level_status(LevelStatus status)
{
  level_status = status;

#if _TARGET_C3

#endif

  level_is_loading_ecs_query([status](bool &level_is_loading) { level_is_loading = (status == LEVEL_LOADING); });

#if _TARGET_ANDROID || _TARGET_IOS
  if (status == LEVEL_LOADING)
    androidLevelStatus = eastl::make_unique<crashlytics::AppState>("level_loading");
  else if (status == LEVEL_LOADED)
    androidLevelStatus = eastl::make_unique<crashlytics::AppState>("level_loaded");
  else
    androidLevelStatus.reset();
#endif
}


static const int render_level_tags[] = {_MAKE4C('TEX'), _MAKE4C('TEX.'), _MAKE4C('DxP2'), _MAKE4C('ENVI'), _MAKE4C('SCN'),
  _MAKE4C('SCNH'), _MAKE4C('OCCL'), _MAKE4C('rivM'), _MAKE4C('Wire')};

static const int server_only_level_tags[] = {_MAKE4C('Lnav'), _MAKE4C('Lnv2'), _MAKE4C('Lnv3'), _MAKE4C('CVRS'), _MAKE4C('Lnvs')};

static void create_efx_objects(Tab<ObjectsToPlace *> &efxToPlace)
{
  debug("placeResObjects: eFX (%d)", efxToPlace.size());
  for (ObjectsToPlace *objs : efxToPlace)
    for (int j = 0; j < objs->objs.size(); ++j)
    {
      debug("  acesfx(%s) instances=%d", objs->objs[j].resName.get(), objs->objs[j].tm.size());
      if (get_world_renderer())
      {
        int fx = acesfx::get_type_by_name(objs->objs[j].resName);
        G_ASSERTF(fx >= 0, "eFX[%d].%s inst=%d", j, objs->objs[j].resName.get(), objs->objs[j].tm.size());
        if (fx < 0)
          continue;
      }

      for (int k = 0; k < objs->objs[j].tm.size(); ++k)
      {
        ecs::ComponentsInitializer attrs;
        if (!dedicated::is_dedicated())
          ECS_INIT(attrs, effect__name, ecs::string(objs->objs[j].resName.get()));
        ECS_INIT(attrs, transform, objs->objs[j].tm[k]);
        g_entity_mgr->createEntityAsync("level_effect", eastl::move(attrs));
      }
    }
  clear_all_ptr_items(efxToPlace);
}
struct WeatherAndDateTime
{
  float timeOfDay;
  const char *weatherBlk;
  const char *weatherChoice;
  int year, month, day;
  float lat, lon;
};

#include <astronomy/astronomy.h>
static struct SceneTime
{
  int year = 1941, month = 6, day = 22;
  float timeOfDay = 6.f;
  float latitude = 55.f, longtitude = 37.f;
} scene_time;

static void load(SceneTime &s, const DataBlock &blk, float time_of_day, int year, int month, int day, float lat, float lon)
{
  s.year = blk.getInt("year", year);
  s.month = blk.getInt("month", month);
  s.day = blk.getInt("month", day);
  s.timeOfDay = blk.getReal("time", time_of_day);
  s.latitude = lat >= -90 ? lat : blk.getReal("latitude", 55.f);
  s.longtitude = lon >= -180 ? lon : blk.getReal("longtitude", 37.f);
}

Point3 calculate_server_sun_dir()
{
  float gmtTime = scene_time.timeOfDay - scene_time.longtitude * (12. / 180);
  double starsJulianDay = julian_day(scene_time.year, scene_time.month, scene_time.day, gmtTime);

  double lst = calc_lst_hours(starsJulianDay, scene_time.longtitude);
  star_catalog::StarDesc sun;
  get_sun(starsJulianDay, sun);
  Point3 sunDir;
  radec_2_xyz(sun.rightAscension, sun.declination, lst, scene_time.latitude, sunDir.x, sunDir.y, sunDir.z);
  return sunDir;
}

const char *get_rendinst_dmg_blk_fn() { return ::dgs_get_settings()->getStr("rendinstDmg", "config/rendinst_dmg.blk"); }

static DataBlock saved_level_blk;

static bool is_render_path_replacement_allowed()
{
  return ::dgs_get_settings()->getBlockByName("graphics")->getBool("allowRenderReplacementInLvl", false);
}

class StrmSceneHolder : public BaseStreamingSceneHolder
{
public:
  using BaseStreamingSceneHolder::mainBindump;

  ecs::EntityId eid;
  eastl::unique_ptr<RenderScene> rivers;
  eastl::unique_ptr<LandMeshManager> lmeshMgr;
  Tab<levelsplines::Spline> splines;
  eastl::unique_ptr<splineroads::SplineRoads> roads;
  uint32_t dataNeeded = 0;
  bool needLmeshLoadedCall = false;
  bool waterHeightmapLoaded = false;
  eastl::unique_ptr<DataBlock> levelBlk; // Warn: can be accessed only during loading (null after)
  Tab<ObjectsToPlace *> objectsToPlace, efxToPlace;
  DataBlock riGenExtraConfig;

  StrmSceneHolder(const char *blk_path, const WeatherAndDateTime &wdt, ecs::EntityId eid) : eid(eid)
  {
    rendinst::tmInst12x32bit = false;
    load(blk_path, wdt);
  }
  ~StrmSceneHolder()
  {
    // To consider: wait for LevelLoadJob's completion?
    set_level_status(LEVEL_NOT_LOADED);
    if (get_world_renderer())
      get_world_renderer()->unloadLevel();
    dacoll::destroy_static_collision();
    dacoll::set_lmesh_phys_map(nullptr);
    ridestr::shutdown();
    rendinst::clearRIGen();
    rendinst::registerRIGenExtraConfig(nullptr);
    close_cables_mgr();
    acesfx::reset();
    clean_up_level_entities();
    // for forward toggle we need to destroy WR
    if (is_render_path_replacement_allowed())
      destroy_world_renderer();
  }

  virtual bool bdlConfirmLoading(unsigned /*bindump_id*/, int tag)
  {
    return (get_world_renderer() || find_value_idx(make_span(render_level_tags, countof(render_level_tags)), tag) < 0) &&
           (is_server() || find_value_idx(make_span(server_only_level_tags, countof(server_only_level_tags)), tag) < 0);
  }
  virtual void bdlSceneLoaded(unsigned bindump_id, RenderScene *scene)
  {
    BaseStreamingSceneHolder::bdlSceneLoaded(bindump_id, scene);
    if (get_world_renderer())
      get_world_renderer()->onSceneLoaded(this);
  }
  void bdlTextureMap(unsigned /*bindump_id*/, dag::ConstSpan<TEXTUREID> texId)
  {
    for (int idx = texId.size() - 1; idx >= 0; idx--)
    {
      TEXTUREID tid = texId[idx];
      if (tid == BAD_TEXTUREID)
        continue;
      if (strstr(get_managed_texture_name(tid), "_lightmap*"))
      {
        get_world_renderer()->onLightmapSet(tid);
        break;
      }
    }
  }
  virtual bool bdlCustomLoad(unsigned /*bindump_id*/, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> texMap)
  {
    if (tag == _MAKE4C('eVER'))
    {
      static const int sha1Size = 20;
      static const int sha1StrSize = sha1Size * 2 + 1;
      char hash_str[sha1StrSize];
      uint8_t hashSHA1[sha1Size];

      int cnt = crd.readInt();
      crd.read(hashSHA1, sizeof(hashSHA1));
      debug("level SHA1=%s (%s)  fullPath=(%s)", data_to_str_hex_buf(hash_str, sha1StrSize, hashSHA1, sha1Size), crd.getTargetName(),
        df_get_real_name(crd.getTargetName()));

      for (int i = 0; i < cnt; ++i)
      {
        int curTag = crd.readInt();
        if (curTag == _MAKE4C('FRT') || curTag == _MAKE4C('RIGz') || curTag == _MAKE4C('lmap'))
        {
          crd.read(hashSHA1, sizeof(hashSHA1));
          debug(" %c%c%c%c SHA1=%s", _DUMP4C(curTag), data_to_str_hex_buf(hash_str, sha1StrSize, hashSHA1, sha1Size));
        }
        else
          crd.seekrel(sizeof(hashSHA1));
      }
      return true;
    }
    if (tag == _MAKE4C('rivM'))
    {
      if (crd.getBlockRest() >= 8)
      {
        rivers = eastl::make_unique<RenderScene>();
        rivers->loadBinary(crd, texMap, false);
      }
      return true;
    }
    if (tag == _MAKE4C('RqRL'))
    {
      {
        const char *rendinstDmgBlkFn = get_rendinst_dmg_blk_fn();
        DataBlock ri_dmg;
        if (dd_file_exists(rendinstDmgBlkFn))
          ri_dmg.load(rendinstDmgBlkFn);
        rendinst::prepareRiExtraRefs(ri_dmg);
      }

      RoNameMap *rqrl_ronm = (RoNameMap *)memalloc(crd.getBlockRest(), tmpmem);
      crd.read(rqrl_ronm, crd.getBlockRest());
      rqrl_ronm->patchData(rqrl_ronm);
      OAHashNameMap<false> requiredResources;

      for (int i = 0; i < rqrl_ronm->map.size(); i++)
      {
        requiredResources.addNameId(rqrl_ronm->map[i]);
        if (get_resource_type_id(rqrl_ronm->map[i]) == RendInstGameResClassId)
        {
          String coRes(0, "%s" RI_COLLISION_RES_SUFFIX, rqrl_ronm->map[i]);
          if (get_resource_type_id(coRes) == CollisionGameResClassId)
            requiredResources.addNameId(coRes);
        }
      }

      enable_gameres_pack_loading(true);
      set_required_res_list_restriction(requiredResources, RRL_setAndPreload);

      /*char hash_str[SHA_STR_DIGEST_LENGTH];
      for (int i = 0; i < rqrl_ronm->map.size(); i ++)
        if (get_resource_type_id(rqrl_ronm->map[i]) == rendinst::HUID_LandClassGameRes &&
            is_game_resource_loaded(GAMERES_HANDLE_FROM_STRING(rqrl_ronm->map[i]), rendinst::HUID_LandClassGameRes, false))
        {
          GameResource *res = get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(rqrl_ronm->map[i]), rendinst::HUID_LandClassGameRes);
          unsigned hashSHA1[5] = { 0, 0, 0, 0, 0 };
          if (rendinst::get_landclass_data_hash(res, hashSHA1, sizeof(hashSHA1)) == sizeof(hashSHA1))
          {
            debug(" lndC SHA1=%s (%s)", get_sha1_str_digest(hashSHA1, hash_str), rqrl_ronm->map[i]);
            levelHash = calc_crc32_continuous((const uint8_t*)hashSHA1, sizeof(hashSHA1), levelHash);
          }
          else
            G_ASSERT_LOG(0, "can't get hash from ri res '%s'", rqrl_ronm->map[i]);
          release_game_resource(res);
        }
      if (levelHash)
        levelHash = ~levelHash; // hash is complete (see calc_crc32())
      */
      memfree(rqrl_ronm, tmpmem);
      // resourcesLoaded = true;
      enable_gameres_pack_loading(false);
      return true;
    }
    if (tag == _MAKE4C('tm24'))
    {
      rendinst::tmInst12x32bit = true;
      return true;
    }
    if (tag == _MAKE4C('RIGz'))
    {
      rendinst::RIGenLoadingAutoLock riGenLd;
      const char *rendinstDmgBlkFn = get_rendinst_dmg_blk_fn();
      if (dd_file_exists(rendinstDmgBlkFn))
        riGenExtraConfig.load(rendinstDmgBlkFn);
      else
        riGenExtraConfig.reset();
      rendinst::registerRIGenExtraConfig(&riGenExtraConfig);
      if (!rendinst::loadRIGen(crd, NULL, false, nullptr, &saved_level_blk))
        return false;
      rendinst::initRiGenDebris(riGenExtraConfig, [](const char *n) { return acesfx::get_type_by_name(n); });
      if (get_world_renderer())
        rendinstfloating::init_floating_ri_res_groups(riGenExtraConfig.getBlockByName("riExtra"), true);
#if DAGOR_DBGLEVEL > 0
      Tab<rendinst::riex_handle_t> tmp;
      for (uint32_t i = 0, cnt = rendinst::getRiGenExtraResCount(); i < cnt; ++i)
      {
        int ni = rendinst::getRiGenExtraInstances(tmp, i);
        if (ni)
          logerr("Not empty (%d) ri extra '%s'(%d) before generation", ni, rendinst::getRIGenExtraName(i), i);
        G_UNUSED(ni);
      }
#endif
      rendinst::precomputeRIGenCellsAndPregenerateRIExtra();
      rendinst::setRiExtraTiledSceneWritingThread(get_main_thread_id());
      return true;
    }
    if (tag == _MAKE4C('HM2'))
    {
      G_ASSERT(lmeshMgr);
      if (lmeshMgr->loadHeightmapDump(crd, dataNeeded))
      {
        // To consider: init it on first hole addition instead?
        if (g_entity_mgr->getOr<bool>(eid, ECS_HASH("level__useGroundHolesCollision"), false))
          lmeshMgr->initHolesManager();

        dacoll::add_collision_hmap(lmeshMgr.get(), /* restitution */ 0.f, /* margin */ 0.f);
        lmeshMgr->getHmapHandler()->pushHmapModificationOnPrepare = false;
        if (get_world_renderer())
        {
          const char *fname = levelBlk->getStr(LEVEL_BIN_NAME, "undefined");
          get_world_renderer()->onLandmeshLoaded(*levelBlk, fname, lmeshMgr.get());
          LmeshMirroringParams lmeshMirror = load_lmesh_mirroring(*levelBlk);
          dacoll::set_landmesh_mirroring(lmeshMirror.numBorderCellsXPos, lmeshMirror.numBorderCellsXNeg,
            lmeshMirror.numBorderCellsZPos, lmeshMirror.numBorderCellsZNeg);
          needLmeshLoadedCall = false;
        }
        return true;
      }
      else
        return false;
    }

    const bool isLMp2 = (tag == _MAKE4C('LMp2'));
    if (tag == _MAKE4C('LMpm') || isLMp2)
    {
      debug("Loading LMpm");
      const bool hasDecals = ::dgs_get_settings()->getBool("physMapDecals", false);
      PhysMap *physMap = hasDecals ? ::load_phys_map_with_decals(crd, isLMp2) : ::load_phys_map(crd, isLMp2);
      if (physMap)
        make_grid_decals(*physMap, 32);
      dacoll::set_lmesh_phys_map(physMap);
      return true;
    }

    if (tag == _MAKE4C('WHM'))
    {
      if (FFTWater *water = dacoll::get_water())
      {
        fft_water::load_heightmap(crd, water);
        waterHeightmapLoaded = true;
      }
      else
        logerr("Water heightmap exist in level but there is no water during level load. "
               "Make sure that water entity resides before level's one in scene file");
    }

    if (tag == _MAKE4C('lmap'))
    {
      int landMeshOffset = crd.tell();
      if (landMeshOffset >= 0)
      {
        lmeshMgr = eastl::make_unique<LandMeshManager>();
        debug("[load]landmesh data");
        lmeshMgr->resetRenderDataNeeded(LC_ALL_DATA);
        if (dataNeeded & LC_GRASS_DATA)
          lmeshMgr->setGrassMaskBlk(*levelBlk->getBlockByNameEx("grass")); // fixme
        lmeshMgr->setRenderDataNeeded(dataNeeded);

        if (!lmeshMgr->loadDump(crd, midmem, dataNeeded != 0))
        {
          lmeshMgr.reset();
          debug("can't load lmesh");
        }
        needLmeshLoadedCall = true;
      }

      crd.seekrel(crd.getBlockRest());
      dacoll::add_collision_landmesh(lmeshMgr.get(), crd.getTargetName());
      return true;
    }
    if (tag == _MAKE4C('FRT'))
    {
      debug("[load]frt");
      return dacoll::load_static_collision_frt(&crd);
    }
    if (tag == _MAKE4C('Obj'))
    {
      auto objs = ObjectsToPlace::make(crd, crd.getBlockRest());
      if (objs->typeFourCc == _MAKE4C('GmO'))
        objectsToPlace.push_back(objs);
      else if (objs->typeFourCc == _MAKE4C('eFX'))
      {
        debug("placeResObjects: %c%c%c%c", _DUMP4C(objs->typeFourCc));

        if (is_server())
          efxToPlace.push_back(objs);
        else if (get_world_renderer())
        {
          for (int j = 0; j < objs->objs.size(); ++j)
          {
            int fx = acesfx::get_type_by_name(objs->objs[j].resName);
            debug("  validate: acesfx(%s)=%d instances=%d", objs->objs[j].resName.get(), fx, objs->objs[j].tm.size());
            G_ASSERTF(fx >= 0, "eFX[%d].%s inst=%d", j, objs->objs[j].resName.get(), objs->objs[j].tm.size());
          }
          objs->destroy();
        }
      }
      else
        objs->destroy();
      return true;
    }
    if (tag == _MAKE4C('hspl'))
    {
      levelsplines::load_splines(crd, splines);
      if (!roads)
      {
        roads = eastl::make_unique<splineroads::SplineRoads>();
        roads->loadRoadObjects(splines);
      }
      crd.seekrel(crd.getBlockRest());
      G_ASSERT(crd.getBlockRest() == 0);
      return true;
    }
    if (tag == _MAKE4C('splg') || tag == _MAKE4C('spgZ') || tag == _MAKE4C('spgz'))
    {
      if (!roads)
        return false;
      if (tag == _MAKE4C('spgZ'))
      {
        LzmaLoadCB lzma_crd(crd, crd.getBlockRest());
        roads->loadRoadGrid(lzma_crd);
        lzma_crd.ceaseReading();
      }
      else if (tag == _MAKE4C('spgz'))
      {
        ZstdLoadCB zstd_crd(crd, crd.getBlockRest());
        roads->loadRoadGrid(zstd_crd);
        zstd_crd.ceaseReading();
      }
      else
        roads->loadRoadGrid(crd);
      crd.seekrel(crd.getBlockRest());
      G_ASSERT(crd.getBlockRest() == 0);
      return true;
    }
    if (tag == _MAKE4C('stbl'))
    {
      if (!roads)
        return false;
      roads->loadIntersections(crd);
      crd.seekrel(crd.getBlockRest());
      G_ASSERT(crd.getBlockRest() == 0);
      return true;
    }
    if (tag == _MAKE4C('Wire'))
    {
      init_cables_mgr();
      execute_delayed_action_on_main_thread(make_delayed_action([]() {
        if (g_entity_mgr->getTemplateDB().getTemplateByName("cable_renderer"))
          g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("cable_renderer"));
        else
          // NOTE: the cables daNetGameLibs module has probably not been linked
          // (or vfsroms are out of date)
          logerr("This game does not support cables!");
      }),
        false, 0);

      if (Cables *cablesMgr = get_cables_mgr())
      {
        cablesMgr->loadCables(crd);
        rendinstdestr::set_on_rendinst_destroyed_cb([](rendinst::riex_handle_t, const TMatrix &tm, const BBox3 &box) {
          if (Cables *cablesMgr = get_cables_mgr())
            cablesMgr->onRIExtraDestroyed(tm, box);
        });
      }
      crd.seekrel(crd.getBlockRest());
      G_ASSERT(crd.getBlockRest() == 0);
      return true;
    }

    if (tag == _MAKE4C('WBBX'))
    {
      BBox3 worldBBox;
      crd.readExact(&worldBBox, sizeof(worldBBox));
      execute_delayed_action_on_main_thread(make_delayed_action([worldBBox]() {
        if (get_world_renderer())
          get_world_renderer()->setWorldBBox(worldBBox);
      }));
      return true;
    }

    bool tag_loaded = false;
    g_entity_mgr->broadcastEventImmediate(EventDoLoadTaggedLocationData(tag, &crd, &tag_loaded));
    if (tag_loaded) //-V547
      debug("[BIN] tag %c%c%c%c loaded via EventDoLoadTaggedLocationData", _DUMP4C(tag));
    return true;
  }

  void onLoaded()
  {
    G_ASSERT(is_main_thread());
    g_entity_mgr->broadcastEvent(EventRendinstsLoaded());
    const char *lvlBinName = levelBlk->getStr(LEVEL_BIN_NAME, "undefined");
    debug("level loaded: %s", lvlBinName);
#if _TARGET_ANDROID || _TARGET_IOS
    crashlytics::setCustomKey("level", lvlBinName);
#endif
    set_level_status(LEVEL_LOADED);
#if _TARGET_C3

#endif
    if (get_world_renderer())
    {
      if (needLmeshLoadedCall)
      {
        LmeshMirroringParams lmeshMirror = load_lmesh_mirroring(*levelBlk);
        dacoll::set_landmesh_mirroring(lmeshMirror.numBorderCellsXPos, lmeshMirror.numBorderCellsXNeg, lmeshMirror.numBorderCellsZPos,
          lmeshMirror.numBorderCellsZNeg);
        get_world_renderer()->onLandmeshLoaded(*levelBlk, levelBlk->getStr(LEVEL_BIN_NAME, "undefined"), lmeshMgr.get());
      }
      if (FFTWater *water = dacoll::get_water(); waterHeightmapLoaded)
      {
        fft_water::reset_render(water);
        fft_water::reset_physics(water);
      }
      get_world_renderer()->onLevelLoaded(*levelBlk);
    }

    create_level_splines(splines);
    create_level_regions(splines, *levelBlk->getBlockByNameEx("regions"));
    if (roads)
      create_level_roads(*roads);
    create_game_objects(eastl::move(objectsToPlace));

    if (efxToPlace.size())
      create_efx_objects(efxToPlace);

    g_entity_mgr->broadcastEventImmediate(EventDoFinishLocationDataLoad());

    G_ASSERT(levelBlk);
    g_entity_mgr->broadcastEvent(EventLevelLoaded(*levelBlk.release())); // Note: freed by `level_es`

    if (!::dgs_app_active)
      flash_window();
  }

private:
  void overrideRenderingPathToCompatibility()
  {
    if (dedicated::is_dedicated())
      return;

    bool usingNonCompatibility = get_corrected_rendering_path() != "default";

    if (!usingNonCompatibility || rendering_path::get_force_default())
      return;

    d3d::GpuAutoLock gpu_al;
    const char *sh_bindump_prefix = ::dgs_get_settings()->getStr("compatShaders", "compiledShaders/compatPC/game");
    load_shaders_bindump(sh_bindump_prefix, 5.0_sm);
    rendering_path::set_force_default(true);
  }

  void restoreRenderingPath()
  {
    if (!rendering_path::get_force_default() || dedicated::is_dedicated())
      return;

    d3d::GpuAutoLock gpu_al;
    const char *sh_bindump_prefix = ::dgs_get_settings()->getStr("forwardShaders", "compiledShaders/gameAndroidForward");
    String renderPath = get_corrected_rendering_path();
    if (renderPath == "mobile_deferred")
      sh_bindump_prefix = ::dgs_get_settings()->getStr("deferredShaders", "compiledShaders/gameAndroidDeferred");
    load_shaders_bindump(sh_bindump_prefix, 5.0_sm);
    rendering_path::set_force_default(false);
  }

  void load(const char *level_fn, const WeatherAndDateTime &wdt)
  {
    G_ASSERT(!levelBlk);
    debug("loading level <%s>", level_fn);
    if (strcmp(level_fn, EMPTY_LEVEL_NAME) == 0) // Special case. Level is intentionally omitted (login screen)
    {
      set_level_status(LEVEL_EMPTY);
      return;
    }
    set_level_status(LEVEL_LOADING);
    auto blk = eastl::make_unique<DataBlock>();
    blk->load(level_fn);
    if (!blk->getStr(LEVEL_BIN_NAME, NULL))
    {
      G_ASSERTF(0, "level binary is missing in <%s>", level_fn);
      set_level_status(LEVEL_LOADED);
      return;
    }
    levelBlk = eastl::move(blk);
    saved_level_blk = DataBlock(*levelBlk);

    if (is_render_path_replacement_allowed())
    {
      if (!levelBlk->getBool("overrideForwardWithCompatibility", false))
        restoreRenderingPath();
      else
        overrideRenderingPathToCompatibility();
    }

    prepare_ri_united_vdata_setup(levelBlk.get());

    create_world_renderer();

    dataNeeded = get_world_renderer() ? LC_TRIVIAL_DATA | LC_DETAIL_DATA | LC_NORMAL_DATA | LC_GRASS_DATA : 0;

    ::load(scene_time, *levelBlk->getBlockByNameEx("skies"), wdt.timeOfDay, wdt.year, wdt.month, wdt.day, wdt.lat, wdt.lon);
    load_daskies(*levelBlk->getBlockByNameEx("skies"), scene_time.timeOfDay, wdt.weatherBlk, scene_time.year, scene_time.month,
      scene_time.day, scene_time.latitude, scene_time.longtitude);
    g_entity_mgr->broadcastEventImmediate(SkiesLoaded{});

    dngsound::reset_listener();

    if (IRenderWorld *wr = get_world_renderer())
      wr->beforeLoadLevel(*levelBlk, eid);
  }
};

struct OnLevelLoadedAction final : public DelayedAction
{
  ecs::EntityId levelEid;
  OnLevelLoadedAction(ecs::EntityId eid) : levelEid(eid) {}
  void performAction() override;
};

struct LevelLoadJob final : public cpujobs::IJob
{
private:
  StrmSceneHolder *scn;
  const char *binName; // points to string buffer within levelBlk
  ecs::EntityId levelEid;

  void waitForPipelinesSetCompilation() const
  {
    const bool isWaitForPipelinesEnabled = dgs_get_settings()->getBlockByNameEx("video")->getBool("waitForPipelinesCompilation", true);
    if (!isWaitForPipelinesEnabled)
      return;
    while (uint32_t queueLength = d3d::driver_command(Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH))
    {
      logdbg("Level loading job: waiting for pipelines compilation (queue: %d)", queueLength);
      constexpr uint32_t waitTimeMs = 500;
      sleep_msec(waitTimeMs);
    }
  }

public:
  LevelLoadJob(StrmSceneHolder *scn_, const char *bin_name) : scn(scn_), binName(bin_name), levelEid(scn->eid) {}
  void doJob() override
  {
    scn->openSingle(binName);
    if (!scn->mainBindump)
      DAG_FATAL("Can't open level binfile %s", binName);

    if (auto wr = get_world_renderer())
      wr->preloadLevelTextures();

    waitForPipelinesSetCompilation();

    // This intentionally stall loading thread to make sure that no further resources are loaded until
    // level is loaded/switched it's loading state (as various entities are implicitly depends on it)
    execute_delayed_action_on_main_thread(new OnLevelLoadedAction(levelEid));
  }
  void releaseJob() override { delete this; }
};
static void add_load_level_action(StrmSceneHolder *scn, const char *bin_name)
{
  G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), new LevelLoadJob(scn, bin_name)));
}

bool is_level_loaded() { return level_status >= LEVEL_LOADED; }
bool is_level_loaded_not_empty() { return level_status == LEVEL_LOADED; }
bool is_level_loading() { return level_status == LEVEL_LOADING; }
bool is_level_unloading() { return sceneload::unload_in_progress; }
ecs::EntityId get_current_level_eid() { return level_eid; }

enum class WeatherCreateOnClient
{
  No,
  Yes
};

enum class KeepTimeOfDay
{
  No,
  Yes
};

static WeatherAndDateTime setup_level_weather_and_datetime(ecs::EntityManager &mgr,
  ecs::EntityId eid,
  const char *preset_name = nullptr,
  WeatherCreateOnClient create_on_client = WeatherCreateOnClient::No,
  KeepTimeOfDay keep_time_of_day = KeepTimeOfDay::No,
  int skies_seed = -1)
{
  Point2 timeRange = mgr.get<Point2>(eid, ECS_HASH("level__timeRange"));
  const ecs::Array *timeVec = mgr.getNullable<ecs::Array>(eid, ECS_HASH("level__timeVec"));
  int timeSeed = mgr.get<int>(eid, ECS_HASH("level__timeSeed")); // seed to have reproducable time of day
  int day = mgr.getOr(eid, ECS_HASH("level__day"), 21);
  int month = mgr.getOr(eid, ECS_HASH("level__month"), 6);
  int year = mgr.getOr(eid, ECS_HASH("level__year"), 1941);

  if (timeSeed == -1)
  {
    timeSeed = grnd();
    mgr.set(eid, ECS_HASH("level__timeSeed"), timeSeed);
    logdbg("Generated time seed: %d", timeSeed);
  }

  const ecs::Object::value_type *preset = nullptr;
  if (preset_name)
    preset = get_preset_by_name(mgr, eid, preset_name);
  else
    preset = get_preset_by_seed(mgr, eid);

  if (preset_name && !preset)
  {
    logerr("Weather preset <%s> not found in level blk, selecting preset based on seed", preset_name);
    preset = get_preset_by_seed(mgr, eid);
  }

  const char *weatherBlk = mgr.getOr(eid, ECS_HASH("level__weather"), "");
  const char *weatherSelectedChoice = "";

  bool cloudsHoleEnabled = true;
  if (preset)
  {
    bool hasWeatherData = !preset->second.is<float>();
    const char *presetName = preset->first.c_str();

    // Remapping is to not break user missions that use the blk version still.
    const ecs::SharedComponent<ecs::Object> *weatherRemaps =
      mgr.getNullable<ecs::SharedComponent<ecs::Object>>(eid, ECS_HASH("level__weatherRemaps"));
    if (weatherRemaps)
    {
      for (auto &remapPair : *weatherRemaps->get())
      {
        if (strcmp(presetName, remapPair.first.c_str()) == 0)
        {
          const char *newPresetName = remapPair.second.get<ecs::string>().c_str();
          debug("Remapping weather preset \"%s\" to \"%s\"", presetName, newPresetName);
          presetName = newPresetName;
          break;
        }
      }
    }

    if (strstr(presetName, ".blk"))
    {
      weatherBlk = presetName;
    }
    else
    {
      weatherBlk = "";

      // create entity from template
      if (is_server() || create_on_client == WeatherCreateOnClient::Yes)
      {
        ecs::ComponentsInitializer init;
        if (hasWeatherData)
        {
          const ecs::Object *comps = preset->second.get<ecs::Object>().getNullable<ecs::Object>(ECS_HASH("weatherTemplateComponents"));
          if (comps)
            for (const auto &kv : *comps)
              init[ECS_HASH_SLOW(kv.first.c_str())] = kv.second;
        }

        if (skies_seed >= 0)
          ECS_INIT(init, skies_settings__weatherSeed, skies_seed);

        mgr.createEntityAsync(String(0, "%s+weather_choice_created", presetName), eastl::move(init));
      }
    }

    weatherSelectedChoice = presetName;

    if (hasWeatherData)
    {
      const ecs::Array *timeVecPreset = preset->second.get<ecs::Object>().getNullable<ecs::Array>(ECS_HASH("level__timeVec"));
      if (timeVecPreset && !timeVecPreset->empty())
        timeVec = timeVecPreset;

      cloudsHoleEnabled = preset->second.get<ecs::Object>().getMemberOr<bool>(ECS_HASH("cloudsHoleEnabled"), true);

      // Do not create weather effects on the client
      // Effects are replicated from the server
      if (is_server() || create_on_client == WeatherCreateOnClient::Yes)
      {
        const ecs::Array *effectsArray = preset->second.get<ecs::Object>().getNullable<ecs::Array>(ECS_HASH("entities"));
        if (effectsArray)
        {
          for (auto &effect : *effectsArray)
          {
            create_weather_choice_entity(mgr, effect, eid);
          }
        }
      }
    }
  }

  float timeOfDay = mgr.get<float>(eid, ECS_HASH("level__timeOfDay"));
  if (keep_time_of_day == KeepTimeOfDay::No)
  {
    if (timeVec && !timeVec->empty())
    {
      for (auto &elem : *timeVec)
      {
        float t = elem.get<float>();
        if (t < 0.f || t > 24.f)
          logerr("Invalid value in level.timeVec: %.3f not in [0; 24]", t);
      }
      int randIdx = _rnd(timeSeed) % timeVec->size();
      timeOfDay = (*timeVec)[randIdx].get<float>();
    }
    else
      timeOfDay = _rnd_float(timeSeed, timeRange.x, timeRange.y);
  }

  if (timeOfDay == 24.f)
    timeOfDay = 0.f;
  mgr.set(eid, ECS_HASH("level__timeOfDay"), timeOfDay);
  mgr.set(eid, ECS_HASH("level__cloudsHoleEnabled"), cloudsHoleEnabled);
  mgr.set(eid, ECS_HASH("level__weather"), weatherSelectedChoice);

  float lat = mgr.getOr<float>(eid, ECS_HASH("level__latitude"), -200.f);
  float lon = mgr.getOr<float>(eid, ECS_HASH("level__longtitude"), -200.f);
  return WeatherAndDateTime{timeOfDay, weatherBlk, weatherSelectedChoice, year, month, day, lat, lon};
}

struct LocationHolder
{
  friend struct OnLevelLoadedAction;
  EA_NON_COPYABLE(LocationHolder)

  LocationHolder(ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    if (level_status == LEVEL_LOADING)
    {
      // just noop for now (to avoid complex logic of thread canceling or waiting for end of async load)
      logwarn("level is already loading");
      return;
    }

    level_eid = eid;

    G_ASSERTF(!level, "Attempt to load level without unloading old one first?!");
    g_entity_mgr->broadcastEventImmediate(EventBeforeLocationEntityCreated());

    const char *blkPath = mgr.getOr(eid, ECS_HASH("level__blk"), "");
    sceneload::set_scene_blk_path(blkPath);

    int fpsLimit = mgr.getOr(eid, ECS_HASH("level__fpsLimit"), -1);
    set_corrected_fps_limit(fpsLimit);

    WeatherAndDateTime wdt = setup_level_weather_and_datetime(mgr, eid);

    G_ASSERT_RETURN(*blkPath, );
    unload();
    level.reset(new StrmSceneHolder(blkPath, wdt, eid));
    if (level_status != LEVEL_EMPTY)
    {
      add_load_level_action(level.get(), level->levelBlk->getStr(LEVEL_BIN_NAME, "undefined"));

      if (!sceneload::is_load_in_progress())
        stop_animated_splash_screen_in_thread();
    }
  }

  ~LocationHolder()
  {
    if (ecs::EntityId goeid = g_entity_mgr->getSingletonEntity(ECS_HASH("game_objects")))
      g_entity_mgr->destroyEntity(goeid);
    unload();
    g_entity_mgr->broadcastEventImmediate(EventAfterLocationEntityDestroyed());
  }

  const LandMeshManager *getLandMeshManager() const { return level ? level->lmeshMgr.get() : nullptr; }

  RenderScene *getRivers() const { return level ? level->rivers.get() : nullptr; }

private:
  void unload()
  {
    if (!level)
      return;
    level.reset();
    delayed_binary_dumps_unload(); // without this call BinLevel won't be actually freed.
                                   // TODO: remove dependency on BaseStreamingSceneHolder and this crap
  }

  eastl::unique_ptr<StrmSceneHolder> level;
};

ECS_DECLARE_RELOCATABLE_TYPE(LocationHolder);
ECS_REGISTER_RELOCATABLE_TYPE(LocationHolder, nullptr);

ECS_AUTO_REGISTER_COMPONENT(ecs::string, "level__weather", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::EidList, "level__navAreas", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(LocationHolder, "level", nullptr, 0, "level__weather", "?level__navAreas");

void OnLevelLoadedAction::performAction()
{
  if (auto scn = g_entity_mgr->getNullable<LocationHolder>(levelEid, ECS_HASH("level")))
    scn->level->onLoaded();
  // else assume edge case of level entity already destroyed (unload/shutdown code path?)
}

ECS_REQUIRE(LocationHolder level)
static inline void level_es(const EventLevelLoaded &evt, bool *level__loaded)
{
  if (level__loaded)
    *level__loaded = true;
  add_delayed_callback_buffered([](void *b) { delete (DataBlock *)b; }, (void *)&evt.get<0>());
}

ECS_REQUIRE(ecs::Tag world_renderer_tag)
ECS_ON_EVENT(on_appear)
static inline void world_renderer_es_event_handler(const ecs::Event &) { create_world_renderer(); }

template <typename Callable>
static void get_level_ecs_query(Callable c);

const LandMeshManager *get_landmesh_manager()
{
  const LandMeshManager *mgr = nullptr;
  get_level_ecs_query([&](const LocationHolder &level) { mgr = level.getLandMeshManager(); });
  return mgr;
}

RenderScene *get_rivers()
{
  RenderScene *rivers = nullptr;
  get_level_ecs_query([&](const LocationHolder &level) { rivers = level.getRivers(); });
  return rivers;
}

template <typename Callable>
static void delete_weather_choice_entities_ecs_query(Callable c);

template <typename Callable>
static void query_level_entity_eid_set_seed_ecs_query(Callable c);


template <typename Callable>
static void query_get_level_seeds_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void query_get_level_weather_and_time_seeds_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void query_get_skies_seed_ecs_query(Callable c);

template <typename Callable>
static void query_weather_entity_for_export_ecs_query(Callable c);


static void delete_weather_choice_entities()
{
  delete_weather_choice_entities_ecs_query(
    [&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag weather_choice_tag)) { g_entity_mgr->destroyEntity(eid); });
}

void save_weather_settings_to_screenshot(DataBlock &blk)
{
  const ecs::EntityId levelEid = get_current_level_eid();
  int weatherSeed = 0;
  int timeSeed = 0;
  int skiesSeed = 0;

  query_get_level_weather_and_time_seeds_ecs_query(levelEid, [&](const int level__weatherSeed, const int level__timeSeed) {
    weatherSeed = level__weatherSeed;
    timeSeed = level__timeSeed;
  });

  query_get_skies_seed_ecs_query([&](const int skies_settings__weatherSeed) { skiesSeed = skies_settings__weatherSeed; });

  blk.setInt("weather_seed", weatherSeed);
  blk.setInt("skies_seed", skiesSeed);
  blk.setInt("time_seed", timeSeed);
  if (get_daskies() != nullptr)
  {
    const DPoint2 cloudsOrigin = get_clouds_origin();
    const DPoint2 strataCloudsOrigin = get_strata_clouds_origin();

    blk.setPoint2("clouds_origin", point2(cloudsOrigin));
    blk.setPoint2("strata_clouds_origin", point2(strataCloudsOrigin));
  }
}

void select_weather_preset(const char *preset_name)
{
  ecs::EntityId levelEid = get_current_level_eid();
  const char *currentPresetName = (g_entity_mgr->get<ecs::string>(levelEid, ECS_HASH("level__weather"))).c_str();
  if (currentPresetName && preset_name && strcmp(currentPresetName, preset_name) == 0)
    return;

  delete_weather_choice_entities();
  ecs_tick();
  // Always create Weather preset on client in cinematic mode
  WeatherAndDateTime wdt =
    setup_level_weather_and_datetime(*g_entity_mgr, levelEid, preset_name, WeatherCreateOnClient::Yes, KeepTimeOfDay::Yes);

  ::load(scene_time, *saved_level_blk.getBlockByNameEx("skies"), wdt.timeOfDay, wdt.year, wdt.month, wdt.day, wdt.lat, wdt.lon);
  load_daskies(*saved_level_blk.getBlockByNameEx("skies"), scene_time.timeOfDay, wdt.weatherBlk, scene_time.year, scene_time.month,
    scene_time.day, scene_time.latitude, scene_time.longtitude);

  // TODO: this will apply clouds settings twice, once when created, once with SkiesLoaded event
  // but it's needed for cinematic mode to know whether entities exist in the template, when it gets SkiesLoaded event
  ecs_tick();
  g_entity_mgr->broadcastEventImmediate(SkiesLoaded{});
}

void reroll_weather_choice(int weather_seed, int skies_seed = -1, int time_seed = -1, const char *preset_name = nullptr)
{
  delete_weather_choice_entities();
  ecs::EntityId levelEid;
  query_level_entity_eid_set_seed_ecs_query([&](ecs::EntityId eid, int &level__weatherSeed, int &level__timeSeed) {
    levelEid = eid;
    level__weatherSeed = weather_seed;
    level__timeSeed = time_seed;
  });
  ecs_tick();
  WeatherAndDateTime wdt =
    setup_level_weather_and_datetime(*g_entity_mgr, levelEid, preset_name, WeatherCreateOnClient::No, KeepTimeOfDay::No, skies_seed);
  load_daskies(*saved_level_blk.getBlockByNameEx("skies"), wdt.timeOfDay, wdt.weatherBlk, wdt.year, wdt.month, wdt.day, wdt.lat,
    wdt.lon, skies_seed);
  g_entity_mgr->broadcastEventImmediate(SkiesLoaded{});

  console::print_d("weather choice: %s", wdt.weatherChoice);
  console::print_d("time: %f", wdt.timeOfDay);
}

static bool skies_setting_test_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("level", "reroll_weather_choice", 1, 4)
  {
    delete_weather_choice_entities();
    int weatherSeed = -1; //-1 means it'll be randomized in setup_level_weather_and_datetime
    int skiesSeed = -1;
    int timeSeed = -1;
    if (argc >= 2)
      weatherSeed = console::to_int(argv[1]);
    if (argc >= 3)
      skiesSeed = console::to_int(argv[2]);
    if (argc >= 4)
      timeSeed = console::to_int(argv[3]);
    reroll_weather_choice(weatherSeed, skiesSeed, timeSeed);
  }

  CONSOLE_CHECK_NAME("level", "select_weather_preset", 2, 2) { select_weather_preset(argv[1]); }

  CONSOLE_CHECK_NAME("level", "weather_save", 1, 3)
  {
    unsigned int slotNo = 0;
    bool exact = false;
    if (argc >= 2)
      slotNo = console::to_int(argv[1]);
    if (argc == 3)
      exact = true;
    ecs::EntityId levelEid = get_current_level_eid();
    DataBlock weatherSavedPresets;
    if (dd_file_exist("weatherSavedPresets.blk"))
      weatherSavedPresets.load("weatherSavedPresets.blk");

    DataBlock *slotBlk = weatherSavedPresets.getBlockByName(String(100, "slot%d", slotNo));

    if (!slotBlk)
      slotBlk = weatherSavedPresets.addNewBlock(String(100, "slot%d", slotNo));
    int weatherSeed = 0;
    int timeSeed = 0;
    int skiesSeed = 0;
    float timeOfDay = 0;
    String presetName;
    DPoint2 cloudsOrigin = get_clouds_origin();
    DPoint2 strataCloudsOrigin = get_strata_clouds_origin();
    query_get_level_seeds_ecs_query(levelEid,
      [&](const int level__weatherSeed, const int level__timeSeed, const float level__timeOfDay, ecs::string level__weather) {
        weatherSeed = level__weatherSeed;
        timeSeed = level__timeSeed;
        timeOfDay = level__timeOfDay;
        presetName = String(level__weather.c_str());
      });

    query_get_skies_seed_ecs_query([&](const int skies_settings__weatherSeed) { skiesSeed = skies_settings__weatherSeed; });
    slotBlk->setInt("weather_seed", weatherSeed);
    slotBlk->setInt("time_seed", timeSeed);
    slotBlk->setInt("skies_seed", skiesSeed);
    if (exact)
    {
      slotBlk->setReal("time_of_day", timeOfDay);
      slotBlk->setStr("preset_name", presetName);
    }
    slotBlk->setPoint2("clouds_origin", point2(cloudsOrigin));
    slotBlk->setPoint2("strata_clouds_origin", point2(strataCloudsOrigin));

    weatherSavedPresets.saveToTextFile("weatherSavedPresets.blk");
  }

  CONSOLE_CHECK_NAME("level", "weather_restore", 1, 2)
  {
    unsigned int slotNo = 0;
    if (argc == 2)
      slotNo = console::to_int(argv[1]);

    DataBlock weatherSavedPresets;
    if (dd_file_exist("weatherSavedPresets.blk"))
      weatherSavedPresets.load("weatherSavedPresets.blk");

    const DataBlock *slotBlk = weatherSavedPresets.getBlockByNameEx(String(100, "slot%d", slotNo));
    set_clouds_origin(dpoint2(slotBlk->getPoint2("clouds_origin", Point2(0, 0))));
    set_strata_clouds_origin(dpoint2(slotBlk->getPoint2("strata_clouds_origin", Point2(0, 0))));
    reroll_weather_choice(slotBlk->getInt("weather_seed", -1), slotBlk->getInt("skies_seed", -1), slotBlk->getInt("time_seed", -1),
      slotBlk->getStr("preset_name", nullptr));

    float timeOfDay = slotBlk->getReal("time_of_day", -1.0);
    if (timeOfDay >= 0.0f)
      set_daskies_time(timeOfDay);
  }

  CONSOLE_CHECK_NAME("level", "export_weather_gen", 1, 1)
  {
    query_weather_entity_for_export_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag weather_choice_tag)) {
      eastl::string templateName = g_entity_mgr->getEntityTemplateName(eid);
      templateName.erase(templateName.find_first_of('+'));

      const ecs::TemplateDB &db = g_entity_mgr->getTemplateDB();
      const ecs::Template *tpl = db.getTemplateByName(templateName);
      if (!tpl)
      {
        console::print_d("Template <%s> not found!", templateName);
        return;
      }

      const char *templatePath = tpl->getPath();
      if (!templatePath[0])
      {
        console::print_d("Path for template <%s> not found!", templateName);
        return;
      }

      DataBlock originalBlk;
      originalBlk.load(templatePath);

      DataBlock *templateBlk = originalBlk.getBlockByName(templateName.c_str());

      for (auto iter = g_entity_mgr->getComponentsIterator(eid); iter; ++iter)
      {
        const auto &comp = *iter;
        const char *name = comp.first;

        // weather seed is randomized, so we don't want to save changes to it
        if (!strcmp(name, "skies_settings__weatherSeed"))
          continue;

        ecs::HashedConstString nameHash = ECS_HASH_SLOW(name);
        if (!tpl->hasComponent(nameHash))
          continue;

        const ecs::EntityComponentRef &entityComp = comp.second;
        const ecs::ChildComponent &templateComp = tpl->getComponent(nameHash, db.data());

        if (!(templateComp == entityComp))
        {
          templateBlk->removeParam(name);
          eastl::string objectName(eastl::string::CtorSprintf{}, "%s:object", name);
          templateBlk->removeBlock(objectName.c_str());
          eastl::string arrayName(eastl::string::CtorSprintf{}, "%s:array", name);
          templateBlk->removeBlock(arrayName.c_str());
          component_to_blk_param(name, entityComp, templateBlk);
        }
      }

      eastl::string outPath;
      outPath.append("../prog/gameBase/");
      outPath.append(templatePath);

      if (!originalBlk.saveToTextFile(outPath.c_str()))
      {
        console::print_d("Failed to write to file %s!", outPath);
        return;
      }

      console::print_d("Weather %s exported.", templateName);
    });
  }

  return found;
}

REGISTER_CONSOLE_HANDLER(skies_setting_test_console_handler);
