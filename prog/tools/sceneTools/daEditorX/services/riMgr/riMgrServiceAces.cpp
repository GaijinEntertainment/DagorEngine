#include <de3_interface.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_drawInvalidEntities.h>
#include <de3_baseInterfaces.h>
#include <de3_lodController.h>
#include <de3_occluderGeomProvider.h>
#include <de3_occludersFromGeom.h>
#include <de3_colorRangeService.h>
#include <de3_editorEvents.h>
#include <de3_rendInstGen.h>
#include <de3_entityGatherTex.h>
#include <de3_randomSeed.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_genObjHierMask.h>
#include <libTools/util/binDumpHierBitmap.h>
#include <rendInst/rendInstGenRender.h>
#include <riGen/genObjUtil.h>
#include <gameMath/traceUtils.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <oldEditor/de_common_interface.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_grpMgr.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <hash/md5.h>
#include <math/dag_TMatrix.h>
#include <math/dag_frustum.h>
#include <pathFinder/tileRICommon.h>
#include <pathFinder/tileCacheUtil.h>
#include <util/dag_simpleString.h>
#include <util/dag_hash.h>
#include <debug/dag_debug.h>
#include <coolConsole/coolConsole.h>
#include <de3_huid.h>
#include <stdio.h>
#include <gameRes/dag_collisionResource.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <regExp/regExp.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <riGen/riRotationPalette.h>
#include <riGen/riUtil.h>
#include <recastTools/recastObstacleFlags.h>

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define GRID_ALIGNMENT 2048

#include <oldEditor/de_clipping.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <math/dag_rayIntersectSphere.h>
#include <coolConsole/coolConsole.h>
#include <de3_collisionPreview.h>
#include <rendInst/rendInstGenRtTools.h>
#include <startup/dag_globalSettings.h>
#if HAS_GAME_PHYS
#include <gamePhys/phys/physDebugDraw.h>
#include <gamePhys/collision/collisionCache.h>
#endif
#include <EditorCore/ec_camera_elem.h>
#include <de3_hmapService.h>

extern const char *daeditor3_get_appblk_fname();
namespace rendinst
{
extern int maxRiGenPerCell, maxRiExPerCell;
extern bool forceRiExtra;
} // namespace rendinst

extern bool(__stdcall *external_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm);

static unsigned char pow_22_tbl[256];

static int rendInstEntityClassId = -1;
static int collisionSubtypeMask = -1;
static int rendEntGeomMask = -1;
static int maskTypeSubtypeMask = -1, rendSubtypeSumMask = -1;
static const int DEF_COL = IColorRangeService::IDX_GRAY;
static IColorRangeService *colRangeSrv = NULL;
static int force_lod_no = -1;
static int traceMode = 0;
static bbox3f rayBoxExt;

static bool(__stdcall *ri_get_height)(Point3 &pos, Point3 *out_norm) = 0;
static bool(__stdcall *ri_trace_ray)(const Point3 &pos, const Point3 &dir, real &t, Point3 *out_norm) = 0;
static vec3f(__stdcall *ri_update_pregen_pos_y)(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy) = 0;

static void(__stdcall *custom_reinit_cb)(void *arg) = NULL;
static void *custom_reinit_arg = NULL;
static uint8_t perInstDataDwords = 0;
static bool perInstDataUseSeed = false;
static bool delayRiResInit = true; // stays true until first HUID_BeforeMainLoop event

class AcesRendInstEntity;
class AcesRendInstEntityPool;

typedef MultiEntityPool<AcesRendInstEntity, AcesRendInstEntityPool> MultiAcesRendInstEntityPool;
typedef VirtualMpEntity<MultiAcesRendInstEntityPool> VirtualAcesRendInstEntity;

static const char *DEFAULT_LEVEL_GAMENAME = "enlisted";

static class NavmeshLayers
{
  SimpleString gameName{DEFAULT_LEVEL_GAMENAME};

  int findRiPool(const char *ri_name)
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      for (size_t i = 0, size = rgl->rtData->riResName.size(); i < size; i++)
        if (!strcmp(ri_name, rgl->rtData->riResName[i]))
          return (int)i;
    return -1;
  }

  int getBiggestLevelSize()
  {
    uint32_t size = 0;
    FOR_EACH_RG_LAYER_DO (rgl)
      size = max(rgl->rtData->riResName.size(), size);
    return size;
  }

  void buildFilter(Tab<RegExp *> &filter, const DataBlock *filterBlk)
  {
    for (int i = 0; i < filterBlk->blockCount(); i++)
    {
      SimpleString filterVal(filterBlk->getBlock(i)->getBlockName());
      RegExp *re = new RegExp;
      if (re->compile(filterVal.str()))
        filter.push_back(re);
      else
      {
        logerr("Wrong regExp \"%s\"", filterVal.str());
        delete re;
      }
    }
  }

  template <typename Lamda>
  void applyRegExp(const DataBlock *filterBlk, Lamda lambda)
  {
    if (!filterBlk)
      return;

    Tab<RegExp *> filter(tmpmem);
    buildFilter(filter, filterBlk);

    FOR_EACH_RG_LAYER_DO (rgl)
      for (size_t i = 0, size = rgl->rtData->riResName.size(); i < size; i++)
        for (RegExp *re : filter)
          if (re->test(rgl->rtData->riResName[i]))
            lambda((int)i);

    for (RegExp *re : filter)
      delete re;
  }

  String makeFilePathForNavMeshKind(const char *base_file_path, const char *nav_mesh_kind)
  {
    String res(base_file_path);
    if (!nav_mesh_kind || !*nav_mesh_kind)
      return res;
    const char *extentionPos = res.find('.', nullptr, /*forward*/ false);
    if (!extentionPos)
      return res;
    String navMeshKindPostfix(100, "_%s", nav_mesh_kind);
    res.insert(extentionPos - res.begin(), navMeshKindPostfix);
    return res;
  }

public:
  Bitarray pools;
  ska::flat_hash_set<int> transparentPools;
  ska::flat_hash_map<int, uint32_t> obstaclePools;
  ska::flat_hash_map<int, uint32_t> materialPools;
  ska::flat_hash_map<int, float> navMeshOffsetPools;
  ska::flat_hash_map<uint32_t, uint32_t> obstacleFlags;

  NavmeshLayers() {}

  void setGameName(const char *game_name)
  {
    if (gameName == game_name)
      return;
    gameName = game_name;
    pools.reset();
    obstaclePools.clear();
  };

  void load(const char *nav_mesh_kind = nullptr)
  {
    DataBlock appblk(daeditor3_get_appblk_fname());
    const DataBlock *gameBlk = appblk.getBlockByNameEx("game");
    if (!gameBlk)
      return;

    String navmesh_layers(gameBlk->getStr("navmesh_layers", nullptr));
    if (navmesh_layers.empty())
      return;
    navmesh_layers = makeFilePathForNavMeshKind(navmesh_layers, nav_mesh_kind);

    pools.resize(getBiggestLevelSize());
    pools.reset();

    obstaclePools.clear();

    DataBlock navmblk(String(260, "%s/%s", DAGORED2->getWorkspace().getAppDir(), navmesh_layers));

    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter")), [&](int pool) { pools.set(pool); });
    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter_exclude")), [&](int pool) { pools.reset(pool); });
    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter_include")), [&](int pool) { pools.set(pool); });

    for (int blkIt = 0; blkIt < navmblk.blockCount(); blkIt++)
    {
      const DataBlock *blk = navmblk.getBlock(blkIt);
      if (blk->getBool("ignoreCollision", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = findRiPool(blk->getBlock(i)->getBlockName());
          if (pool > -1)
            pools.set(pool);
        }
      }
      else
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = findRiPool(blk->getBlock(i)->getBlockName());
          if (pool > -1)
            pools.reset(pool);
        }
      }

      if (blk->getBool("ignoreTracing", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = findRiPool(blk->getBlock(i)->getBlockName());
          if (pool > -1 && transparentPools.count(pool) == 0)
            transparentPools.emplace(pool);
        }
      }
    }

    String rendinstDmgBlk(gameBlk->getStr("rendinst_dmg", ""));
    rendinstDmgBlk.replaceAll("<gameName>", gameName.c_str());
    if (rendinstDmgBlk.empty())
    {
      logdbg("rendinst_dmg is not specified, tilecached navmesh will be built without obstacles");
      return;
    }
    rendinstDmgBlk = makeFilePathForNavMeshKind(rendinstDmgBlk, nav_mesh_kind);
    DataBlock dmgblk(String(260, "%s/%s", DAGORED2->getWorkspace().getAppDir(), rendinstDmgBlk.c_str()));
    const DataBlock *riExtraBlk = dmgblk.getBlockByName("riExtra");
    if (!riExtraBlk)
    {
      logwarn("%s not found or riExtra block is missing inside it, tilecached navmesh will be built without obstacles",
        rendinstDmgBlk.c_str());
      return;
    }

    debug("will be using %s for building tilecached navmesh obstacles", rendinstDmgBlk.c_str());

    ska::flat_hash_set<uint32_t> obstacleResHashes;
    ska::flat_hash_set<uint32_t> materialResHashes;
    const DataBlock *obstacleSettingsBlk = dmgblk.getBlockByName("obstacleSettings");
    bool isObstacleByDefault = obstacleSettingsBlk ? obstacleSettingsBlk->getBool("destructibleRiIsObstacleByDefault", true) : true;
    bool nonDestructiblesCanBeObstacles =
      obstacleSettingsBlk ? obstacleSettingsBlk->getBool("nonDestructiblesCanBeObstacles", false) : false;

    for (int blkIt = 0; blkIt < riExtraBlk->blockCount(); blkIt++)
    {
      const DataBlock *blk = riExtraBlk->getBlock(blkIt);
      int pool = findRiPool(blk->getBlockName());
      if (pool <= -1)
        continue;
      // All riExtras that have hp or destructionImpulse are considered as temporary obstacles,
      // additionally you can specify isObstacle=false to not make it an obstacle. One use case for this
      // are RIs that have very large bounding boxes and you don't want the bots to avoid them. Another use case
      // are RIs that looks like they're 80% or so destroyed and a bot can walk through even when RI isn't totally destroyed.
      bool canBeObstacle =
        nonDestructiblesCanBeObstacles || blk->getReal("hp", 0.0f) > 0.0f || blk->getReal("destructionImpulse", 0.0f) > 0.0f;
      if (canBeObstacle && blk->getBool("isObstacle", isObstacleByDefault))
        if (obstaclePools.count(pool) == 0)
        {
          uint32_t flags = ObstacleFlags::NONE;
          uint32_t hash = str_hash_fnv1(blk->getBlockName());
          if (!obstacleResHashes.emplace(hash).second)
            logerr("Obstacle resource hash collision for %s, expect missing/extra obstacles in level!", blk->getBlockName());
          obstaclePools.emplace(pool, hash);
          if (blk->getBool("crossWithJumpLinks", false))
            flags = flags | ObstacleFlags::CROSS_WITH_JL;
          if (blk->getBool("disableJumplinksAround", false))
            flags = flags | ObstacleFlags::DISABLE_JL_AROUND;
          if (flags != ObstacleFlags::NONE)
            obstacleFlags.emplace(hash, flags);
        }
      const char *material = blk->getStr("material", "");
      if (strcmp(material, "barbwire") == 0)
        if (materialPools.count(pool) == 0)
        {
          uint32_t hash = str_hash_fnv1(blk->getBlockName());
          if (!materialResHashes.emplace(hash).second)
            logerr("Material resource hash collision for %s, expect missing/extra materials in level!", blk->getBlockName());
          materialPools.emplace(pool, hash);
        }
    }

    String navmesh_obstacles(gameBlk->getStr("navmesh_obstacles", nullptr));
    if (navmesh_obstacles.empty())
      return;
    navmesh_obstacles = makeFilePathForNavMeshKind(navmesh_obstacles, nav_mesh_kind);

    DataBlock navObstaclesBlk;
    String navObstaclesPath(260, "%s/%s", DAGORED2->getWorkspace().getAppDir(), navmesh_obstacles);

    if (dblk::load(navObstaclesBlk, navObstaclesPath.c_str(), dblk::ReadFlag::ROBUST))
    {
      for (int blkIt = 0; blkIt < navObstaclesBlk.blockCount(); blkIt++)
      {
        const DataBlock *blk = navObstaclesBlk.getBlock(blkIt);
        int pool = findRiPool(blk->getBlockName());
        if (pool <= -1)
          continue;

        const float navMeshBoxOffset = blk->getReal("navMeshBoxOffset", 0.0f);
        if (navMeshBoxOffset > 0.0f)
        {
          if (navMeshOffsetPools.count(pool) == 0)
            navMeshOffsetPools.emplace(pool, navMeshBoxOffset);
        }
      }
    }
  }
} navmeshLayers;

struct RendinstVertexDataCbEditor : public rendinst::RendinstVertexDataCbBase
{
  Tab<IPoint2> &transparent;
  Tab<NavMeshObstacle> &obstacles;

  RendinstVertexDataCbEditor(Tab<Point3> &verts, Tab<int> &inds, Tab<IPoint2> &transparent, Bitarray &pools,
    ska::flat_hash_map<int, uint32_t> &obstaclePools, ska::flat_hash_map<int, uint32_t> &materialPools,
    ska::flat_hash_map<int, float> &navMeshOffsetPools, Tab<NavMeshObstacle> &obstacles) :
    RendinstVertexDataCbBase(verts, inds, pools, obstaclePools, materialPools, navMeshOffsetPools),
    transparent(transparent),
    obstacles(obstacles)
  {}
  ~RendinstVertexDataCbEditor() { clear_all_ptr_items(riCache); }

  virtual void processCollision(const rendinst::CollisionInfo &coll_info) override
  {
    if (!coll_info.collRes)
      return;
    RiData *data = NULL;
    for (int i = 0; i < riCache.size(); ++i)
    {
      if (*riCache[i] == coll_info)
      {
        data = riCache[i];
        break;
      }
    }

    if (!data)
    {
      // found new one, add it.
      RiData *rdata = new RiData;
      riCache.push_back(rdata);
      rdata->build(coll_info);
      data = rdata;
    }

    if (!data)
      return;

    mat44f instTm;
    v_mat44_make_from_43cu_unsafe(instTm, coll_info.tm.array);
    const int indBase = indices.size();
    int idxBase = vertices.size();

    auto materialIt = navmeshLayers.materialPools.find(coll_info.desc.pool);
    if (materialIt == navmeshLayers.materialPools.end())
    {
      Point3_vec4 tmpVert;
      for (int i = 0; i < data->vertices.size(); ++i)
      {
        v_st(&tmpVert.x, v_mat44_mul_vec3p(instTm, data->vertices[i]));
        vertices.push_back(tmpVert);
      }
      for (int i = 0; i < data->indices.size(); ++i)
        indices.push_back(data->indices[i] + idxBase);

      if (navmeshLayers.transparentPools.find(coll_info.desc.pool) != navmeshLayers.transparentPools.end())
        transparent.push_back({indBase, (int)data->indices.size()});
    }

    auto obstacleIt = navmeshLayers.obstaclePools.find(coll_info.desc.pool);
    if (obstacleIt != navmeshLayers.obstaclePools.end())
    {
      Point3_vec4 p1, p2;
      v_st(&p1, coll_info.collRes->vFullBBox.bmin);
      v_st(&p2, coll_info.collRes->vFullBBox.bmax);
      Point3 c, ext;
      float angY;
      BBox3 oobb(p1, p2);
      // Unfortuantely we don't know agent height and cell size at this point, so we pass zeros
      // and add them later while building the navmesh in finalize_navmesh_tilecached_tile.
      Point2 obstaclePadding = ZERO<Point2>();
      if (materialIt != navmeshLayers.materialPools.end())
        idxBase = -1;

      auto navMeshBoxOffsetIt = navmeshLayers.navMeshOffsetPools.find(coll_info.desc.pool);
      if (navMeshBoxOffsetIt != navmeshLayers.navMeshOffsetPools.end())
        obstaclePadding.x = navMeshBoxOffsetIt->second;

      pathfinder::tilecache_calc_obstacle_pos(coll_info.tm, oobb, 0.0f, obstaclePadding, c, ext, angY);

      obstacles.push_back(
        NavMeshObstacle(obstacleIt->second, BBox3(c - ext, c + ext), angY, coll_info.tm * oobb, idxBase, data->vertices.size()));
    }
  }
};

static WinCritSec ri_gather_cc;

class RendinstGatherCollJob : public cpujobs::IJob
{
  Tab<IPoint2> &outTransparent;
  Tab<NavMeshObstacle> &outObstacles;
  Tab<Point3> &outVert;
  Tab<int> &outInd;
  BBox3 box;

public:
  RendinstGatherCollJob(const BBox3 &in_box, Tab<Point3> &out_vert, Tab<int> &out_ind, Tab<IPoint2> &out_transparent,
    Tab<NavMeshObstacle> &out_obstacles) :
    box(in_box), outVert(out_vert), outInd(out_ind), outTransparent(out_transparent), outObstacles(out_obstacles)
  {}

  virtual void doJob()
  {
    int64_t refproc = ref_time_ticks_qpc();
    Tab<NavMeshObstacle> obstacles;
    Tab<IPoint2> transparent;
    Tab<Point3> vertices;
    Tab<int> indices;

    RendinstVertexDataCbEditor cb(vertices, indices, transparent, navmeshLayers.pools, navmeshLayers.obstaclePools,
      navmeshLayers.materialPools, navmeshLayers.navMeshOffsetPools, obstacles);
    rendinst::testObjToRIGenIntersection(box, TMatrix::IDENT, cb, rendinst::GatherRiTypeFlag::RiGenAndExtra);
    transparent.reserve(cb.transparent.size());
    vertices.reserve(cb.vertNum);
    indices.reserve(cb.indNum);
    cb.procAllCollision();
    debug("RendinstGatherCollJob proc done in %.2f sec", get_time_usec_qpc(refproc) / 1000000.0);

    if ((vertices.empty() || indices.empty()) && obstacles.empty())
      return;

    if (vertices.empty() || indices.empty())
    {
      clear_and_shrink(vertices);
      clear_and_shrink(indices);
    }

    int64_t refpush = ref_time_ticks_qpc();
    WinAutoLock lock(ri_gather_cc);
    const int indBase = outInd.size();
    const int idxBase = outVert.size();
    append_items(outVert, vertices.size(), vertices.data());
    // we need to do smart indices remap here :(
    outInd.reserve(outInd.size() + indices.size());
    for (int i = 0; i < indices.size(); ++i)
      outInd.push_back(indices[i] + idxBase);
    outTransparent.reserve(outTransparent.size() + transparent.size());
    for (int i = 0; i < transparent.size(); ++i)
    {
      IPoint2 tr = transparent[i];
      tr.x += indBase;
      outTransparent.push_back(tr);
    }
    outObstacles.reserve(outObstacles.size() + obstacles.size());
    for (int i = 0; i < obstacles.size(); ++i)
    {
      NavMeshObstacle ob = obstacles[i];
      if (ob.vtxStart != -1)
        ob.vtxStart += idxBase;
      outObstacles.push_back(ob);
    }
    debug("RendinstGatherCollJob push done in %.2f sec", get_time_usec_qpc(refpush) / 1000000.0);
  }
  virtual void releaseJob() { delete this; }
};


static void addPerInstData(Tab<int> &dest_per_inst_data, int per_inst_seed)
{
  if (!perInstDataDwords)
    return;
  int idx = append_items(dest_per_inst_data, perInstDataDwords);
  mem_set_0(make_span(dest_per_inst_data).subspan(idx));
  if (perInstDataUseSeed)
    dest_per_inst_data[idx] = per_inst_seed;
}
static void addPerInstData(BinDumpSaveCB &cell_cwr, int per_inst_seed)
{
  if (!perInstDataDwords)
    return;
  cell_cwr.writeInt32e(perInstDataUseSeed ? per_inst_seed : 0);
  if (perInstDataDwords > 0)
    cell_cwr.writeZeroes((perInstDataDwords - 1) * 4);
}


class AcesRendInstEntity : public VirtualAcesRendInstEntity,
                           public IColor,
                           public ILodController,
                           public IRendInstEntity,
                           public IEntityGatherTex,
                           public IRandomSeedHolder,
                           public IEntityGetRISceneLodsRes,
                           IDataBlockIdHolder
{
public:
  AcesRendInstEntity() : VirtualAcesRendInstEntity(rendInstEntityClassId), colorIdx(DEF_COL)
  {
    instSeed = 0;
    autoInstSeed = true;
    dataBlockId = IDataBlockIdHolder::invalid_id;
    tm.identity();
    tm.m[3][0] = tm.m[3][2] = 1e6;
  }

  virtual void setTm(const TMatrix &_tm);
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy();

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, ILodController);
    RETURN_INTERFACE(huid, IColor);
    RETURN_INTERFACE(huid, IRendInstEntity);
    RETURN_INTERFACE(huid, IEntityGatherTex);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    RETURN_INTERFACE(huid, IEntityGetRISceneLodsRes);
    RETURN_INTERFACE(huid, IDataBlockIdHolder);
    return NULL;
  }

  virtual void setColor(unsigned int in_color)
  {
    colorIdx = in_color;
    G_ASSERTF(colorIdx == in_color, "in_color=0x%X", in_color);
  }
  virtual unsigned int getColor() { return colorIdx; }

  // ILodController
  virtual int getLodCount();
  virtual void setCurLod(int n);
  virtual real getLodRange(int lod_n);

  // no texture quality level support
  virtual int getTexQLCount() const { return 1; }
  virtual void setTexQL(int ql) {}
  virtual int getTexQL() const { return 0; }

  // no named nodes support
  virtual int getNamedNodeCount() { return 0; }
  virtual const char *getNamedNode(int idx) { return NULL; }
  virtual int getNamedNodeIdx(const char *nm) { return -1; }
  virtual bool getNamedNodeVisibility(int idx) { return true; }
  virtual void setNamedNodeVisibility(int idx, bool vis) {}

  virtual DagorAsset *getAsset() override;
  virtual int getAssetNameId() override;
  virtual const char *getResourceName();
  virtual int getPoolIndex() { return poolIdx; }
  virtual bool getRendInstQuantizedTm(TMatrix &out_tm) const override;

  virtual void setSeed(int new_seed) {}
  virtual int getSeed() { return instSeed; }
  virtual void setPerInstanceSeed(int seed);
  virtual int getPerInstanceSeed() { return autoInstSeed ? 0 : instSeed; }
  virtual bool isSeedSetSupported() { return false; }

  // IEntityGatherTex interface
  virtual int gatherTextures(TextureIdSet &out_tex_id, int for_lod);

  // IDataBlockIdHolder
  virtual void setDataBlockId(unsigned id) { dataBlockId = id; }
  virtual unsigned getDataBlockId() { return dataBlockId; }

  virtual RenderableInstanceLodsResource *getSceneLodsRes();
  virtual const RenderableInstanceLodsResource *getSceneLodsRes() const;

public:
  static const int STEP = 2048;

  TMatrix tm;
  unsigned short colorIdx;
  unsigned short autoInstSeed : 1;
  unsigned short dataBlockId : 15;
  int instSeed;
};


class AcesRendInstEntityPool : public EntityPool<AcesRendInstEntity>, public IDagorAssetChangeNotify
{
public:
  Ptr<RenderableInstanceLodsResource> res;
  SimpleString riResName;
  int defColorIdx;
  int pregenId;
  int layerIdx;
  BBox2 precalculatedBox;


  AcesRendInstEntityPool(const DagorAsset &asset) : virtualEntity(rendInstEntityClassId), occlBox(midmem), occlQuadV(midmem)
  {
    precalculatedBox.setempty();
    pregenId = -1;
    layerIdx = 0;
    assetNameId = asset.getNameId();
    aMgr = &asset.getMgr();
    riResName = asset.getName();
    aMgr->subscribeUpdateNotify(this, assetNameId, asset.getType());

    geom = NULL;
    visualGeom = NULL;
    init0(asset);
    if (!delayRiResInit)
      init1();
  }
  void init0(const DagorAsset &asset)
  {
    const DataBlock &colBlk = *asset.props.getBlockByNameEx("colors");
    defColorIdx = colRangeSrv->addColorRange(colBlk.getE3dcolor("colorFrom", 0x80808080U), colBlk.getE3dcolor("colorTo", 0x80808080U));

    bound0rad = 1;
    bsph = BSphere3(ZERO<Point3>(), bound0rad);
    bbox = bsph;
    layerIdx = asset.props.getInt("layerIdx", asset.props.getBool("secondaryLayer", false) ? 1 : 0);

    dagFile.printf(256, "%s/%s.lod00.dag", asset.getFolderPath(), riResName.str());

    del_it(geom);
    del_it(visualGeom);

    bool read_collision = asset.props.getBool("collision", false);
    bool read_occluders = asset.props.getBool("occluders", false);
    if (!read_collision && !read_occluders)
      return;

    geom = new (midmem) GeomObject;
    if (!geom->loadFromDag(dagFile, &EDITORCORE->getConsole()))
    {
      DAEDITOR3.conError("can't load dag file %s: %s", riResName.str(), dagFile.str());
      del_it(geom);
      return;
    }

    if (read_occluders)
      ::getOcclFromGeom(*geom->getGeometryContainer(), asset.getName(), occlBox, occlQuadV);
    if (read_collision)
      parseGeometry();
    else
      del_it(geom);
  }
  void init1(bool on_asset_changed = false)
  {
    if (res)
    {
      ::release_game_resource((GameResource *)res.get());
      res = NULL;
    }

    if (!delayRiResInit)
    {
      FastNameMap resNameMap;
      resNameMap.addNameId(riResName);
      resNameMap.addNameId(String(0, "%s" RI_COLLISION_RES_SUFFIX, riResName));
      ::set_required_res_list_restriction(resNameMap);
    }
    res = (RenderableInstanceLodsResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(riResName), RendInstGameResClassId);
    if (!delayRiResInit)
      ::reset_required_res_list_restriction();

    if (!res)
    {
      DAEDITOR3.conError("resource is not rendInst: %s", (char *)riResName);
      return;
    }

    if (on_asset_changed)
      rendinst::update_rt_pregen_ri(pregenId, *res);
    else
      pregenId = rendinst::register_rt_pregen_ri(res, layerIdx, colRangeSrv->getColorFrom(defColorIdx),
        colRangeSrv->getColorTo(defColorIdx), riResName);
    if (pregenId < -1)
    {
      DAEDITOR3.conError("pregenEnt exceeds RI pool limit %d, so <%s> will not be visible!", -pregenId - 2, riResName);
      pregenId = -1;
    }

    if (res && res->lods.size())
    {
      bound0rad = res->bound0rad;
      bsph = BSphere3(res->bsphCenter, res->bsphRad);
      bbox = res->bbox;
    }
  }

  ~AcesRendInstEntityPool()
  {
    if (aMgr)
      aMgr->unsubscribeUpdateNotify(this);
    del_it(visualGeom);
    del_it(geom);
    res = NULL;
    riResName = NULL;
    if (aMgr)
      aMgr = NULL;
    assetNameId = -1;
  }

  bool checkEqual(const DagorAsset &asset) { return strcmp(riResName, asset.getName()) == 0; }

  void gatherRiP4(Tab<Point4> &dest, Tab<int> &dest_per_inst_data, float x0, float z0, float x1, float z1, const BBox2 &land_bb)
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->checkSubtypeAndLayerHiddenMasks(subtype_mask, lh_mask))
      {
        float x = ent[j]->tm[3][0]; // clamp(tm[3][0], land_bb[0][0]+1, land_bb[1][0]-1);
        float z = ent[j]->tm[3][2]; // clamp(tm[3][2], land_bb[0][1]+1, land_bb[1][1]-1);
        if (x >= x0 && x < x1 && z >= z0 && z < z1)
        {
          dest.push_back().set(ent[j]->tm[3][0], ent[j]->tm[3][1], ent[j]->tm[3][2], ent[j]->tm.getcol(1).length());
          addPerInstData(dest_per_inst_data, ent[j]->instSeed);
        }
      }
  }
  void gatherRiTM(Tab<TMatrix> &dest, Tab<int> &dest_per_inst_data, float x0, float z0, float x1, float z1, const BBox2 &land_bb)
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    int cnt = 0;
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->checkSubtypeAndLayerHiddenMasks(subtype_mask, lh_mask))
      {
        float x = ent[j]->tm[3][0]; // clamp(tm[3][0], land_bb[0][0]+1, land_bb[1][0]-1);
        float z = ent[j]->tm[3][2]; // clamp(tm[3][2], land_bb[0][1]+1, land_bb[1][1]-1);
        if (x >= x0 && x < x1 && z >= z0 && z < z1)
        {
          cnt++;
          dest.push_back(ent[j]->tm);
          addPerInstData(dest_per_inst_data, ent[j]->instSeed);
        }
      }
  }

  void buildBBox()
  {
    for (int j = 0; j < ent.size(); ++j)
    {
      if (!ent[j])
        continue;
      const TMatrix &tm = ent[j]->tm;
      precalculatedBox += Point2(tm[3][0], tm[3][2]);
    }
  }

  void renderService(int subtype_mask, uint64_t lh_mask)
  {
    if (!res.get())
      ::render_invalid_entities(ent, subtype_mask);

    if (subtype_mask & collisionSubtypeMask && collisionpreview::haveCollisionPreview(collision))
    {
      begin_draw_cached_debug_lines();

      const E3DCOLOR color(0, 255, 0);

      int entCnt = ent.size();
      for (int k = 0; k < entCnt; k++)
      {
        if (!ent[k] || (ent[k]->getSubtype() == IObjEntity::ST_NOT_COLLIDABLE) ||
            !ent[k]->checkSubtypeAndLayerHiddenMasks(subtype_mask, lh_mask))
          continue;

        TMatrix wtm = TMatrix::IDENT;
        ent[k]->getTm(wtm);

        collisionpreview::drawCollisionPreview(collision, wtm, color);
      }
      end_draw_cached_debug_lines();
    }
  }

  void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<IOccluderGeomProvider::Quad> &occl_quads)
  {
    if (occlBox.size() + occlQuadV.size() == 0)
      return;

    int bnum = occlBox.size(), qnum = occlQuadV.size() / 4;
    TMatrix tm;

    for (int j = 0; j < ent.size(); j++)
      if (ent[j])
      {
        ent[j]->getTm(tm);

        if (bnum)
        {
          int base = append_items(occl_boxes, bnum);
          for (int i = 0; i < bnum; i++)
            occl_boxes[base + i] = tm * occlBox[i];
        }
        if (qnum)
        {
          int base = append_items(occl_quads, qnum);
          for (int i = 0; i < qnum * 4; i++)
            occl_quads[base + i / 4].v[i % 4] = tm * occlQuadV[i];
        }
      }
  }
  void renderOccluders(const Point3 &camPos, float max_dist)
  {
    if (!res || (!res->getOccluderBox() && !res->getOccluderQuadPts()))
      return;

    TMatrix tm;
    float max_dist2 = max_dist * max_dist;
    const BBox3 &b = *res->getOccluderBox();
    const Point3 *q = res->getOccluderQuadPts();

    begin_draw_cached_debug_lines(false, false);
    for (int j = 0; j < ent.size(); j++)
      if (ent[j])
      {
        ent[j]->getTm(tm);
        if (lengthSq(tm.getcol(3) - camPos) > max_dist2)
          continue;
        set_cached_debug_lines_wtm(tm);
        if (q)
        {
          draw_cached_debug_line(q, 4, 0x80808000);
          draw_cached_debug_line(q[3], q[0], 0x80808000);
        }
        else
          draw_cached_debug_box(b, 0x80808080);
      }
    end_draw_cached_debug_lines();

    begin_draw_cached_debug_lines();
    for (int j = 0; j < ent.size(); j++)
      if (ent[j])
      {
        ent[j]->getTm(tm);
        if (lengthSq(tm.getcol(3) - camPos) > max_dist2)
          continue;
        set_cached_debug_lines_wtm(tm);
        if (q)
        {
          draw_cached_debug_line(q, 4, 0xFFFFFF00);
          draw_cached_debug_line(q[3], q[0], 0xFFFFFF00);
        }
        else
          draw_cached_debug_box(b, 0xFFFFFFFF);
      }
    end_draw_cached_debug_lines();
  }

  void setupVirtualEnt(MultiAcesRendInstEntityPool *p, int idx)
  {
    virtualEntity.pool = p;
    virtualEntity.poolIdx = idx;
  }
  IObjEntity *getVirtualEnt() { return &virtualEntity; }

  bool isValid() const { return res.get() != NULL; }
  const char *getResName() const { return riResName; }

  // ILodController
  inline int getLodCount() { return res ? res->lods.size() : 0; }
  inline real getLodRange(int lod_n) { return res ? res->lods[lod_n].range : 0.0; }

  // IEntityGatherTex interface
  virtual int gatherTextures(TextureIdSet &out_tex_id, int for_lod)
  {
    if (!res)
      return 0;
    int st_cnt = out_tex_id.size();
    if (for_lod < 0)
      res->gatherUsedTex(out_tex_id);
    else if (for_lod < res->lods.size())
      res->lods[for_lod].scene->gatherUsedTex(out_tex_id);
    return out_tex_id.size() - st_cnt;
  }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    if (!custom_reinit_cb)
    {
      DAEDITOR3.conWarning("ignoring removed rendinst");
      return;
    }
    del_it(geom);
    del_it(visualGeom);
    ::release_game_resource((GameResource *)res.get());
    res = NULL;
    pregenId = -1;
    custom_reinit_cb(custom_reinit_arg);
    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    if (!custom_reinit_cb)
    {
      DAEDITOR3.conWarning("ignoring changed rendinst %s", asset.getName());
      return;
    }
    DAEDITOR3.conNote("rendinst changed %s", asset.getName());
    init0(asset);
    init1(true);
    custom_reinit_cb(custom_reinit_arg);
    EDITORCORE->invalidateViewportCache();
  }


  inline void addGeometry(StaticGeometryContainer &cont, int st_mask, int mask_type)
  {
    GeomObject *gobj = (mask_type == IObjEntityFilter::STMASK_TYPE_COLLISION) ? geom : visualGeom;

    if (!gobj)
      return;

    const StaticGeometryContainer *g = gobj->getGeometryContainer();
    Tab<StaticGeometryNode *> nodes(tmpmem);

    const int nodeCnt = g->nodes.size();
    for (int i = 0; i < nodeCnt; i++)
    {
      StaticGeometryNode *node = g->nodes[i];

      if (node && ((node->flags & StaticGeometryNode::FLG_COLLIDABLE) == StaticGeometryNode::FLG_COLLIDABLE ||
                    mask_type != IObjEntityFilter::STMASK_TYPE_COLLISION))
      {
        nodes.push_back(node);
      }
    }

    const int entCnt = ent.size();
    for (int j = 0; j < entCnt; j++)
      if (ent[j] && ent[j]->checkSubtypeMask(st_mask))
      {
        const int nodesCnt = nodes.size();
        for (int i = 0; i < nodesCnt; i++)
        {
          StaticGeometryNode &node = *nodes[i];
          StaticGeometryNode *n = new (midmem) StaticGeometryNode(node);

          n->wtm = ent[j]->tm * node.wtm;
          n->flags = node.flags;
          n->visRange = node.visRange;
          n->lighting = node.lighting;

          n->name = String(256, "%s_%d_%s", assetName(), j, node.name.str());

          if (node.topLodName.length())
            n->topLodName = String(256, "%s_%d_%s", assetName(), j, node.topLodName.str());

          cont.addNode(n);
        }
      }
  }

  void gatherExportToDagGeometry(StaticGeometryContainer &container)
  {
    if (isEmpty())
      return;

    if (!visualGeom)
    {
      visualGeom = new (midmem) GeomObject;
      if (!visualGeom->getGeometryContainer()->loadFromDag(dagFile, &EDITORCORE->getConsole()))
      {
        DAEDITOR3.conError("can't load dag file %s: %s", riResName.str(), dagFile.str());
        del_it(visualGeom);
        return;
      }
    }

    addGeometry(container, IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER),
      IObjEntityFilter::STMASK_TYPE_RENDER);
  }

  inline bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
  {
    if (!geom || maxt <= 0)
      return false;

    TMatrix tm, iwtm;
    BSphere3 wbs;
    const int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    int hit_id = -1;
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && (ent[j]->getSubtype() != IObjEntity::ST_NOT_COLLIDABLE) && ent[j]->checkSubtypeMask(subtype_mask))
      {
        ent[j]->getTm(tm);
        wbs = tm * bsph;
        if (!notNormalizedRayIntersectSphere(p, dir, wbs.c, wbs.r2))
          continue;

        float tm_det = tm.det();
        if (fabsf(tm_det) < 1e-12)
          continue;
        iwtm = inverse(tm, tm_det);
        Point3 transformedP = iwtm * p;
        Point3 transformedP2 = iwtm * (p + dir * maxt);
        Point3 transformedDir = transformedP2 - transformedP;

        real t = 1;

        if (geom->traceRay(transformedP, transformedDir, t, norm))
        {
          maxt *= t;
          hit_id = j;
        }
      }

    if (hit_id >= 0)
    {
      if (norm)
      {
        ent[hit_id]->getTm(tm);
        *norm = normalize(tm % *norm);
      }
      return true;
    }
    return false;
  }

  DagorAsset *getAsset() const
  {
    if (!aMgr)
      return nullptr;
    dag::ConstSpan<DagorAsset *> assets = aMgr->getAssets();
    for (int i = 0; i < assets.size(); ++i)
      if (assets[i]->getNameId() == assetNameId && assets[i]->getType() == rendInstEntityClassId)
        return assets[i];
    return nullptr;
  }

  int getAssetNameId() const { return assetNameId; }

  inline const char *assetName() const { return aMgr ? aMgr->getAssetName(assetNameId) : NULL; }
  const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:rendInst", assetName());
    return buf;
  }
  virtual bool dumpUsedEntitesCount(int idx) override
  {
    if (int cnt = ent.size() - entUuIdx.size())
    {
      debug("  [%4d] %s: %d entities", idx, assetName(), cnt);
      return true;
    }
    debug("  [%4d] %s: ---", idx, assetName());
    return false;
  }

protected:
  inline void parseGeometry()
  {
    if (!geom)
      return;

    StaticGeometryContainer *g = geom->getGeometryContainer();

    for (int i = g->nodes.size() - 1; i >= 0; i--)
    {
      if (!g->nodes[i] || !g->nodes[i]->mesh)
        erase_items(g->nodes, i, 1);
      else if (!(g->nodes[i]->flags & StaticGeometryNode::FLG_COLLIDABLE))
        erase_items(g->nodes, i, 1);
      else if (!g->nodes[i]->mesh->mesh.face.size())
        erase_items(g->nodes, i, 1);
    }

    collisionpreview::assembleCollisionFromNodes(collision, g->nodes);

    if (!g->nodes.size())
    {
      DAEDITOR3.conWarning("no collision in asset %s", assetName());
      del_it(geom);
    }
    else
      geom->setTm(TMatrix::IDENT);
  }

protected:
  GeomObject *geom;
  GeomObject *visualGeom;
  DagorAssetMgr *aMgr;
  int assetNameId;
  String dagFile;

  collisionpreview::Collision collision;
  VirtualAcesRendInstEntity virtualEntity;
  real bound0rad;

  Tab<TMatrix> occlBox;
  Tab<Point3> occlQuadV;
};


static IRendInstGenService *rigenSrv = NULL;
static int frameBlkId = -1;
static BBox2 rigenLandBox;

class AcesRendInstEntityManagementService : public IEditorService,
                                            public IObjEntityMgr,
                                            public IBinaryDataBuilder,
                                            public IWriteAddLtinputData,
                                            public ILevelResListBuilder,
                                            public ILightingChangeClient,
                                            public IGatherStaticGeometry,
                                            public IDagorEdCustomCollider,
                                            public IOccluderGeomProvider,
                                            public IRenderingService,
                                            public IRendInstGenService,
                                            public IExportToDag,
                                            public IGatherCollision
{
public:
  AcesRendInstEntityManagementService()
  {
    rendInstEntityClassId = IDaEditor3Engine::get().getAssetTypeId("rendInst");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");

    maskTypeSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("mask_tile");
    rendSubtypeSumMask = (1 << IDaEditor3Engine::get().registerEntitySubTypeId("single_ent")) |
                         (1 << IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile")) |
                         (1 << IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls")) | maskTypeSubtypeMask;

    visible = true;
    if (DAGORED2)
      DAGORED2->registerCustomCollider(this);

    rigenSrv = NULL;
    treeTopProjectionTexId = BAD_TEXTUREID;
    landBlendNoiseTexId = BAD_TEXTUREID;

    Point3 traceCacheExp = Point3(2.0f, 2.0f, 2.0f);
    traceCacheExpands = v_ldu(&traceCacheExp.x);

    v_bbox3_init_empty(traceCacheOOBB);
    BBox3 traceCacheBox(Point3(-1, -1, -1), Point3(1, 1, 1));
    traceCacheOOBB = v_ldu_bbox3(traceCacheBox);
    lastCacheTM = TMatrix::IDENT;
  }
  void explicitInit()
  {
    DataBlock appBlk(daeditor3_get_appblk_fname());
    rigenSrv = this;
    rendinst::register_land_gameres_factory();
    rendinst::configurateRIGen(*appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getBlockByNameEx("config"));

    perInstDataDwords = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getInt("perInstDataDwords", 0);
    G_ASSERTF(perInstDataDwords <= 1,
      "rendInstGenExtraRender.inc.cpp fills out the 2nd and 3rd members at runtime (check addData[ADDITIONAL_DATA_IDX] = ...)");
    perInstDataUseSeed = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getBool("perInstDataUseSeed", false);
    debug("using perInstDataDwords=%d %s", perInstDataDwords, perInstDataUseSeed ? "[posSeed]" : "");

    rendinst::initRIGen(/*render*/ !d3d::is_stub_driver(), 80, 8000.0f, NULL, NULL, -1, 256.0f);
    frameBlkId = ShaderGlobal::getBlockId("global_frame");
    lastRendMask = 0;
    lastLayerHiddenMask = 0;
    v_bbox3_init(rayBoxExt, v_zero());

    landBlendNoiseTexId = BAD_TEXTUREID;
    initRendinstLandscapeBlend();

    treeTopProjectionTexId = BAD_TEXTUREID;
    initTreeTopProjection();

    rendinst::tmInst12x32bit = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getBool("tmInst12x32bit", false);
    rendinst::maxRiGenPerCell =
      appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getInt("maxRiGenPerCell", 0x10000);
    rendinst::maxRiExPerCell =
      appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getInt("maxRiExPerCell", 0x10000);
    rendinst::forceRiExtra =
      appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("rendInst")->getBool("forceRiExtra", false);
  }
  ~AcesRendInstEntityManagementService()
  {
    if (DAGORED2)
      DAGORED2->unregisterCustomCollider(this);
    if (rigenSrv)
    {
      rendinst::release_rt_rigen_data();
      rendinst::termRIGen();
      rigenSrv = NULL;
    }
    cleanTreeTopProjection();
    cleanRendinstLandscapeBlend();
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_riEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Render instance entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    setGlobalTexturesToShaders();

    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    if (!p.size())
      return;

    int subtypeMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();


    const int cnt = p.size();
    for (int i = 0; i < cnt; i++)
      p[i]->renderService(subtypeMask, lh_mask);

    if (CCameraElem::getCamera() > CCameraElem::MAX_CAMERA) // FPT, TPS...
    {
      begin_draw_cached_debug_lines();
      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_box(lastCacheTM * BBox3(as_point3(&traceCacheOOBB.bmin), as_point3(&traceCacheOOBB.bmax)),
        E3DCOLOR(255, 255, 255, 255));

#if HAS_GAME_PHYS
      gamephys::draw_trace_handle(&cachedTraceMeshFaces);
#endif
      end_draw_cached_debug_lines();
    }
  }
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      DAEDITOR3.conNote("RendInstMgr: %d entities (using %u different models, while %u models have no instances)",
        riPool.getUsedEntityCount(), riPool.getUsedPoolsCount(), riPool.getEmptyPoolsCount());
      riPool.dumpUsedPools();
    }
    else if (event_huid == HUID_BeforeMainLoop && delayRiResInit)
    {
      dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
      if (p.size())
      {
        FastNameMap resNameMap;
        for (int i = 0; i < p.size(); i++)
        {
          resNameMap.addNameId(p[i]->riResName);
          resNameMap.addNameId(String(0, "%s" RI_COLLISION_RES_SUFFIX, p[i]->riResName));
        }

        ::set_required_res_list_restriction(resNameMap);
        ::preload_all_required_res();
        ::reset_required_res_list_restriction();
      }
      for (int i = 0; i < p.size(); i++)
        p[i]->init1();
      {
        mat44f proj;
        d3d::gettm(TM_PROJ, proj);
        rendinst::prepare_rt_rigen_data_render(grs_cur_view.pos, grs_cur_view.itm, proj);
      }

      delayRiResInit = false;
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, ILevelResListBuilder);
    RETURN_INTERFACE(huid, IGatherStaticGeometry);
    RETURN_INTERFACE(huid, IExportToDag);
    // RETURN_INTERFACE(huid, IWriteAddLtinputData);
    RETURN_INTERFACE(huid, IWriteAddLtinputData);
    RETURN_INTERFACE(huid, IOccluderGeomProvider);
    RETURN_INTERFACE(huid, ILightingChangeClient);
    RETURN_INTERFACE(huid, IGatherCollision);
    return NULL;
  }

  // ILightingChangeClient
  virtual void onLightingChanged() {}
  virtual void onLightingSettingsChanged()
  {
    static int fromSunDirectionVarId = ::get_shader_glob_var_id("from_sun_direction");
    Color4 new_sun_dir = ShaderGlobal::get_color4_fast(fromSunDirectionVarId);
    rendinst::set_sun_dir_for_global_shadows(Point3(new_sun_dir.r, new_sun_dir.g, new_sun_dir.b));
  }

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    if ((st_mask & rendEntGeomMask) != rendEntGeomMask)
      return;

    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    rendinst::LayerFlags forceLodBits = {};
    if (force_lod_no >= 0)
      forceLodBits = rendinst::make_forced_lod_layer_flags(force_lod_no);

    int frame_block = -1;

    mat44f ident;
    v_mat44_ident(ident);
    d3d::settm(TM_WORLD, ident);
    mat44f globtm;
    d3d::getglobtm(globtm);
    switch (stage)
    {
      case STG_BEFORE_RENDER:
        if (lastRendMask != (st_mask & rendSubtypeSumMask) || lastLayerHiddenMask != lh_mask)
        {
          rendinst::enable_rigen_mask_generated(st_mask & maskTypeSubtypeMask);
          rendinst::discard_rigen_all();
          lastRendMask = (st_mask & rendSubtypeSumMask);
          lastLayerHiddenMask = lh_mask;
        }
        {
          mat44f proj;
          d3d::gettm(TM_PROJ, proj);
          rendinst::prepare_rt_rigen_data_render(grs_cur_view.pos, grs_cur_view.itm, proj);
        }
        rendinst::applyTiledScenesUpdateForRIGenExtra(2000, 1000);
        break;

      case STG_RENDER_STATIC_OPAQUE:
      {
        frame_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
        IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
        if (hmlService)
          hmlService->startUAVFeedback();
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, globtm, ::grs_cur_view.pos, ::grs_cur_view.itm,
          rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra | forceLodBits);
        if (hmlService)
          hmlService->endUAVFeedback();
        ShaderGlobal::setBlock(frame_block, ShaderGlobal::LAYER_FRAME);
      }
      break;

      case STG_RENDER_STATIC_DECALS:
        frame_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, globtm, ::grs_cur_view.pos, ::grs_cur_view.itm,
          rendinst::LayerFlag::Decals | forceLodBits);
        ShaderGlobal::setBlock(frame_block, ShaderGlobal::LAYER_FRAME);
        break;

      case STG_RENDER_STATIC_TRANS:
        frame_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, globtm, ::grs_cur_view.pos, ::grs_cur_view.itm,
          rendinst::LayerFlag::Transparent | forceLodBits);
        ShaderGlobal::setBlock(frame_block, ShaderGlobal::LAYER_FRAME);
        break;

      case STG_RENDER_STATIC_DISTORTION:
        frame_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, globtm, ::grs_cur_view.pos, ::grs_cur_view.itm,
          rendinst::LayerFlag::Distortion | forceLodBits);
        ShaderGlobal::setBlock(frame_block, ShaderGlobal::LAYER_FRAME);
        break;

      case STG_RENDER_SHADOWS:
        frame_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(frameBlkId, ShaderGlobal::LAYER_FRAME);
        rendinst::render::renderRIGen(rendinst::RenderPass::ToShadow, globtm, ::grs_cur_view.pos, ::grs_cur_view.itm,
          rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra | forceLodBits);
        ShaderGlobal::setBlock(frame_block, ShaderGlobal::LAYER_FRAME);
        break;
    }
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return rendInstEntityClassId >= 0 && rendInstEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    int pool_idx = riPool.findPool(asset);
    if (pool_idx < 0)
      pool_idx = riPool.addPool(new AcesRendInstEntityPool(asset));

    if (virtual_ent)
      return riPool.getVirtualEnt(pool_idx);

    if (!riPool.canAddEntity(pool_idx))
      return NULL;

    AcesRendInstEntity *ent = riPool.allocEntity();
    if (!ent)
      ent = new AcesRendInstEntity;
    ent->setColor(riPool.getPools()[pool_idx]->defColorIdx);

    riPool.addEntity(ent, pool_idx);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    AcesRendInstEntity *o = reinterpret_cast<AcesRendInstEntity *>(origin);
    if (!riPool.canAddEntity(o->poolIdx))
      return NULL;

    AcesRendInstEntity *ent = riPool.allocEntity();
    if (!ent)
      ent = new AcesRendInstEntity;
    ent->setColor(riPool.getPools()[o->poolIdx]->defColorIdx);

    riPool.addEntity(ent, o->poolIdx);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IRendInstGen interface
  virtual rendinst::gen::land::AssetData *getLandClassGameRes(const char *name)
  {
    FastNameMap landcls_nm;
    landcls_nm.addNameId(name);
    ::set_required_res_list_restriction(landcls_nm);
    GameResource *res = ::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), rendinst::HUID_LandClassGameRes);
    ::reset_required_res_list_restriction();
    return (rendinst::gen::land::AssetData *)res;
  }
  virtual void releaseLandClassGameRes(rendinst::gen::land::AssetData *res)
  {
    if (res)
      release_game_resource((GameResource *)res);
  }

  virtual void setCustomGetHeight(bool(__stdcall *c_get_height)(Point3 &pos, Point3 *out_norm))
  {
    ri_get_height = c_get_height;
    ri_update_pregen_pos_y = c_get_height ? &update_pregen_pos_y : NULL;
  }
  virtual void setSweepMask(EditableHugeBitmask *bm, float ox, float oz, float scale)
  {
    rendinst::set_rigen_sweep_mask(bm, ox, oz, scale);
  }

  virtual void clearRtRIGenData()
  {
    rendinst::release_rt_rigen_data();
    rigenLandBox.setempty();
  }
  virtual bool createRtRIGenData(float ofs_x, float ofs_z, float grid2world, int cell_sz, int cell_num_x, int cell_num_z,
    dag::ConstSpan<rendinst::gen::land::AssetData *> land_cls, float dens_map_px, float dens_map_pz)
  {
    if (g_debug_is_in_fatal)
      return false;
    debug("createRtRIGenData");
    rendinst::release_rt_rigen_data();
    rendinst::set_rt_pregen_gather_cb(&pregen_gather_pos_cb, &pregen_gather_tm_cb, &prepare_pool_mapping, &prepare_pools);

    rigenLandBox.setempty();
    if (!rendinst::create_rt_rigen_data(ofs_x, ofs_z, grid2world, cell_sz, cell_num_x, cell_num_z, perInstDataDwords, land_cls,
          dens_map_px, dens_map_pz))
      return false;
    rigenLandBox[0].set(ofs_x, ofs_z);
    rigenLandBox[1].set(ofs_x + cell_sz * cell_num_x * grid2world, ofs_z + cell_sz * cell_num_z * grid2world);

    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    for (int i = 0; i < p.size(); i++)
      p[i]->pregenId = rendinst::register_rt_pregen_ri(p[i]->res, p[i]->layerIdx, colRangeSrv->getColorFrom(p[i]->defColorIdx),
        colRangeSrv->getColorTo(p[i]->defColorIdx), p[i]->riResName);
    return true;
  }
  virtual void releaseRtRIGenData() { rendinst::release_rt_rigen_data(); }
  virtual EditableHugeBitMap2d *getRIGenBitMask(rendinst::gen::land::AssetData *land_cls)
  {
    return rendinst::get_rigen_bit_mask(land_cls);
  }
  virtual void discardRIGenRect(int gx0, int gz0, int gx1, int gz1) { rendinst::discard_rigen_rect(gx0, gz0, gx1, gz1); }

  virtual int getRIGenLayers() const { return rendinst::get_rigen_data_layers(); }
  virtual int getRIGenLayerCellSizeDivisor(int layer_idx) const { return rendinst::get_rigen_layers_cell_div(layer_idx); }

  virtual void setReinitCallback(void(__stdcall *reinit_cb)(void *arg), void *arg)
  {
    custom_reinit_cb = reinit_cb;
    custom_reinit_arg = arg;
  }
  virtual void resetLandClassCache() { rendinst::rt_rigen_free_unused_land_classes(); }

  virtual void onLevelBlkLoaded(const DataBlock &blk) { navmeshLayers.setGameName(blk.getStr("gameName", DEFAULT_LEVEL_GAMENAME)); }

  static void prepare_pools(bool prepare)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = static_cast<AcesRendInstEntityManagementService *>(rigenSrv)->riPool.getPools();
    for (int i = 0; i < p.size(); ++i)
      p[i]->precalculatedBox.setempty();
    if (prepare)
      for (int i = 0; i < p.size(); ++i)
        p[i]->buildBBox();
  }

  static void prepare_pool_mapping(Tab<int> &indices, int flg, unsigned riPoolBitsMask)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = static_cast<AcesRendInstEntityManagementService *>(rigenSrv)->riPool.getPools();
    for (int i = 0; i < p.size(); i++)
    {
      const int pregenId = p[i]->pregenId;
      const int idx = pregenId & riPoolBitsMask;
      if (idx >= indices.size() || (idx | flg) != pregenId)
        continue;
      G_ASSERT(indices[idx] < 0);
      indices[idx] = i;
    }
  }

  static void pregen_gather_pos_cb(Tab<Point4> &dest, Tab<int> &dest_per_inst_data, int idx, int pregen_id, float x0, float z0,
    float x1, float z1)
  {
    if (idx < 0)
      return;
    dag::ConstSpan<AcesRendInstEntityPool *> p = static_cast<AcesRendInstEntityManagementService *>(rigenSrv)->riPool.getPools();
    if (p[idx]->pregenId != pregen_id)
      return;
    p[idx]->gatherRiP4(dest, dest_per_inst_data, x0, z0, x1, z1, rigenLandBox);
  }
  static void pregen_gather_tm_cb(Tab<TMatrix> &dest, Tab<int> &dest_per_inst_data, int idx, int pregen_id, float x0, float z0,
    float x1, float z1)
  {
    if (idx < 0)
      return;
    dag::ConstSpan<AcesRendInstEntityPool *> p = static_cast<AcesRendInstEntityManagementService *>(rigenSrv)->riPool.getPools();
    if (p[idx]->pregenId != pregen_id)
      return;
    BBox2 box(x0, z0, x1, z1);
    if (!p[idx]->precalculatedBox.isempty() && !(p[idx]->precalculatedBox & box))
      return;
    p[idx]->gatherRiTM(dest, dest_per_inst_data, x0, z0, x1, z1, rigenLandBox);
  }

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &log, PropPanel2 *params)
  {
    if (!calcPerPoolEntities({}, IObjEntityFilter::STMASK_TYPE_EXPORT))
      log.addMessage(log.WARNING, "No rendInst entities for export");
    return true;
  }

  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }

  struct TmpEntityTm
  {
    TMatrix tm;
    unsigned int color;
    int seed;
  };

  struct TmpEntityPos
  {
    Point3 pos;
    float scale;
    unsigned int color;
    int seed;
  };

  struct TmpPool
  {
    Tab<TmpEntityTm> entitiesTmList;
    Tab<TmpEntityPos> entitiesPosList;
    bool isPos;
    bool hasPaletteRotation;
    unsigned int numEntitiesTotal;

    TmpPool() : entitiesTmList(tmpmem), entitiesPosList(tmpmem), numEntitiesTotal(0), isPos(false), hasPaletteRotation(false) {}

    unsigned int getNumEntities() { return isPos ? entitiesPosList.size() : entitiesTmList.size(); }

    Point3 getEntityPos(unsigned int entity_no)
    {
      G_ASSERT(entity_no < getNumEntities());
      return isPos ? entitiesPosList[entity_no].pos : entitiesTmList[entity_no].tm.getcol(3);
    }

    float getEntityScale(unsigned int entity_no)
    {
      G_ASSERT(entity_no < getNumEntities());
      if (isPos)
      {
        return entitiesPosList[entity_no].scale;
      }
      else
      {
        TMatrix &tm = entitiesTmList[entity_no].tm;
        return MAX(MAX(tm.getcol(0).length(), tm.getcol(1).length()), tm.getcol(2).length());
      }
    }

    BSphere3 getEntityBoundingSphere(BSphere3 &res_bounding_sphere, unsigned int entity_no)
    {
      G_ASSERT(entity_no < getNumEntities());
      if (isPos)
      {
        return BSphere3(entitiesPosList[entity_no].pos + res_bounding_sphere.c * entitiesPosList[entity_no].scale,
          res_bounding_sphere.r * entitiesPosList[entity_no].scale);
      }
      else
      {
        TMatrix &tm = entitiesTmList[entity_no].tm;
        float scale = MAX(MAX(tm.getcol(0).length(), tm.getcol(1).length()), tm.getcol(2).length());
        return BSphere3(tm * res_bounding_sphere.c, res_bounding_sphere.r * scale);
      }
    }
  };

  virtual void gatherCollision(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices, Tab<IPoint2> &transparent,
    Tab<NavMeshObstacle> &obstacles)
  {
    navmeshLayers.load();

    const int max_cores = 128;
    bool started[max_cores];
    int coresId[max_cores];
    int cores_count = 0;
    cpujobs::release_done_jobs();
    for (int ci = 0; ci < min(max_cores, cpujobs::get_core_count()); ++ci)
      if (!cpujobs::start_job_manager(ci, 1 << 20))
        started[ci] = false;
      else
      {
        started[ci] = true;
        coresId[cores_count] = ci;
        cores_count++;
      }
    if (cores_count > 1)
    {
      int64_t refdet = ref_time_ticks_qpc();
      int current_core = 0;
      const int subdivX = 4;
      const int subdivY = 4;
      for (int y = 0; y < subdivX; ++y)
        for (int x = 0; x < subdivY; ++x)
        {
          BBox3 jobBox = BBox3(Point3(lerp(box.lim[0].x, box.lim[1].x, x * safeinv(float(subdivX))), box.lim[0].y,
                                 lerp(box.lim[0].z, box.lim[1].z, y * safeinv(float(subdivY)))),
            Point3(lerp(box.lim[0].x, box.lim[1].x, (x + 1) * safeinv(float(subdivX))), box.lim[1].y,
              lerp(box.lim[0].z, box.lim[1].z, (y + 1) * safeinv(float(subdivY)))));
          cpujobs::add_job(coresId[current_core], new RendinstGatherCollJob(jobBox, vertices, indices, transparent, obstacles));
          current_core = (current_core + 1) % cores_count;
        }
      for (int ci = cores_count - 1; ci >= 0; --ci)
      {
        while (cpujobs::is_job_manager_busy(coresId[ci]))
          ::sleep_msec(10);
        DAEDITOR3.conNote("RendinstGatherCollJob core done in %.2f sec", get_time_usec_qpc(refdet) / 1000000.0);
        cpujobs::stop_job_manager(coresId[ci], cores_count ? true : false);
      }
      cpujobs::release_done_jobs();
    }
    else
    {
      RendinstVertexDataCbEditor cb(vertices, indices, transparent, navmeshLayers.pools, navmeshLayers.obstaclePools,
        navmeshLayers.materialPools, navmeshLayers.navMeshOffsetPools, obstacles);
      rendinst::testObjToRIGenIntersection(box, TMatrix::IDENT, cb, rendinst::GatherRiTypeFlag::RiGenAndExtra);
      cb.procAllCollision();
    }
  }

  virtual void getObstaclesFlags(const ska::flat_hash_map<uint32_t, uint32_t> *&obstacle_flags_by_res_name_hash)
  {
    obstacle_flags_by_res_name_hash = &navmeshLayers.obstacleFlags;
  }

  virtual bool buildAndWrite(BinDumpSaveCB &main_cwr, const ITextureNumerator &tn, PropPanel2 *)
  {
    G_ASSERT(DAGORED2);
    G_ASSERT(colRangeSrv);
    int sm_render = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER,
      IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT));

    rendinst::discard_rigen_all();
    {
      mat44f proj;
      d3d::gettm(TM_PROJ, proj);
      rendinst::prepare_rt_rigen_data_render(grs_cur_view.pos, grs_cur_view.itm, proj);
    }

    if (!riPool.getPools().size())
    {
      IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, sm_render);
      return true;
    }

    Tab<TmpPool> poolsList(tmpmem);
    poolsList.resize(riPool.getPools().size());


    // Check dynamic impostor and cloud.

    Tab<bool> isDynamicImpostorList(tmpmem);
    isDynamicImpostorList.resize(poolsList.size());
    mem_set_0(isDynamicImpostorList);

    for (unsigned int poolNo = 0; poolNo < poolsList.size(); poolNo++)
    {
      G_ASSERT(riPool.getPools()[poolNo]);
      if (!riPool.getPools()[poolNo]->res)
      {
        DAEDITOR3.conWarning("RendInst <%s> not resolved, instances export skipped", riPool.getPools()[poolNo]->riResName);
        poolsList[poolNo].isPos = false;
        poolsList[poolNo].hasPaletteRotation = false;
        continue;
      }
      G_ASSERT(riPool.getPools()[poolNo]->res);
      G_ASSERT(!riPool.getPools()[poolNo]->res->lods.empty());
      G_ASSERT(riPool.getPools()[poolNo]->res->lods.back().scene);
      G_ASSERT(riPool.getPools()[poolNo]->res->lods.back().scene->getMesh());
      G_ASSERT(riPool.getPools()[poolNo]->res->lods.back().scene->getMesh()->getMesh());
      G_ASSERT(riPool.getPools()[poolNo]->res->lods.back().scene->getMesh()->getMesh()->getMesh());

      ShaderMesh *mesh = riPool.getPools()[poolNo]->res->lods.back().scene->getMesh()->getMesh()->getMesh();

      poolsList[poolNo].isPos = isDynamicImpostorList[poolNo] = riPool.getPools()[poolNo]->res->hasImpostor();
      poolsList[poolNo].hasPaletteRotation = riPool.getPools()[poolNo]->res->isBakedImpostor();
    }


    // Add non-sowed entities.

    DAGORED2->getConsole().addMessage(ILogWriter::REMARK, "Fetching non-sowed entities...");
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();

    for (unsigned int poolNo = 0; poolNo < poolsList.size(); poolNo++)
    {
      for (unsigned int entityNo = 0; entityNo < riPool.getPools()[poolNo]->getEntities().size(); entityNo++)
      {
        if (riPool.getPools()[poolNo]->getEntities()[entityNo] &&
            riPool.getPools()[poolNo]->getEntities()[entityNo]->checkSubtypeAndLayerHiddenMasks(subtype_mask, lh_mask))
        {
          if (poolsList[poolNo].isPos)
          {
            append_items(poolsList[poolNo].entitiesPosList, 1);

            TMatrix &tm = riPool.getPools()[poolNo]->getEntities()[entityNo]->tm;
            poolsList[poolNo].entitiesPosList.back().pos = tm.getcol(3);
            poolsList[poolNo].entitiesPosList.back().scale = tm.getcol(1).length(); // Use Y scale to avoid flying trees.

            poolsList[poolNo].entitiesPosList.back().color =
              ((AcesRendInstEntity *)riPool.getPools()[poolNo]->getEntities()[entityNo])->colorIdx;
            poolsList[poolNo].entitiesPosList.back().seed =
              ((AcesRendInstEntity *)riPool.getPools()[poolNo]->getEntities()[entityNo])->instSeed;
          }
          if (!poolsList[poolNo].isPos || poolsList[poolNo].hasPaletteRotation)
          {
            append_items(poolsList[poolNo].entitiesTmList, 1);

            poolsList[poolNo].entitiesTmList.back().tm = riPool.getPools()[poolNo]->getEntities()[entityNo]->tm;

            poolsList[poolNo].entitiesTmList.back().color =
              ((AcesRendInstEntity *)riPool.getPools()[poolNo]->getEntities()[entityNo])->colorIdx;
            poolsList[poolNo].entitiesTmList.back().seed =
              ((AcesRendInstEntity *)riPool.getPools()[poolNo]->getEntities()[entityNo])->instSeed;
          }
          poolsList[poolNo].numEntitiesTotal++;
        }
      }
    }


    // Add sowed entities.

    DAGORED2->getConsole().addMessage(ILogWriter::REMARK, "Fetching sowed entities...");

    String projDir;
    DAGORED2->getProjectFolderPath(projDir);
    String fileName(260, "%s.blk", ::make_full_path(projDir, "DoNotCommitMe/sowedRi").str());

    DataBlock fileBlk(fileName);
    G_ASSERT(fileBlk.getBool("gen", false));

    // validate some data before build
    for (int layer = 0; layer < rendinst::get_rigen_data_layers(); layer++)
    {
      const DataBlock &b = get_layer_blk(fileBlk, layer);
      int ri_cell_sz = b.getInt("riCellSz", 1);
      if (ri_cell_sz < 1 || ri_cell_sz > 32767)
      {
        DAEDITOR3.conError("Bad riCellSz=%d for layer #%d, only 1...32767 is allowed", ri_cell_sz, layer);
        return false;
      }
    }

    exportGenEntities(fileBlk, main_cwr, poolsList, isDynamicImpostorList);
    fileBlk.reset();
    IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, sm_render);
    return true;
  }

  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    int cnt = calcPerPoolEntities({}, IObjEntityFilter::STMASK_TYPE_EXPORT);
    int maxCount = metrics_blk.getInt("max_entities", 0);

    if (cnt > maxCount)
    {
      DAEDITOR3.conError("Metrics validation failed: RendInst count=%d  > maximum allowed=%d.", cnt, maxCount);
      return false;
    }
    return true;
  }

  // IWriteAddLtinputData interface
  virtual void writeAddLtinputData(IGenSave &cwr)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    if (!p.size())
      return;

    // calculate per-pool count of filtered entities
    SmallTab<int, TmpmemAlloc> cnt;
    clear_and_resize(cnt, p.size());
    if (!calcPerPoolEntities(make_span(cnt), IObjEntityFilter::STMASK_TYPE_EXPORT))
      return;

    // get list of needed GRP and write them all to stream
    Tab<const char *> needed_res(tmpmem);
    Tab<String> grp_files(tmpmem);

    needed_res.reserve(p.size());
    for (int i = 0; i < p.size(); i++)
      if (cnt[i])
        needed_res.push_back(p[i]->getResName());

    ::get_used_gameres_packs(needed_res, true, grp_files);
    cwr.beginTaggedBlock(_MAKE4C('GRPs'));
    for (int i = 0; i < grp_files.size(); i++)
    {
      cwr.beginBlock();
      ::copy_file_to_stream(grp_files[i], cwr);
      cwr.endBlock();
      // debug_ctx ( "grp[%d]=%s", i, (char*)grp_files[i] );
    }
    cwr.endBlock();

    // write render instance groups
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    TMatrix tm;

    for (int i = 0; i < p.size(); i++)
      if (cnt[i])
      {
        cwr.beginTaggedBlock(_MAKE4C('RIns'));
        cwr.writeString(p[i]->getResName());
        cwr.writeInt(cnt[i]);

        dag::ConstSpan<AcesRendInstEntity *> ent = p[i]->getEntities();
        for (int j = 0; j < ent.size(); j++)
          if (ent[j] && ent[j]->checkSubtypeMask(subtype_mask))
          {
            ent[j]->getTm(tm);
            cwr.write(&tm, sizeof(tm));
          }
        cwr.endBlock();
      }
  }

  // ILevelResListBuilder
  virtual void addUsedResNames(OAHashNameMap<true> &res_list)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    if (!p.size())
      return;

    for (int i = 0; i < p.size(); i++)
      if (p[i]->getUsedEntityCount())
        res_list.addNameId(p[i]->getResName());

    String projDir;
    DAGORED2->getProjectFolderPath(projDir);
    String fileName(260, "%s.blk", ::make_full_path(projDir, "DoNotCommitMe/sowedRi").str());
    DataBlock fileBlk(fileName);
    int nid_mask = fileBlk.getNameId("mask");
    G_ASSERT(fileBlk.getBool("gen", false));
    for (int layer = 0; layer < rendinst::get_rigen_data_layers(); layer++)
    {
      const DataBlock &layerBlk = get_layer_blk(fileBlk, layer);
      for (int i = 0; i < layerBlk.blockCount(); i++)
        if (layerBlk.getBlock(i)->getBlockNameId() == nid_mask)
          res_list.addNameId(layerBlk.getBlock(i)->getStr("land"));
    }
  }

  // IGatherStaticGeometry
  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &container) {}
  virtual void gatherStaticEnviGeometry(StaticGeometryContainer & /*container*/) {}

  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &container)
  {
    addGeometry(container, IObjEntityFilter::STMASK_TYPE_COLLISION);
  }
  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &container)
  {
    addGeometry(container, IObjEntityFilter::STMASK_TYPE_COLLISION);
  }

  virtual void gatherExportToDagGeometry(StaticGeometryContainer &container)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    const int pCnt = p.size();
    for (int i = 0; i < pCnt; i++)
      p[i]->gatherExportToDagGeometry(container);
  }


  // IDagorEdCustomCollider
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
  {
    if (traceMode == 1)
    {
      if (dir.y < -0.999f)
      {
        Trace trace(p, Point3(0, -1, 0), maxt, nullptr);
        rendinst::RendInstDesc ri_desc;
        bbox3f box;
        vec4f vFrom = *(vec3f *)&trace.pos.x;
        v_bbox3_init(box, vFrom);
        v_bbox3_add_pt(box, v_madd(*(vec4f *)&trace.dir.x, v_splat_w(vFrom), vFrom));
        v_bbox3_add_pt(box, v_add(vFrom, rayBoxExt.bmin));
        v_bbox3_add_pt(box, v_add(vFrom, rayBoxExt.bmax));
        if (rendinst::traceDownMultiRay(make_span(&trace, 1), box, make_span(&ri_desc, 1), NULL, -1, rendinst::TraceFlag::Destructible,
              navmeshLayers.pools.size() ? &navmeshLayers.pools : nullptr))
          if (trace.pos.outT > 0)
          {
            maxt = trace.pos.outT;
            if (norm)
              *norm = trace.outNorm;
            return true;
          }
      }
      else
      {
        Point3 dummyNorm;
        Point3 &resultNorm = norm ? *norm : dummyNorm;
        return rendinst::traceRayRendInstsNormalized(p, dir, maxt, resultNorm, false, true);
      }
      return false;
    }

    if (maxt <= 0)
      return false;

    dag::ConstSpan<AcesRendInstEntityPool *> pool = riPool.getPools();
    bool ret = false;
    for (int i = 0; i < pool.size(); i++)
      if (pool[i]->traceRay(p, dir, maxt, norm))
        ret = true;
    return ret;
  }

  virtual void clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized)
  {
    rendinst::clipCapsuleRI(c, cp1, cp2, md, movedirNormalized, &cachedTraceMeshFaces);
  }

  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) { return false; }
  virtual const char *getColliderName() const { return getServiceFriendlyName(); }
  virtual bool isColliderVisible() const { return visible; }
  virtual bool setupColliderParams(int mode, const BBox3 &area)
  {
    v_bbox3_init(rayBoxExt, v_zero());
    if (mode == 1)
    {
#if HAS_GAME_PHYS
      bbox3f areaBB;
      areaBB.bmin = v_ldu(&area[0].x);
      areaBB.bmax = v_ldu(&area[1].x);

      if (!dacoll::try_use_trace_cache(areaBB, &cachedTraceMeshFaces) || !area.isempty())
      {
        {
          mat44f proj;
          d3d::gettm(TM_PROJ, proj);
          rendinst::prepare_rt_rigen_data_render(area.center(), grs_cur_view.itm, proj);
        }
        rendinst::generate_rt_rigen_main_cells(area);
        rendinst::calculate_box_extension_for_objects_in_grid(rayBoxExt);
      }

      if (!area.isempty())
      {
        TMatrix tm = TMatrix::IDENT;
        tm.setcol(3, area.center());
        lastCacheTM = tm;

        dacoll::validate_trace_cache_oobb(tm, traceCacheOOBB, traceCacheExpands, 0.f, &cachedTraceMeshFaces);
      }
#endif
    }
    traceMode = mode;
    return true;
  }

  void prepareCollider() override { navmeshLayers.load(); }

  // IOccluderGeomProvider
  virtual void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    for (int i = 0; i < p.size(); i++)
      p[i]->gatherOccluders(occl_boxes, occl_quads);
  }
  virtual void renderOccluders(const Point3 &camPos, float max_dist)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    for (int i = 0; i < p.size(); i++)
      p[i]->renderOccluders(camPos, max_dist);
  }

  BBox3 calcWorldBBox() const override
  {
    BBox3 worldBBox;
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    for (int i = 0; i < p.size(); i++)
      if (!p[i]->isEmpty() && p[i]->isValid())
      {
        for (auto e : p[i]->getEntities())
        {
          if (!e)
            continue;
          TMatrix tm;
          e->getTm(tm);
          worldBBox += tm * p[i]->getBbox();
        }
      }
    return worldBBox;
  }

protected:
  TraceMeshFaces cachedTraceMeshFaces; // for custom collider capsule collision
  bbox3f traceCacheOOBB;
  vec3f traceCacheExpands;
  TMatrix lastCacheTM;
  MultiAcesRendInstEntityPool riPool;
  bool visible;
  int lastRendMask;
  uint64_t lastLayerHiddenMask;
  TEXTUREID treeTopProjectionTexId;
  float treeTopProjectionNormalY, treeTopProjectionSharpness;

  TEXTUREID landBlendNoiseTexId;

  void initRendinstLandscapeBlend()
  {
    DataBlock *gameParamsBlkb = const_cast<DataBlock *>(dgs_get_game_params());
    const DataBlock *rendinstLandBlendBlk = gameParamsBlkb->getBlockByNameEx("rendinstLandBlend");
    landBlendNoiseTexId = d3d::check_texformat(TEXFMT_ATI1N) ? get_tex_gameres("perlin_noise_ati1n") : BAD_TEXTUREID;
    if (landBlendNoiseTexId == BAD_TEXTUREID)
      landBlendNoiseTexId = get_tex_gameres("perlin_noise");
    static int perlinTexVarId = get_shader_variable_id("perlin_tex", true);
    ShaderGlobal::set_texture(perlinTexVarId, landBlendNoiseTexId);

    float rendinstLandBlendSharpness = rendinstLandBlendBlk->getReal("blendSharpness", 8.f);
    float rendinstLandBlendAngleMin = sinf(rendinstLandBlendBlk->getReal("appearanceMinAngle", 12.f) / DEG_TO_RAD);
    float rendinstLandBlendAngleMax = sinf(rendinstLandBlendBlk->getReal("appearanceMaxAngle", 25.f) / DEG_TO_RAD);
    float rendinstLandBlendPerlinTexScale = rendinstLandBlendBlk->getReal("perlinTexScale", 0.25f);
    ShaderGlobal::set_color4(get_shader_variable_id("rendinst_land_blend", true), rendinstLandBlendSharpness,
      rendinstLandBlendAngleMin, rendinstLandBlendAngleMax, rendinstLandBlendPerlinTexScale);
  }

  void cleanRendinstLandscapeBlend()
  {
    release_managed_tex(landBlendNoiseTexId);
    landBlendNoiseTexId = BAD_TEXTUREID;
  }

  void initTreeTopProjection()
  {
    DataBlock *b = const_cast<DataBlock *>(dgs_get_game_params());
    const DataBlock *treesTopProjectionBlk = b->getBlockByNameEx("TreesTopProjection");

    const char *treesTopProjectionTexName = treesTopProjectionBlk->getStr("texName", "tree_snow_d");
    treeTopProjectionTexId = treesTopProjectionTexName ? get_tex_gameres(treesTopProjectionTexName) : BAD_TEXTUREID;

    treeTopProjectionNormalY = treesTopProjectionBlk->getReal("normalY", 0.2f);
    treeTopProjectionSharpness = treesTopProjectionBlk->getReal("sharpness", 5.f);
  }

  void cleanTreeTopProjection()
  {
    release_managed_tex(treeTopProjectionTexId);
    treeTopProjectionTexId = BAD_TEXTUREID;
  }

  void setGlobalTexturesToShaders()
  {
    static int treeTopProjectionTexVarId = get_shader_variable_id("trees_top_projection_tex", true);
    ShaderGlobal::set_texture(treeTopProjectionTexVarId, treeTopProjectionTexId);

    ShaderGlobal::set_color4(get_shader_variable_id("tree_top_projection_params", true), treeTopProjectionNormalY,
      treeTopProjectionSharpness, 0.f, 0.f);
  }


  inline void addGeometry(StaticGeometryContainer &container, int mask_type)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    const int st_mask = IObjEntityFilter::getSubTypeMask(mask_type);
    const int pCnt = p.size();
    for (int i = 0; i < pCnt; i++)
      p[i]->addGeometry(container, st_mask, mask_type);
  }

  int calcPerPoolEntities(dag::Span<int> cnt, int filter_type, int *nonvoid_pools_count = NULL)
  {
    dag::ConstSpan<AcesRendInstEntityPool *> p = riPool.getPools();
    int subtype_mask = IObjEntityFilter::getSubTypeMask(filter_type);
    int total = 0;

    mem_set_0(cnt);
    if (nonvoid_pools_count)
      *nonvoid_pools_count = 0;

    for (int i = 0; i < p.size(); i++)
      if (!p[i]->isEmpty() && p[i]->isValid())
      {
        int n = p[i]->calcEntities(subtype_mask);

        total += n;
        if (i < cnt.size())
          cnt[i] = n;

        if (nonvoid_pools_count && n)
          nonvoid_pools_count[0]++;
      }

    return total;
  }

  static inline bool isMaskRefParam(const char *pname) { return pname[0] == 'm' && pname[1] >= '0' && pname[1] <= '9'; }
  static inline int countMaskRefParam(const DataBlock &b)
  {
    int cnt = 0;
    for (int i = 0; i < b.paramCount(); i++)
      if (isMaskRefParam(b.getParamName(i)))
        cnt++;
    return cnt;
  }

  void exportGenEntities(BinDumpSaveCB &cwr, BinDumpSaveCB &cdata_cwr, const DataBlock &blk, dag::ConstSpan<TmpPool> poolsList,
    dag::ConstSpan<bool> isDynamicImpostorList, int layer_idx)
  {
    static const int SUBCELL_DIV = 8;

    rendinst::gen::RotationPaletteManager *rotationPaletteMgr = rendinst::gen::get_rotation_palette_manager();
    G_ASSERT(rotationPaletteMgr);

    int ri_cell_sz = blk.getInt("riCellSz", 1);
    int ri_cw = blk.getInt("riNumW", 1), ri_ch = blk.getInt("riNumH", 1);
    int mask_num = blk.getInt("maskNum", 0);
    mkbindump::PatchTabRef tabCls, tabCell, tabPregenPool;
    Tab<int> tmpOfs(tmpmem);
    int ri_atype = DAEDITOR3.getAssetTypeId("rendInst");
    int land_atype = DAEDITOR3.getAssetTypeId("land");

    Tab<int> riPoolReMap(tmpmem);
    int usedRiPoolCnt = 0;

    riPoolReMap.resize(poolsList.size());
    mem_set_ff(riPoolReMap);
    for (int i = 0; i < poolsList.size(); i++)
    {
      if (poolsList[i].entitiesPosList.size() + poolsList[i].entitiesTmList.size() == 0)
        continue;

      riPoolReMap[i] = (riPool.getPools()[i] && riPool.getPools()[i]->layerIdx == layer_idx) ? usedRiPoolCnt++ : -1;
    }

    Tab<Tab<Tabint>> riLdCnt(tmpmem);
    riLdCnt.resize(usedRiPoolCnt);

    Point3 gridOfs(blk.getReal("gridOfsX"), 0, blk.getReal("gridOfsZ"));
    const float cellSz = ri_cell_sz * blk.getReal("gridCell");
    const float subCellSz = cellSz / SUBCELL_DIV;

    IBBox2 used_cells_bbox;
    for (int row = 0; row < ri_ch; row++)
    {
      const DataBlock *b_row = blk.getBlock(mask_num + row);
      for (int col = 0; col < ri_cw; col++)
      {
        const DataBlock &b = *b_row->getBlock(col);
        if (countMaskRefParam(b) > 0)
          used_cells_bbox += IPoint2(col, row);
      }
    }
    for (int i = 0; i < poolsList.size(); i++)
    {
      int idx = riPoolReMap[i];
      if (idx < 0)
        continue;
      bool isPos = poolsList[i].isPos;
      for (int j = 0, je = isPos ? poolsList[i].entitiesPosList.size() : poolsList[i].entitiesTmList.size(); j < je; j++)
      {
        Point3 p = (isPos ? poolsList[i].entitiesPosList[j].pos : poolsList[i].entitiesTmList[j].tm.getcol(3));
        int ix = clamp(int((p.x - gridOfs.x) / cellSz), 0, ri_cw - 1);
        int iy = clamp(int((p.z - gridOfs.z) / cellSz), 0, ri_ch - 1);
        used_cells_bbox += IPoint2(ix, iy);
      }
    }
    if (used_cells_bbox.isEmpty())
      used_cells_bbox[0] = used_cells_bbox[1] = IPoint2(0, 0);
    else
      used_cells_bbox[1].x++, used_cells_bbox[1].y++;

    bool crop_area =
      used_cells_bbox[0].x != 0 || used_cells_bbox[0].y != 0 || used_cells_bbox[1].x != ri_cw || used_cells_bbox[1].y != ri_ch;

    DAEDITOR3.conNote("rgLayer.%d: %dx%d cells (each cell is %dx%d grid with gridCellSz=%.1f)\n"
                      "  area=(%.1f,%.1f)-(%.1f,%.1f) used=(%.1f,%.1f)-(%.1f,%.1f) usedRiPoolCnt=%d/%d  riLdCnt.sz=%d * %d",
      layer_idx + 1, ri_cw, ri_ch, ri_cell_sz, ri_cell_sz, subCellSz * SUBCELL_DIV / ri_cell_sz, gridOfs.x, gridOfs.z,
      gridOfs.x + ri_cw * cellSz, gridOfs.z + ri_ch * cellSz, gridOfs.x + used_cells_bbox[0].x * cellSz,
      gridOfs.z + used_cells_bbox[0].y * cellSz, gridOfs.x + used_cells_bbox[1].x * cellSz, gridOfs.z + used_cells_bbox[1].y * cellSz,
      usedRiPoolCnt, poolsList.size(), usedRiPoolCnt, size_t(ri_cw * ri_ch) * SUBCELL_DIV * SUBCELL_DIV * sizeof(Tabint)); //-V1028
    if (crop_area)
    {
      gridOfs.x += used_cells_bbox[0].x * cellSz;
      gridOfs.z += used_cells_bbox[0].y * cellSz;
      ri_cw = used_cells_bbox.size().x;
      ri_ch = used_cells_bbox.size().y;
      DAEDITOR3.conNote("rgLayer.%d: cropped to %dx%d cells (each cell is %dx%d grid with gridCellSz=%.1f)\n"
                        "  cropped_area=(%.1f,%.1f)-(%.1f,%.1f)",
        layer_idx + 1, ri_cw, ri_ch, ri_cell_sz, ri_cell_sz, subCellSz * SUBCELL_DIV / ri_cell_sz, gridOfs.x, gridOfs.z,
        gridOfs.x + ri_cw * cellSz, gridOfs.z + ri_ch * cellSz);
    }

    for (int i = 0; i < poolsList.size(); i++)
    {
      int idx = riPoolReMap[i];
      if (idx < 0)
        continue;
      Tab<Tabint> &cur_cnt = riLdCnt[idx];
      cur_cnt.resize(ri_cw * ri_ch * SUBCELL_DIV * SUBCELL_DIV);
      bool isPos = poolsList[i].isPos;
      for (int j = 0, je = isPos ? poolsList[i].entitiesPosList.size() : poolsList[i].entitiesTmList.size(); j < je; j++)
      {
        Point3 p = (isPos ? poolsList[i].entitiesPosList[j].pos : poolsList[i].entitiesTmList[j].tm.getcol(3));
        int ix = clamp(int((p.x - gridOfs.x) / subCellSz), 0, ri_cw * SUBCELL_DIV - 1);
        int iy = clamp(int((p.z - gridOfs.z) / subCellSz), 0, ri_ch * SUBCELL_DIV - 1);
        cur_cnt[SUBCELL_DIV * SUBCELL_DIV * ((iy / SUBCELL_DIV) * ri_cw + (ix / SUBCELL_DIV)) + (iy % SUBCELL_DIV) * SUBCELL_DIV +
                (ix % SUBCELL_DIV)]
          .push_back(j);
      }
    }

    Tab<Point2> cellHtLim(tmpmem);
    cellHtLim.resize(ri_cw * ri_ch);

    Tab<RendInstGenData::PregenEntCounter> entCntStor(tmpmem);
    Tab<int> entCntRef(tmpmem), entDataOfs(tmpmem);
    entCntRef.resize(ri_cw * ri_ch * (SUBCELL_DIV * SUBCELL_DIV + 1));
    entDataOfs.resize(ri_cw * ri_ch);
    mem_set_ff(entDataOfs);
    for (int i = 0; i < ri_cw * ri_ch; i++)
    {
      if (!rendinst::compute_rt_rigen_tight_ht_limits(layer_idx, used_cells_bbox[0].x + (i % ri_cw),
            used_cells_bbox[0].y + (i / ri_cw), cellHtLim[i].x, cellHtLim[i].y))
        cellHtLim[i].x = 0, cellHtLim[i].y = 8192;

      BinDumpSaveCB cell_cwr(64 << 10, cwr.getTarget(), cwr.WRITE_BE);
      float cell_xz_sz = SUBCELL_DIV * subCellSz;
      float cell_y_sz = cellHtLim[i].y;
      float ox = gridOfs.x + (i % ri_cw) * cell_xz_sz;
      float oy = cellHtLim[i].x;
      float oz = gridOfs.z + (i / ri_cw) * cell_xz_sz;
      rendinst::gen::InstancePackData packData{ox, oy, oz, cell_xz_sz, cell_y_sz, 0};
      // debug("%d:%d,%d: cellHtLim[%d]=%@ oy=%.3f, cell_y_sz=%.3f", layer_idx, i%ri_cw, i/ri_cw, i, cellHtLim[i], oy, cell_y_sz);

      for (int j = 0; j < SUBCELL_DIV * SUBCELL_DIV; j++)
      {
        entCntRef[i * (SUBCELL_DIV * SUBCELL_DIV + 1) + j] = entCntStor.size();
        for (int k = 0; k < usedRiPoolCnt; k++)
          if (int cnt = riLdCnt[k][i * SUBCELL_DIV * SUBCELL_DIV + j].size())
          {
            entCntStor.push_back().set(k, cnt);

            const int *ent_idx = riLdCnt[k][i * SUBCELL_DIV * SUBCELL_DIV + j].data();
            int poolId = find_value_idx(riPoolReMap, k);
            const TmpPool &pool = poolsList[poolId];
            // TODO use a better writing method and try to move the writing code out of the if blocks
            if (pool.isPos)
              for (int m = 0; m < cnt; m++)
              {
                const TmpEntityPos &p = pool.entitiesPosList[ent_idx[m]];
                int32_t palette_id = -1;
                if (pool.hasPaletteRotation)
                {
                  rendinst::gen::RotationPaletteManager::Palette palette =
                    rendinst::gen::get_rotation_palette_manager()->getPalette({layer_idx, poolId});
                  G_ASSERT(palette.count > 0);
                  rendinst::gen::RotationPaletteManager::clamp_euler_angles(palette, pool.entitiesTmList[ent_idx[m]].tm, &palette_id);
                }
                int16_t data[4];
                size_t size =
                  rendinst::gen::pack_entity_pos_inst_16(packData, Point3(p.pos.x, p.pos.y, p.pos.z), p.scale, palette_id, data);
                G_ASSERT(size == 4 * sizeof(data[0]));
                for (int i = 0; i < 4; ++i)
                  cell_cwr.writeInt16e(data[i]);
                addPerInstData(cell_cwr, p.seed);
                // debug("y=%.1f -> %04X -> %.1f", p.pos.y, int(clamp((p.pos.y - oy)/cell_y_sz, -1.f, 1.f) *32767.0),
                //   int(clamp((p.pos.y - oy)/cell_y_sz, -1.f, 1.f) *32767.0)*cell_y_sz/32767.0+oy);
              }
            else if (!rendinst::tmInst12x32bit)
              for (int m = 0; m < cnt; m++)
              {
                const TMatrix &tm = pool.entitiesTmList[ent_idx[m]].tm;
                int16_t data[12];
                size_t size = rendinst::gen::pack_entity_tm_16(packData, tm, data);
                G_ASSERT(size == 12 * sizeof(data[0]));
                for (int i = 0; i < 12; ++i)
                  cell_cwr.writeInt16e(data[i]);
                addPerInstData(cell_cwr, pool.entitiesTmList[ent_idx[m]].seed);
                // debug("y=%.1f -> %04X -> %.1f", tm.m[3][1], int(clamp((tm.m[3][1] - oy)/cell_y_sz, -1.f, 1.f) *32767.0),
                //   int(clamp((tm.m[3][1] - oy)/cell_y_sz, -1.f, 1.f) *32767.0)*cell_y_sz/32767.0+oy);
              }
            else
              for (int m = 0; m < cnt; m++)
              {
                const TMatrix &tm = pool.entitiesTmList[ent_idx[m]].tm;
                int32_t data[12];
                size_t size = rendinst::gen::pack_entity_tm_32(packData, tm, data);
                G_ASSERT(size == 12 * sizeof(data[0]));
                for (int i = 0; i < 12; ++i)
                  cell_cwr.writeInt32e(data[i]);
                addPerInstData(cell_cwr, pool.entitiesTmList[ent_idx[m]].seed);
              }

            clear_and_shrink(riLdCnt[k][i * SUBCELL_DIV * SUBCELL_DIV + j]);
          }
      }
      entCntRef[i * (SUBCELL_DIV * SUBCELL_DIV + 1) + SUBCELL_DIV * SUBCELL_DIV] = entCntStor.size();

      if (cell_cwr.getSize())
      {
        entDataOfs[i] = cdata_cwr.tell();

        MemoryLoadCB mcrd(cell_cwr.getMem(), false);
        cdata_cwr.beginBlock();
        if (ShaderMeshData::preferZstdPacking)
          zstd_compress_data(cdata_cwr.getRawWriter(), mcrd, cell_cwr.getSize());
        else
          lzma_compress_data(cdata_cwr.getRawWriter(), 9, mcrd, cell_cwr.getSize());
        cdata_cwr.endBlock(ShaderMeshData::preferZstdPacking ? btag_compr::ZSTD : btag_compr::UNSPECIFIED);
      }
    }


    float grid2world = blk.getReal("gridCell");
    float world0x = gridOfs.x;
    float world0z = gridOfs.z;
    float invGridCellSzV = 1.f / (grid2world * ri_cell_sz);
    float lastCellXF = ri_cw - 0.9;
    float lastCellZF = ri_ch - 0.9;

    unsigned dataFlags = RendInstGenData::DFLG_PREGEN_ENT32_CNT32;

    cwr.beginBlock();
    cwr.setOrigin();
    // struct RendInstGenData
    cwr.writePtr64e(0);                               // PATCHABLE_DATA64(RtData*, rtData);
    tabCell.reserveTab(cwr);                          // PatchableTab<Cell> cells;
    cwr.writeInt32e(ri_cw);                           // int32_t cellNumW;
    cwr.writeInt32e(ri_ch);                           // int32_t cellNumH;
    cwr.writeInt16e(ri_cell_sz);                      // int32_t cellSz;
    cwr.writeInt8e(dataFlags);                        // uint8_t dataFlags;
    cwr.writeInt8e(perInstDataDwords);                // uint8_t perInstDataDwords;
    cwr.writeFloat32e(blk.getReal("sweepMaskScale")); // float sweepMaskScale;
    cwr.writePtr64e(0);                               // PATCHABLE_DATA64(HugeBitmask*, sweepMask);
    tabCls.reserveTab(cwr);                           // PatchableTab<LandClassRec> landCls;
    cwr.writeFloat32e(world0x);                       // vec4f world0Vxz;
    cwr.writeFloat32e(world0z);
    cwr.writeFloat32e(world0x);
    cwr.writeFloat32e(world0z);
    cwr.writeFloat32e(invGridCellSzV); // vec4f invGridCellSzV;
    cwr.writeFloat32e(invGridCellSzV);
    cwr.writeFloat32e(invGridCellSzV);
    cwr.writeFloat32e(invGridCellSzV);
    cwr.writeFloat32e(lastCellXF); // vec4f lastCellXZXZ;
    cwr.writeFloat32e(lastCellZF);
    cwr.writeFloat32e(lastCellXF);
    cwr.writeFloat32e(lastCellZF);
    cwr.writeFloat32e(grid2world);                   // float grid2world;
    cwr.writeFloat32e(blk.getReal("densMapPivotX")); // float densMapPivotX;
    cwr.writeFloat32e(blk.getReal("densMapPivotZ")); // float densMapPivotZ;
    cwr.writeInt32e(0);                              // int32_t pregenDataBaseOfs;
    tabPregenPool.reserveTab(cwr);                   // PatchableTab<PregenEntPoolDesc> pregenEnt;
    cwr.writeInt64e(0);                              // PATCHABLE_DATA64(file_ptr_t, fpLevelBin);
    cwr.align16();

    tmpOfs.resize(mask_num);
    for (int i = 0; i < mask_num; i++)
    {
      tmpOfs[i] = cwr.tell();
      const char *nm = blk.getBlock(i)->getStr("land");
      cwr.writeRaw(nm, i_strlen(nm) + 1);
    }

    cwr.align8();
    tabCls.startData(cwr, mask_num);
    for (int i = 0; i < mask_num; i++)
    {
      // struct RendInstGenData::LandClassRec
      cwr.writePtr64e(tmpOfs[i]); // PatchablePtr<const char> landClassName;
      cwr.writePtr64e(0);         // PATCHABLE_DATA64(rendinst::gen::land::AssetData*, asset);
      cwr.writePtr64e(0);         // PATCHABLE_DATA64(HugeBitmask*, mask);
      cwr.writePtr64e(0);         // PatchablePtr<int16_t> riResMap;
    }
    tabCls.finishTab(cwr);

    tmpOfs.resize(ri_cw * ri_ch);
    int col0 = used_cells_bbox[0].x;
    int row0 = used_cells_bbox[0].y;
    for (int row = 0; row < ri_ch; row++)
    {
      const DataBlock *b_row = blk.getBlock(mask_num + row + row0);
      for (int col = 0; col < ri_cw; col++)
      {
        const DataBlock *b = b_row->getBlock(col + col0);
        int i = row * ri_cw + col;
        tmpOfs[i] = cwr.tell();
        for (int j = 0; j < b->paramCount(); j++)
        {
          if (!isMaskRefParam(b->getParamName(j)))
            continue;
          // struct RendInstGenData::Cell::LandClassCoverage
          cwr.writeInt32e(atoi(b->getParamName(j) + 1)); // int landClsIdx;
          cwr.writeInt32e(b->getInt(j));                 // int msq;
        }
      }
    }

    int tabCellSize = ri_cw * ri_ch * (cwr.TAB_SZ + cwr.PTR_SZ + 2 + 2 + 4 + (SUBCELL_DIV * SUBCELL_DIV + 1) * cwr.PTR_SZ);

    int ent_cnt_stor_base = cwr.tell() + tabCellSize;
    tabCell.startData(cwr, ri_cw * ri_ch);
    for (int row = 0; row < ri_ch; row++)
    {
      const DataBlock *b_row = blk.getBlock(mask_num + row + row0);
      for (int col = 0; col < ri_cw; col++)
      {
        const DataBlock &b = *b_row->getBlock(col + col0);
        int i = row * ri_cw + col;
        // struct RendInstGenData::Cell
        cwr.writeRef(tmpOfs[i], countMaskRefParam(b));          // PatchableTab<LandClassCoverage> cls;
        cwr.writePtr64e(0);                                     // PATCHABLE_DATA64(CellRtData*, rtData);
        cwr.writeInt16e(cellHtLim[i].x);                        // int16_t htMin;
        cwr.writeInt16e(cellHtLim[i].y);                        // int16_t htDelta;
        cwr.writeInt32e(entDataOfs[i]);                         // int32_t riDataRelOfs;
        for (int j = 0; j < SUBCELL_DIV * SUBCELL_DIV + 1; j++) // PatchablePtr<PregenEntCounter> entCnt[SUBCELL_DIV*SUBCELL_DIV+1];
          cwr.writePtr64e(ent_cnt_stor_base + entCntRef[i * (SUBCELL_DIV * SUBCELL_DIV + 1) + j] * elem_size(entCntStor));
      }
    }
    tabCell.finishTab(cwr);
    G_ASSERT(cwr.tell() == ent_cnt_stor_base);

    cwr.write32ex(entCntStor.data(), data_size(entCntStor));

    tabPregenPool.startData(cwr, usedRiPoolCnt);
    int pos = cwr.tell();
    for (int i = 0; i < poolsList.size(); i++)
      if (riPoolReMap[i] >= 0)
      {
        unsigned ci = poolsList[i].isPos ? poolsList[i].entitiesPosList[0].color : poolsList[i].entitiesTmList[0].color;

        const int descStart = cwr.tell();

        // struct RendInstGenData::PregenEntPoolDesc
        cwr.writePtr64e(0);                                // PatchablePtr<RenderableInstanceLodsResource> riRes;
        cwr.writePtr64e(0);                                // PatchablePtr<const char> riName;
        cwr.writeE3dColorE(colRangeSrv->getColorFrom(ci)); // E3DCOLOR colPair[2];
        cwr.writeE3dColorE(colRangeSrv->getColorTo(ci));

        unsigned bits = 0;
        if (poolsList[i].isPos)
          bits |= 1 << 0;
        if (poolsList[i].hasPaletteRotation)
          bits |= 1 << 1;
        cwr.writeInt32e(bits); // uint32_t posInst:1, paletteRotation:1, _resv30:30;
        uint32_t paletteSize = poolsList[i].hasPaletteRotation ? rotationPaletteMgr->getPalette({layer_idx, i}).count : 0;
        cwr.writeInt32e(paletteSize); // int32_t paletteRotationCount;

        G_ASSERTF(sizeof(RendInstGenData::PregenEntPoolDesc) == cwr.tell() - descStart, "PregenEntPoolDesc is not exported properly");
      }
    tabPregenPool.finishTab(cwr);
    for (int i = 0; i < poolsList.size(); i++)
      if (riPoolReMap[i] >= 0)
      {
        cwr.writeInt32eAt(cwr.tell(), pos + riPoolReMap[i] * (2 * cwr.PTR_SZ + 4 * sizeof(int)) + cwr.PTR_SZ);
        const char *nm = riPool.getPools()[i]->riResName;
        cwr.writeRaw(nm, i_strlen(nm) + 1);
      }
    cwr.align16();

    cwr.popOrigin();
    cwr.endBlock();

    if (layer_idx == 0)
    {
      if (!crop_area)
        copy_file_to_stream(blk.getStr("sweepMask"), cwr.getRawWriter());
      else
        crop_rohh_bitmap(blk.getStr("sweepMask"), cwr, used_cells_bbox, (int)floorf(cellSz * blk.getReal("sweepMaskScale")));
    }
    for (int i = 0; i < mask_num; i++)
    {
      if (!crop_area)
        copy_file_to_stream(blk.getBlock(i)->getStr("mask"), cwr.getRawWriter());
      else
        crop_rohh_bitmap(blk.getBlock(i)->getStr("mask"), cwr, used_cells_bbox, ri_cell_sz);
    }
  }
  void exportGenEntities(const DataBlock &blk, BinDumpSaveCB &main_cwr, dag::ConstSpan<TmpPool> poolsList,
    dag::ConstSpan<bool> isDynamicImpostorList)
  {
    if (rendinst::tmInst12x32bit)
    {
      main_cwr.beginTaggedBlock(_MAKE4C('tm24'));
      main_cwr.endBlock();
    }
    main_cwr.beginTaggedBlock(_MAKE4C('RIGz'));
    unsigned rigen_layers_mask = 0;

    for (int layer = 0; layer < rendinst::get_rigen_data_layers(); layer++)
      if (get_layer_blk(blk, layer).getBlockByName("mask"))
        rigen_layers_mask |= 1 << layer;
      else
        for (int i = 0; i < poolsList.size(); i++)
          if (poolsList[i].entitiesPosList.size() + poolsList[i].entitiesTmList.size() > 0 && riPool.getPools()[i] &&
              riPool.getPools()[i]->layerIdx == layer)
          {
            rigen_layers_mask |= 1 << layer;
            break;
          }

    debug("rgLayers export mask=%04X", rigen_layers_mask);
    for (int layer = 0; layer < rendinst::get_rigen_data_layers() && rigen_layers_mask; layer++, rigen_layers_mask >>= 1)
    {
      BinDumpSaveCB cwr(16 << 20, main_cwr.getTarget(), main_cwr.WRITE_BE);
      BinDumpSaveCB cdata_cwr(4 << 20, main_cwr.getTarget(), main_cwr.WRITE_BE);
      exportGenEntities(cwr, cdata_cwr, get_layer_blk(blk, layer), poolsList, isDynamicImpostorList, layer);

      int main_pos = main_cwr.tell();
      main_cwr.beginBlock();
      MemoryLoadCB mcrd(cwr.getMem(), false);
      if (ShaderMeshData::preferZstdPacking && ShaderMeshData::allowOodlePacking)
      {
        main_cwr.writeInt32e(cwr.getSize());
        oodle_compress_data(main_cwr.getRawWriter(), mcrd, cwr.getSize());
      }
      else if (ShaderMeshData::preferZstdPacking)
        zstd_compress_data(main_cwr.getRawWriter(), mcrd, cwr.getSize());
      else
        lzma_compress_data(main_cwr.getRawWriter(), 9, mcrd, cwr.getSize());
      main_cwr.endBlock(ShaderMeshData::preferZstdPacking ? (ShaderMeshData::allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD)
                                                          : btag_compr::UNSPECIFIED);
      debug("rgLayer.%d: rigen.size=%d (%d compr) cdata.size=%d %s", layer + 1, cwr.getSize(), main_cwr.tell() - main_pos,
        cdata_cwr.getSize(), (rigen_layers_mask & 1) ? "" : "(empty)");
      main_cwr.beginBlock();
      cdata_cwr.copyDataTo(main_cwr.getRawWriter());
      main_cwr.endBlock();
    }

    main_cwr.align8();
    main_cwr.endBlock();
  }

  static vec3f __stdcall update_pregen_pos_y(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy)
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
  static const DataBlock &get_layer_blk(const DataBlock &blk, int layer)
  {
    return (layer == 0) ? blk : *blk.getBlockByNameEx(String(0, "layer%d", layer), blk.getBlockByNameEx(layer == 1 ? "secLayer" : ""));
  }
  static void crop_rohh_bitmap(const char *fname, mkbindump::BinDumpSaveCB &cwr, const IBBox2 &used_cells_bbox, int dot_per_cell)
  {
    FullFileLoadCB crd(fname);
    RoHugeHierBitMap2d<4, 3> *roBm = RoHugeHierBitMap2d<4, 3>::create(crd);
    int bm_w = used_cells_bbox.size().x * dot_per_cell, bm_h = used_cells_bbox.size().y * dot_per_cell;
    objgenerator::HugeBitmask bm(bm_w, bm_h);

    debug("crop_rohh_bitmap(%s, dpc=%d, crop %dx%d to (%d,%d) %dx%d", fname, dot_per_cell, roBm->getW(), roBm->getH(),
      used_cells_bbox[0].x * dot_per_cell, used_cells_bbox[0].y * dot_per_cell, bm_w, bm_h);
    for (int my = 0; my < bm_h; my++)
      for (int mx = 0; mx < bm_w; mx++)
        if (roBm->get(mx + used_cells_bbox[0].x * dot_per_cell, my + used_cells_bbox[0].y * dot_per_cell))
          bm.set(mx, my);

    mkbindump::build_ro_hier_bitmap(cwr, bm, 4, 3);
  }
};


int AcesRendInstEntity::getLodCount() { return pool->getPools()[poolIdx]->getLodCount(); }

void AcesRendInstEntity::setCurLod(int n) { force_lod_no = n; }

real AcesRendInstEntity::getLodRange(int lod_n) { return pool->getPools()[poolIdx]->getLodRange(lod_n); }

int AcesRendInstEntity::gatherTextures(TextureIdSet &out_tex_id, int for_lod)
{
  return pool->getPools()[poolIdx]->gatherTextures(out_tex_id, for_lod);
}

DagorAsset *AcesRendInstEntity::getAsset()
{
  G_ASSERT(pool);
  G_ASSERT(poolIdx < pool->getPools().size());
  return pool->getPools()[poolIdx]->getAsset();
}

int AcesRendInstEntity::getAssetNameId()
{
  G_ASSERT(pool);
  G_ASSERT(poolIdx < pool->getPools().size());
  return pool->getPools()[poolIdx]->getAssetNameId();
}

const char *AcesRendInstEntity::getResourceName()
{
  G_ASSERT(pool);
  G_ASSERT(poolIdx < pool->getPools().size());
  return pool->getPools()[poolIdx]->riResName;
}

bool AcesRendInstEntity::getRendInstQuantizedTm(TMatrix &out_tm) const
{
  G_ASSERT(pool);
  if (!pool)
    return false;

  G_ASSERT(poolIdx < pool->getPools().size());
  int layerIdx;
  int riIdx;
  rendinst::get_layer_idx_and_ri_idx_from_pregen_id(pool->getPools()[poolIdx]->pregenId, layerIdx, riIdx);

  const Point3 position = tm.getcol(3);

  rendinst::GetRendInstMatrixByRiIdxResult getResult = rendinst::get_rendinst_matrix_by_ri_idx(layerIdx, riIdx, position, out_tm);
  if (getResult == rendinst::GetRendInstMatrixByRiIdxResult::Success)
    return true;

  rendinst::QuantizeRendInstMatrixResult quantizeResult = rendinst::quantize_rendinst_matrix(layerIdx, riIdx, tm, out_tm);
  if (quantizeResult == rendinst::QuantizeRendInstMatrixResult::Success)
    return true;

  return false;
}

void AcesRendInstEntity::setTm(const TMatrix &_tm)
{
  if (memcmp(&tm, &_tm, sizeof(tm)) != 0)
    rendinst::notify_ri_moved(pool->getPools()[poolIdx]->pregenId, tm[3][0], tm[3][2], _tm[3][0], _tm[3][2]);
  tm = _tm;
  if (autoInstSeed)
    instSeed = mem_hash_fnv1((const char *)&tm.m[3][0], 12);
}
void AcesRendInstEntity::setPerInstanceSeed(int seed)
{
  if (instSeed != seed)
    rendinst::notify_ri_moved(pool->getPools()[poolIdx]->pregenId, tm[3][0], tm[3][2], tm[3][0], tm[3][2]);

  if (seed)
    instSeed = seed, autoInstSeed = false;
  else
    instSeed = mem_hash_fnv1((const char *)&tm.m[3][0], 12), autoInstSeed = true;
}
void AcesRendInstEntity::destroy()
{
  rendinst::notify_ri_deleted(pool->getPools()[poolIdx]->pregenId, tm[3][0], tm[3][2]);
  pool->delEntity(this);
  instSeed = 0;
  autoInstSeed = true;
}

RenderableInstanceLodsResource *AcesRendInstEntity::getSceneLodsRes() { return pool->getPools()[poolIdx]->res; }

const RenderableInstanceLodsResource *AcesRendInstEntity::getSceneLodsRes() const { return pool->getPools()[poolIdx]->res; }

void init_aces_rimgr_service(const DataBlock &app_blk)
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }
  rendinst::rendinstGlobalShadows = true;
  for (int i = 0; i < 255; i++)
    pow_22_tbl[i] = pow(i / 255.0, 2.2) * 255;
  colRangeSrv = EDITORCORE->queryEditorInterface<IColorRangeService>();
  G_ASSERT(colRangeSrv);

  AcesRendInstEntityManagementService *srv = new (inimem) AcesRendInstEntityManagementService;
  if (IDaEditor3Engine::get().registerService(srv))
    srv->explicitInit();
}
void init_rimgr_service(const DataBlock &app_blk) { return init_aces_rimgr_service(app_blk); }

void *get_generic_rendinstgen_service() { return rigenSrv; }

void generic_rendinstgen_service_set_callbacks(bool(__stdcall *get_height)(Point3 &pos, Point3 *out_norm),
  bool(__stdcall *trace_ray)(const Point3 &pos, const Point3 &dir, real &t, Point3 *out_norm),
  vec3f(__stdcall *update_pregen_pos_y)(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy))
{
  ri_get_height = get_height;
  ri_trace_ray = trace_ray;
  ri_update_pregen_pos_y = update_pregen_pos_y;
  if (get_height || trace_ray || update_pregen_pos_y)
    rendinst::set_rt_pregen_gather_cb(&AcesRendInstEntityManagementService::pregen_gather_pos_cb,
      &AcesRendInstEntityManagementService::pregen_gather_tm_cb, &AcesRendInstEntityManagementService::prepare_pool_mapping,
      &AcesRendInstEntityManagementService::prepare_pools);
  else
    rendinst::set_rt_pregen_gather_cb(NULL, NULL, NULL, NULL);
}

void rimgr_set_force_lod_no(int lod_no) { force_lod_no = lod_no; }
int rimgr_get_force_lod_no() { return force_lod_no; }

namespace rendinst::gen
{
float custom_max_trace_distance = 100;
bool custom_trace_ray(const Point3 &p, const Point3 &d, real &t, Point3 *n) { return ri_trace_ray ? ri_trace_ray(p, d, t, n) : false; }
bool custom_trace_ray_earth(const Point3 &p, const Point3 &d, real &t) { return ri_trace_ray ? ri_trace_ray(p, d, t, NULL) : false; }
bool custom_get_height(Point3 &pos, Point3 *out_norm)
{
  if (ri_get_height)
    return ri_get_height(pos, out_norm);
  return false;
}
vec3f custom_update_pregen_pos_y(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy)
{
  if (ri_update_pregen_pos_y)
    return ri_update_pregen_pos_y(pos, dest_packed_y, csz_y, oy);
  return pos;
}
void custom_get_land_min_max(BBox2, float &out_min, float &out_max) {}
} // namespace rendinst::gen
