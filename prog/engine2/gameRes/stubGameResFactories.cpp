#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResHooks.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_rendInstRes.h>
#include <fx/dag_baseFxClasses.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point4.h>
#include <util/dag_fastIntList.h>
#include <util/dag_oaHashNameMap.h>
#include <phys/dag_physResource.h>
#include <util/dag_oaHashNameMap.h>

namespace stubgameres
{
struct DynModel;
struct RendInst;
struct Effect;
struct PhysSys;
struct PhysObj;

template <typename T>
static auto makeStubRes() -> decltype(T::makeStubRes())
{
  return T::makeStubRes();
}

template <typename T>
static auto makeStubRes() -> decltype(new T)
{
  return new T;
}

template <typename T>
auto makeStubRes(const DataBlock *b) -> decltype(T::makeStubRes(b))
{
  return T::makeStubRes(b);
}

template <typename T>
auto makeStubRes(const DataBlock *b) -> decltype(new T(b))
{
  return new T(b);
}

template <class T>
static inline GameResource *cast(T *r)
{
  return r->gameRes();
}
template <>
GameResource *cast(RenderableInstanceLodsResource *r)
{
  return (GameResource *)r;
}
template <>
GameResource *cast(DynamicRenderableSceneLodsResource *r)
{
  return (GameResource *)r;
}

template <class T>
static inline GameResource *cast(Ptr<T> &r)
{
  return cast(r.get());
}
} // namespace stubgameres
namespace gamereshooks
{
extern void (*on_load_res_packs_from_list_complete)(const char *pack_list_blk_fname, bool load_grp, bool load_tex);
}

static constexpr int STUB_RESID_BASE = 128 << 10;
static FastIntList stub_types;
static OAHashNameMap<false> resMap;
static Tab<unsigned> resTypes;
static Tab<GameResourceFactory *> stub_gameres_factories;

static bool stub_resolve_res_handle(GameResHandle rh, unsigned class_id, int &out_res_id);
static int stub_resolve_res_id(int res_id)
{
  if (res_id >= STUB_RESID_BASE)
    return res_id;

  String nm;
  get_game_resource_name(res_id, nm);
  int id = resMap.getNameId(nm);
  return id < 0 ? res_id : id + STUB_RESID_BASE;
}


template <class T, unsigned CLS>
struct StubGameResFactory : public GameResourceFactory
{
  Ptr<T> stubRes;

  StubGameResFactory() { stubRes = stubgameres::makeStubRes<T>(); }
  virtual ~StubGameResFactory() { reset(); }

  virtual unsigned getResClassId() { return CLS; }
  virtual const char *getResClassName() { return "stub"; }

  virtual bool isResLoaded(int res_id)
  {
    G_UNREFERENCED(res_id);
    return false;
  }
  virtual bool checkResPtr(GameResource *res) { return isStubRes(res); }
  virtual GameResource *getGameResource(int res_id)
  {
    stubRes.addRef();
    dumpCreate(res_id);
    return stubgameres::cast(stubRes);
  }
  virtual bool addRefGameResource(GameResource *res) { return isStubRes(res); }
  virtual void releaseGameResource(int res_id)
  {
    G_UNREFERENCED(res_id);
    stubRes.delRef();
  }

  virtual bool releaseGameResource(GameResource *res)
  {
    if (!isStubRes(res))
      return false;
    stubRes.delRef();
    return true;
  }
  virtual bool freeUnusedResources(bool, bool) { return false; }

  void loadGameResourceData(int, IGenLoad &) override {}
  void createGameResource(int, const int *, int) override {}
  virtual void reset()
  {
    if (stubRes->getRefCount() > 1)
      debug("%08X.factory ref=%d", CLS, stubRes->getRefCount());
    while (stubRes->getRefCount() > 1)
      stubRes->delRef();
  }

  bool isStubRes(void *r) { return r == stubgameres::cast(stubRes); }

  void dumpCreate(int id)
  {
    G_UNREFERENCED(id);
    // debug("stubRes: %s:%08X created, ref=%d", resMap.getName(id-STUB_RESID_BASE), CLS, stubRes->getRefCount());
  }
};

template <class T, unsigned CLS>
struct StubGameResArrayFactory : public GameResourceFactory
{
  PtrTab<T> stubRes;
  OAHashNameMap<false> nameMap;
  const DataBlock &desc;

  StubGameResArrayFactory(const DataBlock &b) : desc(b)
  {
    stub_types.addInt(CLS);
    for (int i = 0, res_id = 0; i < desc.blockCount(); i++)
      stub_resolve_res_handle((GameResHandle)desc.getBlock(i)->getBlockName(), CLS, res_id);
  }
  virtual ~StubGameResArrayFactory() { reset(); }

  virtual unsigned getResClassId() { return CLS; }
  virtual const char *getResClassName() { return "stubArray"; }

  virtual bool isResLoaded(int res_id)
  {
    G_UNREFERENCED(res_id);
    return false;
  }
  virtual bool checkResPtr(GameResource *res) { return isStubRes(res); }
  virtual GameResource *getGameResource(int res_id)
  {
    if (T *res = findRes(res_id, true))
    {
      res->addRef();
      return stubgameres::cast(res);
    }
    return NULL;
  }
  virtual bool addRefGameResource(GameResource *res) { return isStubRes(res); }
  virtual void releaseGameResource(int res_id)
  {
    if (T *res = findRes(res_id, false))
      res->delRef();
  }
  virtual bool releaseGameResource(GameResource *res)
  {
    if (!isStubRes(res))
      return false;
    reinterpret_cast<T *>(res)->delRef();
    return true;
  }
  virtual bool freeUnusedResources(bool, bool) { return false; }

  void loadGameResourceData(int, IGenLoad &) override {}
  void createGameResource(int, const int *, int) override {}
  virtual void reset()
  {
    for (int i = 0; i < stubRes.size(); i++)
    {
      if (!stubRes[i].get())
        continue;
      if (stubRes[i]->getRefCount() > 1)
        debug("%08X.factory ref[%d]=%d", CLS, i, stubRes[i]->getRefCount());
      while (stubRes[i]->getRefCount() > 1)
        stubRes[i]->delRef();
    }
    clear_and_shrink(stubRes);
    nameMap.reset();
  }

  bool isStubRes(void *r)
  {
    for (int i = 0; i < stubRes.size(); i++)
      if (r == stubgameres::cast(stubRes[i]))
        return true;
    return false;
  }
  T *findRes(int res_id, bool can_add)
  {
    res_id = stub_resolve_res_id(res_id);
    const char *res_name = resMap.getName(res_id - STUB_RESID_BASE);
    if (!res_name)
      return NULL;
    const DataBlock *b = desc.getBlockByName(res_name);
    int id = can_add && b ? nameMap.addNameId(res_name) : nameMap.getNameId(res_name);
    if (id < 0)
      return NULL;
    if (id >= stubRes.size())
    {
      G_ASSERT(id == stubRes.size());
      stubRes.push_back(stubgameres::makeStubRes<T>(b));
      // debug("stubRes: %s:%08X created, ref=%d", res_name, CLS, stubRes[id]->getRefCount());
    }

    return stubRes[id];
  }
};

struct stubgameres::Effect : public DObject
{
  struct Fx : public BaseEffectObject
  {
    virtual BaseParamScript *clone() { return this; }
    virtual void loadParamsData(const char *, int, BaseParamScriptLoadCB *) {}
    virtual void setParam(unsigned, void *) {}
    virtual void *getParam(unsigned, void *) { return NULL; }

    virtual void update(float) {}
    virtual void drawEmitter(const Point3 &) {}
    virtual void render(unsigned) {}
  } fx;

  GameResource *gameRes() { return (GameResource *)&fx; }
};

struct stubgameres::PhysSys : public PhysicsResource
{
  GameResource *gameRes() { return (GameResource *)this; }
};

struct stubgameres::PhysObj : public DObject, public DynamicPhysObjectData
{
  GameResource *gameRes() { return (GameResource *)static_cast<DynamicPhysObjectData *>(this); }
};

static GameResourceFactory *findFactory(dag::Span<GameResourceFactory *> f, unsigned class_id)
{
  if (!class_id)
    return NULL;
  for (int i = 0; i < f.size(); i++)
    if (f[i]->getResClassId() == class_id)
      return f[i];
  return NULL;
}

static bool stub_resolve_res_handle(GameResHandle rh, unsigned class_id, int &out_res_id)
{
  if (class_id == 0)
  {
    int id = resMap.getNameId((const char *)rh);
    if (id < 0)
      return false;
    out_res_id = id + STUB_RESID_BASE;
    return true;
  }
  if (!stub_types.hasInt(class_id))
    return false;

  out_res_id = resMap.addNameId((const char *)rh);
  if (out_res_id >= resTypes.size())
  {
    G_ASSERT(out_res_id == resTypes.size());
    resTypes.push_back(class_id);
    // debug("add %s %08X as %d+STUB_RESID_BASE", (const char*)rh, class_id, out_res_id);
  }

  if (resTypes[out_res_id] != class_id)
    return false;

  out_res_id += STUB_RESID_BASE;
  return true;
}
static bool stub_validate_game_res_id(int res_id, int &out_res_id)
{
  res_id = stub_resolve_res_id(res_id);
  if (res_id < STUB_RESID_BASE) //== fallback for old resources built separately
    return false;

  out_res_id = res_id;
  return true;
}
static bool stub_get_game_res_class_id(int res_id, unsigned &out_class_id)
{
  res_id = stub_resolve_res_id(res_id);
  if (res_id < STUB_RESID_BASE) //== fallback for old resources built separately
    return false;

  out_class_id = resTypes[res_id - STUB_RESID_BASE];
  return true;
}
static bool stub_get_game_resource(int res_id, dag::Span<GameResourceFactory *> f, GameResource *&out_res)
{
  res_id = stub_resolve_res_id(res_id);
  if (res_id < STUB_RESID_BASE) //== fallback for old resources built separately
    return false;

  GameResourceFactory *fac = findFactory(f, resTypes[res_id - STUB_RESID_BASE]);
  if (!fac)
  {
    logerr("no factory (classid=%p) to load asset %s", resTypes[res_id - STUB_RESID_BASE], resMap.getName(res_id - STUB_RESID_BASE));
    out_res = NULL;
    return true;
  }

  out_res = fac->getGameResource(res_id);
  return true;
}


static void (*chained_on_load_res_packs_from_list_complete)(const char *pack_list_blk_fname, bool load_grp, bool load_tex) = 0;
static void ri_stub_on_load_res_packs(const char *pack_list_blk_fname, bool load_grp, bool load_tex)
{
  if (chained_on_load_res_packs_from_list_complete)
    chained_on_load_res_packs_from_list_complete(pack_list_blk_fname, load_grp, load_tex);
  if (!load_grp)
    return;
  debug("ri_stub_on_load_res_packs for (%s, grp=%d, tex=%d), gameres_rendinst_desc=%d", pack_list_blk_fname, load_grp, load_tex,
    gameres_rendinst_desc.blockCount());
  for (int i = 0, res_id = 0; i < gameres_rendinst_desc.blockCount(); i++)
    stub_resolve_res_handle((GameResHandle)gameres_rendinst_desc.getBlock(i)->getBlockName(), RendInstGameResClassId, res_id);
}

void register_stub_gameres_factories(dag::ConstSpan<unsigned> stubbed_types)
{
  using namespace stubgameres;
  for (int i = 0; i < stubbed_types.size(); i++)
  {
    switch (stubbed_types[i])
    {
      case DynModelGameResClassId:
        stub_gameres_factories.push_back(new StubGameResFactory<DynamicRenderableSceneLodsResource, DynModelGameResClassId>);
        break;
      case RendInstGameResClassId:
        stub_gameres_factories.push_back(
          new StubGameResArrayFactory<RenderableInstanceLodsResource, RendInstGameResClassId>(gameres_rendinst_desc));
        chained_on_load_res_packs_from_list_complete = gamereshooks::on_load_res_packs_from_list_complete;
        gamereshooks::on_load_res_packs_from_list_complete = &ri_stub_on_load_res_packs;
        break;
      case EffectGameResClassId: stub_gameres_factories.push_back(new StubGameResFactory<Effect, EffectGameResClassId>); break;
      case PhysSysGameResClassId: stub_gameres_factories.push_back(new StubGameResFactory<PhysSys, PhysSysGameResClassId>); break;
      case PhysObjGameResClassId: stub_gameres_factories.push_back(new StubGameResFactory<PhysObj, PhysObjGameResClassId>); break;
      // case CharacterGameResClassId:    break;
      // case FastPhysDataGameResClassId: break;
      // case Anim2DataGameResClassId:    break;
      // case MaterialGameResClassId:     break;
      default: logerr("register_stub_gameres_factories: skipping unsupported type=%08X", stubbed_types[i]); continue;
    }
    stub_types.addInt(stubbed_types[i]);
  }

  if (stub_types.empty())
    return;

  for (int i = 0; i < stub_gameres_factories.size(); i++)
    ::add_factory(stub_gameres_factories[i]);

  gamereshooks::resolve_res_handle = stub_resolve_res_handle;
  gamereshooks::on_validate_game_res_id = stub_validate_game_res_id;
  gamereshooks::on_get_game_res_class_id = stub_get_game_res_class_id;
  gamereshooks::on_get_game_resource = stub_get_game_resource;
}

void terminate_stub_gameres_factories()
{
  gamereshooks::resolve_res_handle = NULL;
  gamereshooks::on_validate_game_res_id = NULL;
  gamereshooks::on_get_game_res_class_id = NULL;
  gamereshooks::on_get_game_resource = NULL;
  if (gamereshooks::on_load_res_packs_from_list_complete == &ri_stub_on_load_res_packs)
    gamereshooks::on_load_res_packs_from_list_complete = chained_on_load_res_packs_from_list_complete;
  chained_on_load_res_packs_from_list_complete = NULL;

  clear_all_ptr_items_and_shrink(stub_gameres_factories);
  stub_types.reset(true);
  resMap.reset(true);
  clear_and_shrink(resTypes);
}
