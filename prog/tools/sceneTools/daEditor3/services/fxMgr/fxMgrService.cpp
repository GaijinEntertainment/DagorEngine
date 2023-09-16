#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_writeObjsToPlaceDump.h>
#include <de3_entityUserData.h>
#include <de3_editorEvents.h>
#include <oldEditor/de_common_interface.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <fx/dag_baseFxClasses.h>
#include <fx/dag_commonFx.h>
#include <fx/dag_fxInterface.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>
#include <3d/dag_resourcePool.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <EASTL/unique_ptr.h>
#include <dafxToolsHelper/dafxToolsHelper.h>

static bool dafx_enabled = false, haze_enabled = false;
dafx::ContextId g_dafx_ctx;
dafx::CullingId g_dafx_cull;
dafx::Stats g_dafx_stats;
float g_dafx_dt_mul = 1.f;

static int fxEntityClassId = -1;
static int rendEntGeomMask = -1;
static IEffectRayTracer *fxRayTracer = NULL;
static OAHashNameMap<true> fxNames;

class FxEntity;
typedef SingleEntityPool<FxEntity> FxEntityPool;

class VirtualFxEntity : public IObjEntity
{
public:
  VirtualFxEntity(int cls) : IObjEntity(cls), pool(NULL), fx(NULL), nameId(-1) {}
  virtual ~VirtualFxEntity() { del_it(fx); }

  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }
  virtual void *queryInterfacePtr(unsigned huid) { return NULL; }

  virtual BSphere3 getBsph() const { return BSphere3(Point3(0, 0, 0), 1.0); }
  virtual BBox3 getBbox() const { return BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5)); }

  virtual const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:fx", fxNames.getName(nameId));
    return buf;
  }

  void setup(const DagorAsset &asset)
  {
    const char *name = asset.getName();
    if (::get_resource_type_id(name) != EffectGameResClassId)
    {
      fx = NULL;
      DAEDITOR3.conError("resource is not fx: %s", name);
      return;
    }
    GameResource *res = ::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), EffectGameResClassId);
    if (res)
    {
      nameId = fxNames.addNameId(name);
      fx = (BaseEffectObject *)((BaseEffectObject *)res)->clone();
      if (fx)
        fx->setParam(HUID_RAYTRACER, fxRayTracer);
      ::release_game_resource(res);

      if (dafx_enabled)
        set_up_dafx_effect(g_dafx_ctx, fx, false);
    }
  }

  bool isNonVirtual() const { return pool; }

public:
  FxEntityPool *pool;
  BaseEffectObject *fx;
  int nameId;
};

class FxEntity : public VirtualFxEntity, public IObjEntityUserDataHolder
{
public:
  FxEntity(int cls) : VirtualFxEntity(cls), idx(MAX_ENTITIES), userDataBlk(NULL) { tm.identity(); }
  ~FxEntity() { clear(); }

  virtual void setTm(const TMatrix &_tm)
  {
    tm = _tm;
    if (!fx)
      return;

    TMatrix ident = TMatrix::IDENT;
    fx->setParam(HUID_EMITTER_TM, &ident);
    fx->setParam(HUID_TM, &tm);
    Point4 p4 = Point4::xyz0(tm.getcol(3));
    fx->setParam(_MAKE4C('PFXP'), &p4);

    /*== this code is designed for emitter-editing mode
    TMatrix fm, em;

    fm.setcol(0, Point3(length(tm.getcol(0)), 0, 0));
    fm.setcol(1, Point3(0, length(tm.getcol(1)), 0));
    fm.setcol(2, Point3(0, 0, length(tm.getcol(2))));
    fm.setcol(3, tm.getcol(3));
    fx->setParam(HUID_TM, &fm);

    em.setcol(0, normalize(tm.getcol(0)));
    em.setcol(1, normalize(tm.getcol(1)));
    em.setcol(2, normalize(tm.getcol(2)));
    em.setcol(3, Point3(0, 0, 0));

    fx->setParam((unsigned)HUID_EMITTER_TM, &em);
    */
  }
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy()
  {
    pool->delEntity(this);
    clear();
  }
  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityUserDataHolder);
    return NULL;
  }

  // IObjEntityUserDataHolder
  virtual DataBlock *getUserDataBlock(bool create_if_not_exist)
  {
    if (!userDataBlk && create_if_not_exist)
      userDataBlk = new DataBlock;
    return userDataBlk;
  }
  virtual void resetUserDataBlock() { del_it(userDataBlk); }

  void copyFrom(const VirtualFxEntity &e)
  {
    nameId = e.nameId;
    fx = e.fx ? (BaseEffectObject *)e.fx->clone() : NULL;
    if (fx)
    {
      fx->setParam(HUID_RAYTRACER, fxRayTracer);
      set_up_dafx_effect(g_dafx_ctx, fx, false);
    }
  }
  void clear()
  {
    del_it(fx);
    del_it(userDataBlk);
  }

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  TMatrix tm;
  DataBlock *userDataBlk;
};


class FxEntityManagementService : public IEditorService,
                                  public IObjEntityMgr,
                                  public IRenderingService,
                                  public IBinaryDataBuilder,
                                  public ILevelResListBuilder
{
public:
  FxEntityManagementService()
  {
    fxEntityClassId = IDaEditor3Engine::get().getAssetTypeId("fx");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");

    visible = true;

    if (dafx_enabled)
      set_up_dafx_context(g_dafx_ctx, g_dafx_cull);
    if (!g_dafx_ctx)
      dafx_enabled = false;
  }

  ~FxEntityManagementService() { EffectsInterface::shutdown(); }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_fxEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Effect entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt)
  {
    dag::ConstSpan<FxEntity *> ent = fxPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
        ent[i]->fx->update(dt * g_dafx_dt_mul);

    if (dafx_enabled)
      act_dafx(g_dafx_ctx, g_dafx_cull, g_dafx_stats, dt * g_dafx_dt_mul, globtmPrev);
  }
  virtual void beforeRenderService() {}
  virtual void renderService() {}
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      DAEDITOR3.conNote("FxMgr: %d entities", fxPool.getUsedEntityCount());
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, ILevelResListBuilder);
    return NULL;
  }

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if ((st_mask & rendEntGeomMask) != rendEntGeomMask)
      return;

    dag::ConstSpan<FxEntity *> ent = fxPool.getEntities();

    switch (stage)
    {
      case STG_BEFORE_RENDER:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->fx->render(FX_RENDER_BEFORE);
        break;

      case STG_RENDER_DYNAMIC_OPAQUE:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->fx->render(FX_RENDER_SOLID);
        break;

      case STG_RENDER_FX:
        if (dafx_enabled)
        {
          dafx::render(g_dafx_ctx, g_dafx_cull, "lowres");
          dafx::render(g_dafx_ctx, g_dafx_cull, "highres");
          dafx::render(g_dafx_ctx, g_dafx_cull, "underwater");
        }

        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->fx->render(FX_RENDER_TRANS);

        if (dafx_enabled)
          calc_dafx_stats(g_dafx_ctx, g_dafx_stats);

        break;

      case STG_RENDER_FX_DISTORTION:
        if (dafx_enabled)
          dafx::render(g_dafx_ctx, g_dafx_cull, "distortion");

        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->fx->render(FX_RENDER_DISTORTION);

        if (dafx_enabled)
          calc_dafx_stats(g_dafx_ctx, g_dafx_stats);

        break;
    }
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const { return fxEntityClassId >= 0 && fxEntityClassId == entity_class; }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (virtual_ent)
    {
      VirtualFxEntity *ent = new VirtualFxEntity(fxEntityClassId);
      ent->setup(asset);
      return ent;
    }

    FxEntity *ent = fxPool.allocEntity();
    if (!ent)
      ent = new FxEntity(fxEntityClassId);

    ent->setup(asset);
    fxPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    VirtualFxEntity *o = reinterpret_cast<VirtualFxEntity *>(origin);
    FxEntity *ent = fxPool.allocEntity();
    if (!ent)
      ent = new FxEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    fxPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &log, PropPanel2 *params)
  {
    if (!fxPool.calcEntities(IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT)))
      log.addMessage(log.WARNING, "No fx entities for export");
    return true;
  }

  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }

  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *)
  {
    dag::Vector<SrcObjsToPlace> objs;

    dag::ConstSpan<FxEntity *> ent = fxPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        int k;
        for (k = 0; k < objs.size(); k++)
          if (ent[i]->nameId == objs[k].nameId)
            break;
        if (k == objs.size())
        {
          objs.push_back();
          objs[k].nameId = ent[i]->nameId;
          objs[k].resName = fxNames.getName(ent[i]->nameId);
        }
        objs[k].tm.push_back(ent[i]->tm);
        if (ent[i]->userDataBlk)
          objs[k].addBlk.addNewBlock(ent[i]->userDataBlk, "");
        else
          objs[k].addBlk.addNewBlock("");
      }

    writeObjsToPlaceDump(cwr, make_span(objs), _MAKE4C('FX'));
    return true;
  }

  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    int cnt = fxPool.calcEntities(subtype_mask);
    int maxCount = metrics_blk.getInt("max_entities", 0);

    if (cnt > maxCount)
    {
      DAEDITOR3.conError("Metrics validation failed: fx count=%d  > maximum allowed=%d.", cnt, maxCount);
      return false;
    }
    return true;
  }

  // ILevelResListBuilder
  virtual void addUsedResNames(OAHashNameMap<true> &res_list)
  {
    dag::ConstSpan<FxEntity *> ent = fxPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    SmallTab<bool, TmpmemAlloc> used_mark;
    clear_and_resize(used_mark, fxNames.nameCount());
    mem_set_0(used_mark);

    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        if (used_mark[ent[i]->nameId])
          continue;
        used_mark[ent[i]->nameId] = 1;
        res_list.addNameId(fxNames.getName(ent[i]->nameId));
      }
  }

protected:
  FxEntityPool fxPool;
  bool visible;

  TMatrix4 globtmPrev = TMatrix4::IDENT;
};

void init_fxmgr_service(const DataBlock &app_blk)
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  const DataBlock &fxBlk = *app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("fx");
  dafx_enabled = fxBlk.getBool("dafx_enabled", true);
  haze_enabled = fxBlk.getBool("haze_enabled", true);

  if (!IDaEditor3Engine::get().registerService(new (inimem) FxEntityManagementService))
    return;

  ::register_effect_gameres_factory();
  ::register_all_common_fx_factories();
  EffectsInterface::startup();
}