// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <phys/dag_physResource.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/utility.h>
#include <debug/dag_log.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>

// Note: out-of line due to forward declaration of `DynamicRenderableSceneLodsResource`
DynamicPhysObjectData::DynamicPhysObjectData() = default;
DynamicPhysObjectData::~DynamicPhysObjectData() = default;

class PhysObjGameResFactory final : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId;
    int modelCount = 0;
    Ptr<PhysicsResource> resource;
    ResData(int rid = -1) : resId(rid) {}
  };

  struct GameRes //-V730
  {
    int resId;
    int refCount = 0;
    eastl::unique_ptr<DynamicPhysObjectData> data;
  };

  Tab<ResData> resData;
  Tab<GameRes> gameRes;


  int findResData(int res_id) const
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findRes(int res_id) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].resId == res_id)
        return i;

    return -1;
  }


  int findRes(GameResource *res) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].data.get() == (DynamicPhysObjectData *)res)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return PhysObjGameResClassId; }

  const char *getResClassName() override { return "PhysObj"; }

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findRes(res) >= 0; }

  GameResource *getGameResource(RRL rrl, int res_id) override
  {
    WinAutoLock lock(get_gameres_main_cs());
    int id = findRes(res_id);
    if (id < 0)
    {
      load_game_resource_pack_gameres_main_cs_locked(res_id, rrl);
      id = findRes(res_id);
    }
    if (id < 0)
      return NULL;

    addRef(id);
    return (GameResource *)gameRes[id].data.get();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findRes(resource);
    if (id < 0)
      return false;

    addRef(id);
    return true;
  }


  void addRef(int id)
  {
    if (auto *data = gameRes[id].data.get())
      for (auto &m : data->models)
        if (m)
          m->addInstanceRef();
    gameRes[id].refCount++;
  }
  void delRef(int id)
  {
    if (id < 0)
      return;

    if (gameRes[id].refCount == 0)
    {
      String name;
      get_game_resource_name(gameRes[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "PhysObj", name.str());
      return;
    }

    gameRes[id].refCount--;
    if (auto *data = gameRes[id].data.get())
      for (auto &m : data->models)
        if (m)
          m->delInstanceRef();
  }


  void releaseGameResource(int res_id) override
  {
    int id = findRes(res_id);
    delRef(id);
  }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findRes(resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(RRL rrl, bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = gameRes.size() - 1; i >= 0; --i)
    {
      if (rrl && is_res_required(rrl, gameRes[i].resId))
        continue;
      if (gameRes[i].refCount != 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(gameRes[i].resId, gameRes[i].refCount);
      }

      if (gameRes[i].data->nodeTree)
        ::release_game_resource_ex(gameRes[i].data->nodeTree, GeomNodeTreeGameResClassId);

      erase_items(gameRes, i, 1);
      result = true;
      if (once)
        break;
    }

    for (int j = resData.size() - 1; j >= 0; j--)
      if (resData[j].resource->getRefCount() == 1)
      {
        erase_items(resData, j, 1);
        result = true;
      }

    return result;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findResData(res_id) >= 0)
        return;
    }

    ResData rrd(res_id);

    int label = cb.readInt();
    if (label == _MAKE4C('pho0'))
    {
      const int version = 1;
      if (cb.readInt() == version)
        rrd.modelCount = cb.readInt();
      else
        logerr("Invalid version PhysObj. Need rebuild resources");
    }
    else if (label == _MAKE4C('po1s'))
    {
      rrd.modelCount = 1;
      rrd.resource = PhysicsResource::loadResource(cb, 0);
#if _TARGET_PC || DAGOR_DBGLEVEL > 0
      {
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
        static const float maxPhysObjDens = dgs_get_settings()->getReal("maxPhysObjDensity", 1e4f);
        int dll = (maxPhysObjDens != 0) ? (maxPhysObjDens > 0.f ? LOGLEVEL_WARN : LOGLEVEL_ERR) : -1;
        auto calcBodyVolume = [](const PhysicsResource::Body &b, int &cm) {
          float vol = 0.f;
          for (auto &c : b.sphColl)
          {
            vol += 4.f / 3.f * PI * sqr(c.radius) * c.radius;
            cm |= 1;
          }
          for (auto &c : b.boxColl)
          {
            vol += c.size.x * c.size.y * c.size.z;
            cm |= 2;
          }
          for (auto &c : b.capColl)
          {
            vol += PI * sqr(c.radius) * (length(c.extent) + 4.f / 3.f * c.radius);
            cm |= 4;
          }
          return vol > 0.f ? vol : /*vol of 1mm cube*/ 1e-9f;
        };

#endif
        String resname;
        auto getResName = [&]() {
          if (resname.empty())
            get_game_resource_name(res_id, resname);
          return resname.c_str();
        };
        for (const auto &bodies = rrd.resource->getBodies(); auto &b : bodies)
        {
          if (vec3f vmj = v_perm_xyzz(v_ldu(&b.momj.x)); !v_test_all_bits_zeros(v_cmp_lt(vmj, v_zero())))
          {
            Point3 size =
              b.boxColl.size() == 1 ? b.boxColl[0].size : (b.sphColl.size() == 1 ? Point3(b.sphColl[0].radius, 0, 0) : Point3{});
            int ll = (b.mass < 0.f && dgs_get_settings()->getBool("physObjAllowNegativeMass", false)) ? LOGLEVEL_WARN : LOGLEVEL_ERR;
            logmessage(ll,
              "<%s> body[%d]=%s has negative moment of inertia (%g,%g,%g) mass=%g size=(%g,%g,%g) # box/sph/cap=%d/%d/%d; %s",
              getResName(), &b - bodies.data(), b.name.c_str(), P3D(b.momj), b.mass, P3D(size), b.boxColl.size(), b.sphColl.size(),
              b.capColl.size(),
              b.mass >= 0 ? "re-export physObj gameres" : "run with \"physObjAllowNegativeMass:b=no\" to disable this logerr");
          }
#if DAGOR_DBGLEVEL > 0
          for (auto &bc : b.boxColl)
            if (vec3f vbsz = v_perm_xyzz(v_ldu(&bc.size.x)); !v_test_all_bits_zeros(v_cmp_lt(vbsz, v_zero())))
              logerr("<%s> body[%d]=%s box[%d] has negative size (%g,%g,%g)!", getResName(), &b - bodies.data(), b.name.c_str(),
                &bc - b.boxColl.data(), P3D(bc.size));
#endif
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
          int cm = 0;
          double vol = (dll >= 0) ? calcBodyVolume(b, cm) : 0;
          if (cm)
            if (double dens = b.mass / vol; dens > fabsf(maxPhysObjDens))
              logmessage(dll, "<%s> body[%d]=%s has density of %g > maxPhysObjDens (%g); mass=%g volume=%g collTypeMask=%d",
                getResName(), &b - bodies.data(), b.name.c_str(), dens, fabsf(maxPhysObjDens), b.mass, vol, cm);
#endif
        }
      }
#endif
    }

    WinAutoLock lock(cs);
    resData.push_back() = eastl::move(rrd);
  }


  void createGameResource(RRL rrl, int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    PhysicsResource *resource;
    int modelCount;
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;
      int rid = findResData(res_id);
      if (rid < 0)
      {
        String name;
        get_game_resource_name(res_id, name);
        logerr("BUG: no PhysObj resource %s", name.str());
        return;
      }

      resource = resData[rid].resource.get();
      if ((num_refs < 2 && !resource) || (resource && num_refs < 1))
      {
        gameRes.push_back().resId = res_id;
        return;
      }
      modelCount = resData[rid].modelCount;
    }

    DynamicPhysObjectData *data = new DynamicPhysObjectData;

    for (int index = 0; index < modelCount; index++)
    {
      auto m = (DynamicRenderableSceneLodsResource *)get_game_resource(ref_ids[index], rrl);
      data->models.push_back(m);
      if (m)
        ::release_game_resource_ex(m, DynModelGameResClassId);
    }

    if (resource)
    {
      data->physRes = resource;
      data->nodeTree = (num_refs < modelCount + 1) ? NULL : (GeomNodeTree *)get_game_resource(ref_ids[modelCount], rrl);
    }
    else
    {
      data->physRes = (PhysicsResource *)get_game_resource(ref_ids[modelCount], rrl);
      if (data->physRes)
        ::release_game_resource_ex(data->physRes, PhysSysGameResClassId);
      data->nodeTree = (GeomNodeTree *)get_game_resource(ref_ids[modelCount + 1], rrl);
    }

    WinAutoLock lock2(cs);
    GameRes &rd = gameRes.push_back();
    rd.resId = res_id;
    rd.data.reset(data);
  }

  void reset() override
  {
    if (!gameRes.empty())
    {
      logerr("%d leaked PhysObjs", gameRes.size());
      for (unsigned int resNo = 0; resNo < gameRes.size(); resNo++)
      {
        String name;
        get_game_resource_name(gameRes[resNo].resId, name);
        logerr("    '%s', refCount=%d", name.str(), gameRes[resNo].refCount);
      }
    }

    resData.clear();
    gameRes.clear();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameRes, resId, refCount)
};

static InitOnDemand<PhysObjGameResFactory> phys_obj_factory;

void register_phys_obj_gameres_factory()
{
  phys_obj_factory.demandInit();
  ::add_factory(phys_obj_factory);
}
