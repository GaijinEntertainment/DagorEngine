#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_writeObjsToPlaceDump.h>
#include <de3_entityUserData.h>
#include <de3_groundHole.h>
#include <oldEditor/de_common_interface.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <math/dag_capsule.h>

static int collisionSubtypeMask = -1;
static int gameobjEntityClassId = -1;
static int rendEntGeomMask = -1;
static int rendEntGeom = -1;
static bool registeredNotifier = false;
static OAHashNameMap<true> gameObjNames;

class GameObjEntity;
typedef SingleEntityPool<GameObjEntity> GameObjEntityPool;

class VirtualGameObjEntity : public IObjEntity, public IObjEntityUserDataHolder
{
public:
  VirtualGameObjEntity(int cls) : IObjEntity(cls), pool(NULL), nameId(-1), userDataBlk(NULL)
  {
    assetNameId = -1;
    bbox = BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
    ladderStepsCount = -1;
    isGroundHole = false;
    bsph.setempty();
    bsph = bbox;
    volType = VT_BOX;
    needsSuperEntRef = false;
    visEntity = NULL;
  }
  virtual ~VirtualGameObjEntity()
  {
    clear();
    assetNameId = -1;
  }

  void clear()
  {
    del_it(userDataBlk);
    destroy_it(visEntity);
  }

  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }
  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityUserDataHolder);
    return NULL;
  }

  virtual BSphere3 getBsph() const { return bsph; }
  virtual BBox3 getBbox() const { return bbox; }

  virtual const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:gameObj", gameObjNames.getName(nameId));
    return buf;
  }

  void setup(const DagorAsset &asset, bool virtual_ent)
  {
    const char *name = asset.getName();
    assetNameId = asset.getNameId();

    nameId = gameObjNames.addNameId(name);
    needsSuperEntRef = asset.props.getBool("needsSuperEntRef", false);
    const char *type = asset.props.getStr("volumeType", "box");
    volType = VT_BOX;
    if (strcmp(type, "box") == 0)
      volType = VT_BOX;
    else if (strcmp(type, "sphere") == 0)
      volType = VT_SPH;
    else if (strcmp(type, "point") == 0)
      volType = VT_PT;
    else
      DAEDITOR3.conError("%s: unrecognized volumeType:t=\"%s\"", getObjAssetName(), type);

    float hsz = asset.props.getReal("boxSz", 1);
    bbox[0].set(-hsz, -hsz, -hsz);
    bbox[1].set(hsz, hsz, hsz);

    ladderStepsCount = asset.props.getInt("ladderStepsCount", -1);
    isGroundHole = asset.props.getBool("isGroundHole", false);

    bsph = bbox;
    if (volType == VT_SPH)
      bsph.r = hsz, bsph.r2 = hsz * hsz;
    const char *ref_dm = asset.props.getStr("ref_dynmodel", NULL);
    if (DagorAsset *a = ref_dm && *ref_dm ? DAEDITOR3.getAssetByName(ref_dm) : NULL)
      visEntity = a ? DAEDITOR3.createEntity(*a, virtual_ent) : NULL;
    if (ref_dm && *ref_dm && !visEntity)
      DAEDITOR3.conError("%s: failed to create ref dynmodel=%s", asset.getName(), ref_dm);
    if (visEntity)
      visEntity->setSubtype(rendEntGeom);
  }

  bool isNonVirtual() const { return pool; }
  int getAssetNameId() const { return assetNameId; }


  // IObjEntityUserDataHolder
  virtual DataBlock *getUserDataBlock(bool create_if_not_exist)
  {
    if (!userDataBlk && create_if_not_exist)
      userDataBlk = new DataBlock;
    return userDataBlk;
  }
  virtual void resetUserDataBlock() { del_it(userDataBlk); }
  virtual void setSuperEntityRef(const char *ref)
  {
    if (needsSuperEntRef)
      getUserDataBlock(true)->setStr("ref", ref);
  }

public:
  DataBlock *userDataBlk;
  GameObjEntityPool *pool;
  BBox3 bbox;
  int ladderStepsCount;
  bool isGroundHole;
  BSphere3 bsph;
  enum
  {
    VT_BOX,
    VT_SPH,
    VT_PT
  };
  int16_t volType;
  bool needsSuperEntRef;
  int nameId;
  int assetNameId;
  IObjEntity *visEntity;
};

class GameObjEntity : public VirtualGameObjEntity
{
public:
  GameObjEntity(int cls) : VirtualGameObjEntity(cls), idx(MAX_ENTITIES) { tm.identity(); }
  ~GameObjEntity() { clear(); }

  void clear() { VirtualGameObjEntity::clear(); }

  virtual void setTm(const TMatrix &_tm)
  {
    tm = _tm;
    if (visEntity)
      visEntity->setTm(_tm);
  }
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy()
  {
    pool->delEntity(this);
    clear();
  }

  void copyFrom(const GameObjEntity &e)
  {
    bbox = e.bbox;
    bsph = e.bsph;
    ladderStepsCount = e.ladderStepsCount;
    isGroundHole = e.isGroundHole;
    volType = e.volType;
    needsSuperEntRef = e.needsSuperEntRef;

    nameId = e.nameId;
    assetNameId = e.assetNameId;
    visEntity = e.visEntity ? DAEDITOR3.cloneEntity(e.visEntity) : NULL;
    if (visEntity)
      visEntity->setSubtype(rendEntGeom);
  }

  void renderObj()
  {
    set_cached_debug_lines_wtm(tm);

    const E3DCOLOR color(0, 255, 0);

    if (volType == VT_BOX)
    {
      draw_cached_debug_box(bbox, color);
      Point3 leftPt((bbox[0].x + bbox[1].x) * 0.5f, 0.f, bbox[0].z);
      Point3 rightPt((bbox[0].x + bbox[1].x) * 0.5f, 0.f, bbox[1].z);
      for (int i = 0; i < ladderStepsCount; ++i)
      {
        float ht = lerp(bbox[0].y, bbox[1].y, float(i) / float(ladderStepsCount - 1));
        draw_cached_debug_line(Point3::xVz(leftPt, ht), Point3::xVz(rightPt, ht), color);
      }
    }
    else if (volType == VT_SPH)
      draw_cached_debug_sphere(bsph.c, bsph.r, color);
    else if (volType == VT_PT)
    {
      draw_cached_debug_line(Point3(bbox[0].x, 0, 0), Point3(bbox[1].x, 0, 0), color);
      draw_cached_debug_line(Point3(0, bbox[0].y, 0), Point3(0, bbox[1].y, 0), color);
      draw_cached_debug_line(Point3(0, 0, bbox[0].z), Point3(0, 0, bbox[1].z), color);
    }
  }

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  TMatrix tm;
};


class GameObjEntityManagementService : public IEditorService,
                                       public IObjEntityMgr,
                                       public IBinaryDataBuilder,
                                       public IDagorAssetChangeNotify,
                                       public IGatherGroundHoles,
                                       public IGatherGameLadders
{
public:
  GameObjEntityManagementService()
  {
    gameobjEntityClassId = IDaEditor3Engine::get().getAssetTypeId("gameObj");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    rendEntGeom = IDaEditor3Engine::get().registerEntitySubTypeId("single_ent" /*"rend_ent_geom"*/);
    visible = true;
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_goEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Game object entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    begin_draw_cached_debug_lines();
    int subtypeMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(subtypeMask, lh_mask))
        ent[i]->renderObj();
    end_draw_cached_debug_lines();
  }
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, IGatherGroundHoles);
    RETURN_INTERFACE(huid, IGatherGameLadders);
    return NULL;
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return gameobjEntityClassId >= 0 && gameobjEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (!registeredNotifier)
    {
      registeredNotifier = true;
      asset.getMgr().subscribeUpdateNotify(this, -1, gameobjEntityClassId);
    }

    if (virtual_ent)
    {
      VirtualGameObjEntity *ent = new VirtualGameObjEntity(gameobjEntityClassId);
      ent->setup(asset, true);
      return ent;
    }

    GameObjEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new GameObjEntity(gameobjEntityClassId);

    ent->setup(asset, false);
    objPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    GameObjEntity *o = reinterpret_cast<GameObjEntity *>(origin);
    GameObjEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new GameObjEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    objPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
        ent[i]->clear();
    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
      {
        ent[i]->clear();
        ent[i]->setup(asset, !ent[i]->isNonVirtual());
      }
    EDITORCORE->invalidateViewportCache();
  }

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &log, PropPanel2 *params)
  {
    if (!objPool.calcEntities(IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT)))
      log.addMessage(log.WARNING, "No gameObj entities for export");
    return true;
  }

  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }

  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *)
  {
    dag::Vector<SrcObjsToPlace> objs;

    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        int k;
        for (k = 0; k < objs.size(); k++)
          if (ent[i]->nameId == objs[k].nameId)
            break;
        if (k == objs.size())
        {
          objs.push_back();
          objs[k].nameId = ent[i]->nameId;
          objs[k].resName = gameObjNames.getName(ent[i]->nameId);
        }

        objs[k].tm.push_back(ent[i]->tm);
        if (ent[i]->userDataBlk)
          objs[k].addBlk.addNewBlock(ent[i]->userDataBlk, "");
        else
          objs[k].addBlk.addNewBlock("");
      }

    for (int k = 0; k < objs.size(); k++)
      if (DagorAsset *a = DAEDITOR3.getAssetByName(objs[k].resName, gameobjEntityClassId))
        objs[k].addBlk.addNewBlock(&a->props, "__asset");

    writeObjsToPlaceDump(cwr, make_span(objs), _MAKE4C('GmO'));
    return true;
  }

  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    int cnt = objPool.calcEntities(subtype_mask);
    int maxCount = metrics_blk.getInt("max_entities", 0);

    if (cnt > maxCount)
    {
      DAEDITOR3.conError("Metrics validation failed: GameObj count=%d  > maximum allowed=%d.", cnt, maxCount);
      return false;
    }
    return true;
  }

  // IGatherGroundHoles interface
  virtual void gatherGroundHoles(Tab<GroundHole> &obstacles)
  {
    dag::ConstSpan<GameObjEntity *> entities = objPool.getEntities();
    for (GameObjEntity *entity : entities)
      if (entity && entity->isGroundHole &&
          (entity->volType == VirtualGameObjEntity::VT_BOX || entity->volType == VirtualGameObjEntity::VT_SPH))
      {
        TMatrix tm;
        entity->getTm(tm);
        const bool isSphere = entity->volType == VirtualGameObjEntity::VT_SPH;
        const Point2 center = Point2::xz(tm.getcol(3));
        const Point2 xAxis = Point2::xz(tm.getcol(0));
        const Point2 yAxis = Point2::xz(tm.getcol(2));
        obstacles.push_back(GroundHole(xAxis, yAxis, center, isSphere));
      }
  }

  // IGatherGameLadders
  virtual void gatherGameLadders(Tab<GameLadder> &ladders)
  {
    int numLadders = 0;
    dag::ConstSpan<GameObjEntity *> entities = objPool.getEntities();
    for (GameObjEntity *entity : entities)
    {
      if (entity && entity->ladderStepsCount > 0 && entity->volType == VirtualGameObjEntity::VT_BOX)
        ++numLadders;
    }
    ladders.reserve(ladders.size() + numLadders);
    for (GameObjEntity *entity : entities)
    {
      if (entity && entity->ladderStepsCount > 0 && entity->volType == VirtualGameObjEntity::VT_BOX)
      {
        TMatrix tm;
        entity->getTm(tm);
        ladders.push_back(GameLadder(tm, entity->ladderStepsCount));
      }
    }
  }

protected:
  GameObjEntityPool objPool;
  bool visible;
};


void init_gameobj_mgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  IDaEditor3Engine::get().registerService(new (inimem) GameObjEntityManagementService);
}
