#include <de3_interface.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_editorEvents.h>
#include <de3_composit.h>
#include <de3_waterSrv.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <de3_drawInvalidEntities.h>
#include <de3_randomSeed.h>
#include <de3_baseInterfaces.h>
#include <de3_colorRangeService.h>
#include <de3_genObjUtil.h>
#include <de3_splineGenSrv.h>
#include <EditorCore/ec_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <math/dag_math3d.h>
#include <util/dag_simpleString.h>
#include <util/dag_hash.h>
#include <debug/dag_debug.h>
#include <math/random/dag_random.h>
#include <generic/dag_relocatableFixedVector.h>

static int compositEntityClassId = -1;

class CompositEntity;
class CompositEntityPool;
typedef MultiEntityPool<CompositEntity, CompositEntityPool> MultiCompositEntityPool;
typedef VirtualMpEntity<MultiCompositEntityPool> VirtualCompositEntity;

static FastNameMapEx labelId, refAssetNames;

static int getAssetNid(const char *nm)
{
  if (!nm || !*nm)
    return -1;
  if (nm[0] == '*' && !nm[1])
    return -2;
  return refAssetNames.addNameId(nm);
}

struct BoundsData
{
  __forceinline const BSphere3 getBsph() { return BSphere3(sc, sr); }
  __forceinline void setBsph(const BSphere3 &new_sphr)
  {
    sc = new_sphr.c;
    sr = new_sphr.r;
  }

public:
  BBox3 bbox;

protected:
  Point3 sc;
  real sr;
};

struct BoundsPool
{
  BoundsPool() : data(midmem), uuDataIdx(midmem) {}

  inline int add()
  {
    if (!uuDataIdx.size())
      return append_items(data, 1);

    const int id = uuDataIdx.back();
    uuDataIdx.pop_back();
    return id;
  }

  inline void del(int id)
  {
    G_ASSERT(id >= 0 && id < data.size());
    uuDataIdx.push_back(id);
  }

  inline BoundsData &operator[](int i) { return data[i]; }
  int inUse() { return data.size() - uuDataIdx.size(); }

protected:
  Tab<BoundsData> data;
  Tab<int> uuDataIdx;
};

static BoundsPool boxCache;


class CompositEntity : public VirtualCompositEntity,
                       public IRandomSeedHolder,
                       public IColor,
                       public ICompositObj,
                       public IDataBlockIdHolder
{
public:
  CompositEntity(int cls) : VirtualCompositEntity(cls)
  {
    gizmoEnabled = 0;
    boxId = -2;
    rndSeed = 123;
    autoRndSeed = true;
    tm.identity();
  }

  virtual void setTm(const TMatrix &_tm);
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy()
  {
    pool->delEntity(this);
    instSeed = 0;
    rndSeed = 123;
    autoRndSeed = true;
    tm.identity();
  }
  virtual void setSubtype(int t);
  virtual void setEditLayerIdx(int t);
  void setGizmoTranformMode(bool enabled) override;

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    RETURN_INTERFACE(huid, ICompositObj);
    if (huid == IColor::HUID && supportsColor())
      RETURN_INTERFACE(huid, IColor);
    RETURN_INTERFACE(huid, IDataBlockIdHolder);
    return NULL;
  }

  // ICompositObj interface
  virtual int getCompositSubEntityCount();
  virtual IObjEntity *getCompositSubEntity(int idx);
  virtual const ICompositObj::Props &getCompositSubEntityProps(int idx);
  virtual int getCompositSubEntityIdxByLabel(const char *label);
  void setCompositPlaceTypeOverride(int placeType) override;
  int getCompositPlaceTypeOverride() override { return placeTypeOverride; }

  // IRandomSeedHolder interface
  virtual void setSeed(int new_seed);
  virtual int getSeed() { return rndSeed; }
  virtual void setPerInstanceSeed(int seed);
  virtual int getPerInstanceSeed() { return instSeed; }

  // IColor interface
  virtual void setColor(unsigned int in_color);
  virtual unsigned int getColor() { return 0x00000000; }

  // IDataBlockIdHolder
  virtual void setDataBlockId(unsigned id) override { dataBlockId = id; }
  virtual unsigned getDataBlockId() override { return dataBlockId; }

  virtual BBox3 getBbox() const;
  virtual BSphere3 getBsph() const;

public:
  static const int STEP = 512;
  TMatrix tm;
  int entIdx : 30;
  unsigned autoRndSeed : 1, gizmoEnabled : 1;
  int rndSeed;
  int instSeed = 0;
  int placeTypeOverride = -1;
  mutable int boxId;
  unsigned short dataBlockId = IDataBlockIdHolder::invalid_id;

  bool supportsColor();
  void recreateSubent();
};

class CompositEntityPool : public EntityPool<CompositEntity>, public IDagorAssetChangeNotify
{
  struct HierIter // iterate component index 0..comp.size() and get proper parent matrix
  {
    dag::RelocatableFixedVector<int, 8, true, TmpmemAlloc, uint32_t, false> bi;
    dag::RelocatableFixedVector<TMatrix, 8, true, TmpmemAlloc, uint32_t, false> tm;
    const int *bIdx, *eIdx, *bIdxEnd;

    HierIter(dag::ConstSpan<int> begin_ind, dag::ConstSpan<int> end_ind) :
      bIdx(begin_ind.data()), eIdx(end_ind.data()), bIdxEnd(bIdx + begin_ind.size())
    {
      G_ASSERTF(begin_ind.size() == end_ind.size(), "begin_ind.size=%d end_ind.size=%d", begin_ind.size(), end_ind.size());
    }
    void iterate(int i, const TMatrix &stm)
    {
      while (!bi.empty() && i == bi.back())
      {
        bi.pop_back();
        tm.pop_back();
      }
      if (bIdx < bIdxEnd && i == *bIdx)
      {
        bi.push_back(*eIdx);
        tm.push_back(stm);
        bIdx++;
        eIdx++;
      }
      // DAEDITOR3.conNote("%d: block=%d:%d (depth %d) topPos=%@", i, *(bIdx-1), *(eIdx-1), bi.size(), getTm().getcol(3));
    }
    const TMatrix &getTm() const { return tm.back(); }
  };

public:
  CompositEntityPool(const DagorAsset &asset) :
    comp(midmem), emptyComponents(0), beginInd(midmem), endInd(midmem), virtualEntity(compositEntityClassId)
  {
    assetNameId = asset.getNameId();
    aMgr = &asset.getMgr();
    aMgr->subscribeUpdateNotify(this, assetNameId, asset.getType());

    loadAsset(asset.props);
  }
  ~CompositEntityPool()
  {
    if (aMgr)
      aMgr->unsubscribeUpdateNotify(this);
  }

  void loadAsset(const DataBlock &blk)
  {
    bsph.setempty();
    bbox.setempty();
    bboxBuildPairs.clear();

    generated = false;
    forceInstSeed0 = blk.getBool("forceInstSeed0", false);
    quantizeTm = blk.getBool("quantizeTm", false);

    G_STATIC_ASSERT(IDataBlockIdHolder::invalid_id == 0);
    unsigned nextDataBlockId = 1;
    defPlaceTypeOverride = blk.getInt("placeTypeOverride", -1);
    loadAssetData(blk, nextDataBlockId, TMatrix::IDENT, false, false);
    /*
    DAEDITOR3.conNote("beginInd.size=%d endInd.size=%d comp.size=%d", beginInd.size(), endInd.size(), comp.size());
    for (int i = 0; i < beginInd.size(); i ++)
      DAEDITOR3.conNote("  beginInd[%d]=%d", i, beginInd[i]);
    for (int i = 0; i < endInd.size(); i ++)
      DAEDITOR3.conNote("  endInd[%d]=%d", i, endInd[i]);
    for (int i = 0; i < comp.size(); i ++)
      DAEDITOR3.conNote("  comp[%d]: entList.size=%d ctm=%@", i, comp[i].entList.size(), comp[i].ctm());
    */

    if (comp.size() == emptyComponents)
    {
      comp.clear();
      emptyComponents = 0;
    }
    else
      for (int i = 0, j = 0; i < comp.size(); i++)
        if (!comp[i].empty())
        {
          comp[i].realIdx = j;
          j++;
        }

    recalcPoolBbox();

    if (const DataBlock *b = blk.getBlockByName("require"))
    {
      for (int i = 0; i < b->blockCount(); i++)
      {
        const DataBlock &b2 = *b->getBlock(i);
        int ridx = getCompositSubEntityIdxByLabel(b2.getBlockName());
        if (ridx < 0)
        {
          DAEDITOR3.conError("undefined label <%s> referenced in %s", b2.getBlockName(), blk.resolveFilename());
          continue;
        }
        Requirement &r = req.push_back();
        r.targetRealIdx = ridx;
        r.checkAll = b2.getBool("all", true);
        for (int j = 0; j < b2.paramCount(); j++)
          if (b2.getParamType(j) == DataBlock::TYPE_STRING)
          {
            int ridx2 = getCompositSubEntityIdxByLabel(b2.getParamName(j));
            if (ridx2 < 0)
            {
              DAEDITOR3.conError("undefined label <%s> referenced in %s", b2.getParamName(j), blk.resolveFilename());
              continue;
            }
            Requirement::Cond &c = r.cond.push_back();
            c.srcRealIdx = ridx2;
            c.subLabelId = -1;
            c.assetNid = getAssetNid(b2.getStr(j));
          }
        for (int k = 0; k < b2.blockCount(); k++)
        {
          const DataBlock &b3 = *b2.getBlock(k);
          int ridx2 = getCompositSubEntityIdxByLabel(b3.getBlockName());
          if (ridx2 < 0)
          {
            DAEDITOR3.conError("undefined label <%s> referenced in %s", b3.getBlockName(), blk.resolveFilename());
            continue;
          }
          for (int j = 0; j < b3.paramCount(); j++)
            if (b3.getParamType(j) == DataBlock::TYPE_STRING)
            {
              Requirement::Cond &c = r.cond.push_back();
              c.srcRealIdx = ridx2;
              c.subLabelId = labelId.addNameId(b3.getParamName(j));
              c.assetNid = getAssetNid(b3.getStr(j));
            }
        }

        if (!r.cond.size())
          req.pop_back();
      }
    }
  }

  bool checkEqual(const DagorAsset &asset)
  {
    return asset.getNameId() == assetNameId; // strcmp(assetFname, asset.getName()) == 0;
  }

  void createSubEnt(CompositEntity *e)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return;
    e->entIdx = ePool.addEntities(realEntCnt, 128);

    int seed = 0, pos_seed = (!forceInstSeed0 && e->instSeed) ? e->instSeed : mem_hash_fnv1((const char *)&e->tm.m[3][0], 12);
    int place_type_override = -1;
    if (ICompositObj *ico = e->queryInterface<ICompositObj>())
      place_type_override = ico->getCompositPlaceTypeOverride();
    if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())
      seed = irsh->getSeed();

    for (int i = 0, j = 0; i < comp.size(); i++)
      if (!comp[i].empty())
      {
        G_ASSERT(j < realEntCnt);
        const Component::Obj &obj = comp[i].selectEnt(seed);
        IObjEntity *entity = obj.ent;
        if (entity)
        {
          if (ISplineEntity *se = entity->queryInterface<ISplineEntity>())
            ePool.ent[e->entIdx + j] = se->createSplineEntityInstance();
          else
            ePool.ent[e->entIdx + j] = IObjEntity::clone(entity);
          if (IRandomSeedHolder *irsh = ePool.ent[e->entIdx + j]->queryInterface<IRandomSeedHolder>())
          {
            irsh->setSeed(seed);
            irsh->setPerInstanceSeed(comp[i].setInstSeed0 ? 0 : pos_seed);
          }
          if (ICompositObj *ico = ePool.ent[e->entIdx + j]->queryInterface<ICompositObj>())
            ico->setCompositPlaceTypeOverride(place_type_override);
          if (comp[i].defColorIdx != 0x400000 && ePool.ent[e->entIdx + j])
            if (IColor *c = ePool.ent[e->entIdx + j]->queryInterface<IColor>())
              c->setColor(comp[i].defColorIdx);
          if (IDataBlockIdHolder *dbih = ePool.ent[e->entIdx + j]->queryInterface<IDataBlockIdHolder>())
            dbih->setDataBlockId(obj.dataBlockId);
          if (ePool.ent[e->entIdx + j])
            ePool.ent[e->entIdx + j]->setSubtype(IObjEntity::ST_NOT_COLLIDABLE);
        }
        else
          ePool.ent[e->entIdx + j] = NULL;
        j++;
      }

    for (int i = 0; i < req.size(); i++)
    {
      if (!ePool.ent[e->entIdx + req[i].targetRealIdx])
        continue;
      bool check = req[i].checkAll ? true : false;
      for (int j = 0; j < req[i].cond.size(); j++)
      {
        IObjEntity *entity = ePool.ent[e->entIdx + req[i].cond[j].srcRealIdx];
        bool ok = false;

        if (entity && req[i].cond[j].subLabelId >= 0)
        {
          if (ICompositObj *co = entity->queryInterface<ICompositObj>())
          {
            int ridx = co->getCompositSubEntityIdxByLabel(labelId.getName(req[i].cond[j].subLabelId));
            entity = ridx < 0 ? NULL : co->getCompositSubEntity(ridx);
          }
          else
            entity = NULL;
        }

        if (req[i].cond[j].assetNid == -1 && !entity)
          ok = true;
        else if (req[i].cond[j].assetNid == -2 && entity)
          ok = true;
        else if (entity && refAssetNames.getNameId(entity->getObjAssetName()) == req[i].cond[j].assetNid)
          ok = true;

        if (req[i].checkAll && !ok)
        {
          check = false;
          break;
        }
        else if (!req[i].checkAll && ok)
        {
          check = true;
          break;
        }
      }

      if (!check)
      {
        ePool.ent[e->entIdx + req[i].targetRealIdx]->destroy();
        ePool.ent[e->entIdx + req[i].targetRealIdx] = NULL;
      }
    }
  }
  void releaseSubEnt(CompositEntity *e)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt > 0)
    {
      for (int i = 0; i < realEntCnt; i++)
        if (ePool.ent[e->entIdx + i])
          ePool.ent[e->entIdx + i]->destroy();
      ePool.delEntities(e->entIdx);
    }
  }

  void addEntityRaw(CompositEntity *e)
  {
    EntityPool<CompositEntity>::addEntityRaw(e);
    e->placeTypeOverride = defPlaceTypeOverride;
    createSubEnt(e);
  }
  void delEntityRaw(CompositEntity *e)
  {
    releaseSubEnt(e);
    afterDeleteEntity(e);
    EntityPool<CompositEntity>::delEntityRaw(e);
  }

  inline int calcBox(int ent_idx, int rndseed)
  {
    int realEntCnt = comp.size() - emptyComponents;

    if (!generated || (realEntCnt <= 0))
      return -1;

    TMatrix stm(TMatrix::IDENT);

    int boxId = boxCache.add();

    BBox3 ceBbox;
    BSphere3 ceBsph;
    ceBbox.setempty();
    ceBsph.setempty();

    const int cnt = comp.size();
    HierIter iter(beginInd, endInd);
    for (int i = 0, k = 0; i < cnt; i++)
    {
      iter.iterate(i, stm);

      if (comp[i].type == DST_TYPE_MATRIX)
        stm = iter.getTm() * comp[i].ctm();
      else
      {
        generateNewTm(stm, i, rndseed);
        stm = iter.getTm() * stm;
      }

      if (!comp[i].empty())
      {
        if (IObjEntity *e = ePool.ent[ent_idx + k])
        {
          ceBbox += stm * e->getBbox();
          ceBsph += stm * e->getBsph();
        }
        k++;
      }
    }

    boxCache[boxId].bbox = ceBbox;
    boxCache[boxId].setBsph(ceBsph);
    return boxId;
  }

  void setTm(const TMatrix &tm, int ent_idx, CompositEntity *ce)
  {
    int realEntCnt = comp.size() - emptyComponents;

    if (realEntCnt <= 0)
      return;

    TMatrix stm(tm);

    int rndseed;
    int pos_seed = (!forceInstSeed0 && ce->instSeed) ? ce->instSeed : mem_hash_fnv1((const char *)&tm.m[3][0], 3 * 4);
    IRandomSeedHolder *irsh = ce->queryInterface<IRandomSeedHolder>();
    if (irsh)
      rndseed = irsh->getSeed();
    else
      rndseed = 0;

    const int cnt = comp.size();
    HierIter iter(beginInd, endInd);
    for (int i = 0, k = 0; i < cnt; i++)
    {
      iter.iterate(i, stm);
      if (comp[i].type == DST_TYPE_MATRIX)
        stm = iter.getTm() * comp[i].ctm();
      else
      {
        generateNewTm(stm, i, rndseed);
        stm = iter.getTm() * stm;
      }

      if (!comp[i].empty())
      {
        IObjEntity *e = ePool.ent[ent_idx + k];
        ICompositObj::Props props = comp[i].p;
        Point3 p = stm.getcol(3);

        if (ce->placeTypeOverride >= 0)
          props.placeType = ce->placeTypeOverride;

        if (!ce->gizmoEnabled)
        {
          if (props.placeType == props.PT_coll)
            objgenerator::place_on_ground(p, props.aboveHt);
          else if (props.placeType == props.PT_collNorm)
          {
            Point3 norm = normalize(stm.getcol(1));
            Point3 scl(stm.getcol(0).length(), stm.getcol(1).length(), stm.getcol(2).length());
            objgenerator::place_on_ground(p, norm, props.aboveHt);
            stm.setcol(1, scl[1] * norm);
            stm.setcol(2, scl[2] * (normalize(stm.getcol(0)) % normalize(stm.getcol(1))));
            stm.setcol(0, scl[0] * (norm % normalize(stm.getcol(2))));
          }
          else if (props.placeType == props.PT_3pod && e)
          {
            MpPlacementRec mppRec;
            mppRec.mpOrientType = mppRec.MP_ORIENT_3POINT;
            mppRec.makePointsFromBox(e->getBbox());
            mppRec.computePivotBc();

            stm.setcol(3, ZERO<Point3>());
            objgenerator::place_multipoint(mppRec, p, stm, props.aboveHt);
            objgenerator::rotate_multipoint(stm, mppRec);
          }
          else if (props.placeType == props.PT_fnd && e)
          {
            BBox3 box = e->getBbox();
            float dh;
            box[0].y = 0;
            dh = objgenerator::dist_to_ground(stm * box.point(0), props.aboveHt);
            inplace_max(dh, objgenerator::dist_to_ground(stm * box.point(1), props.aboveHt));
            inplace_max(dh, objgenerator::dist_to_ground(stm * box.point(4), props.aboveHt));
            inplace_max(dh, objgenerator::dist_to_ground(stm * box.point(5), props.aboveHt));
            p.y -= dh;
          }
          else if (props.placeType == props.PT_flt && e)
          {
            IWaterService *waterService = EDITORCORE->queryEditorInterface<IWaterService>();
            p.y = waterService ? waterService->get_level() : 0.0f;
          }
          stm.setcol(3, p);
        }

        if (e)
        {
          if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())
            irsh->setPerInstanceSeed(comp[i].setInstSeed0 ? 0 : pos_seed);
          if (ICompositObj *ico = e->queryInterface<ICompositObj>())
            ico->setCompositPlaceTypeOverride(ce->placeTypeOverride);
          e->setTm(stm);
        }
        k++;
      }
    }
  }
  void setSubtype(int t, int ent_idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return;
    for (int i = 0; i < realEntCnt; i++)
      if (ePool.ent[ent_idx + i])
        ePool.ent[ent_idx + i]->setSubtype(t);
  }
  void setEditLayerIdx(int t, int ent_idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return;
    for (int i = 0; i < realEntCnt; i++)
      if (ePool.ent[ent_idx + i])
        ePool.ent[ent_idx + i]->setEditLayerIdx(t);
  }
  void setGizmoTranformMode(int ent_idx, bool enabled)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return;
    for (int i = 0; i < realEntCnt; i++)
      if (ePool.ent[ent_idx + i])
        ePool.ent[ent_idx + i]->setGizmoTranformMode(enabled);
  }
  bool supportsColor(int ent_idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return false;
    for (int i = 0; i < realEntCnt; i++)
      if (ePool.ent[ent_idx + i] && ePool.ent[ent_idx + i]->queryInterface<IColor>())
        return true;
    return false;
  }
  void setColor(unsigned int in_color, int ent_idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return;
    for (int i = 0; i < realEntCnt; i++)
      if (IColor *c = ePool.ent[ent_idx + i] ? ePool.ent[ent_idx + i]->queryInterface<IColor>() : NULL)
        c->setColor(in_color);
  }
  int getCompositSubEntityCount(int ent_idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    int cnt = 0;
    for (int i = 0; i < realEntCnt; i++)
      if (ePool.ent[ent_idx + i])
        cnt++;
    return cnt;
  }
  IObjEntity *getCompositSubEntity(int ent_idx, int idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    int cnt = 0;
    for (int i = 0; i < realEntCnt; i++)
      if (ePool.ent[ent_idx + i])
      {
        if (idx == 0)
          return ePool.ent[ent_idx + i];
        idx--;
      }
    return NULL;
  }
  const ICompositObj::Props &getCompositSubEntityProps(int idx)
  {
    static const ICompositObj::Props def = {ICompositObj::Props::PT_none, 0.f};
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return def;

    for (int i = 0; i < comp.size(); i++)
      if (comp[i].realIdx == idx)
        return comp[i].p;
    return def;
  }
  int getCompositSubEntityIdxByLabel(const char *label)
  {
    int lid = labelId.getNameId(label);
    if (lid < 0)
      return -1;
    for (int i = 0; i < comp.size(); i++)
      if (comp[i].labelId == lid)
        return comp[i].realIdx;
    return -1;
  }
  void setCompositPlaceTypeOverride(int place_type_override, int ent_idx)
  {
    int realEntCnt = comp.size() - emptyComponents;
    if (realEntCnt <= 0)
      return;
    for (int i = 0; i < realEntCnt; i++)
      if (ICompositObj *c = ePool.ent[ent_idx + i] ? ePool.ent[ent_idx + i]->queryInterface<ICompositObj>() : NULL)
        c->setCompositPlaceTypeOverride(place_type_override);
  }

  const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:%s", aMgr->getAssetName(assetNameId), aMgr->getAssetTypeName(compositEntityClassId));
    return buf;
  }

  void setupVirtualEnt(MultiCompositEntityPool *p, int idx)
  {
    virtualEntity.pool = p;
    virtualEntity.poolIdx = idx;
  }
  IObjEntity *getVirtualEnt() { return &virtualEntity; }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    // release all derivative entities
    if (comp.size())
      for (int i = ent.size() - 1; i >= 0; i--)
      {
        CompositEntity *e = ent[i];
        if (!e)
          continue;

        releaseSubEnt(e);
        afterDeleteEntity(e);
      }
    ePool.reset();
    comp.clear();
    emptyComponents = 0;
    beginInd.clear();
    endInd.clear();

    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    onAssetRemoved(asset_name_id, asset_type);

    // reload asset
    loadAsset(asset.props);

    // recreate derivative entities
    if (comp.size())
      for (int i = ent.size() - 1; i >= 0; i--)
      {
        CompositEntity *e = ent[i];
        if (!e)
          continue;

        createSubEnt(e);
        setTm(e->tm, e->entIdx, e);
        setSubtype(e->getSubtype(), e->entIdx);
        setEditLayerIdx(e->getEditLayerIdx(), e->entIdx);
      }
  }

  void renderInvalidEntities(int subtype_mask)
  {
    if (comp.size() - emptyComponents <= 0)
      ::render_invalid_entities(ent, subtype_mask);
  }

  inline void generateNewTm(TMatrix &tm, int index, int rnd_seed) { getTm(tm, comp[index].cel(), rnd_seed); }

  void recalcPoolBbox()
  {
    bsph.setempty();
    bbox.setempty();
    for (BboxBuildPairs &p : bboxBuildPairs)
    {
      bsph += p.m * p.e->getBsph();
      bbox += p.m * p.e->getBbox();
    }
    if (generated || !comp.size() || !bboxBuildPairs.size())
      bbox = bsph = BSphere3(Point3(0, 0, 0), 0.4);
  }

  bool getQuantizeTm() const { return quantizeTm; }

protected:
  struct ObjCoordData
  {
    Point2 rotX, rotY, rotZ;
    Point2 xOffset, yOffset, zOffset;
    Point2 xScale, yScale;
    Point2 scale;
  };

  struct Component
  {
    struct Obj
    {
      IObjEntity *ent;
      float weight;
      unsigned short dataBlockId;
    };

    Tab<Obj> entList;
    int defColorIdx;
    unsigned type : 7, setInstSeed0 : 1;
    ICompositObj::Props p;
    int labelId;
    int realIdx;

    union
    {
      char tm[sizeof(TMatrix)];
      char cel[sizeof(ObjCoordData)];
    } data;
    TMatrix &ctm() { return reinterpret_cast<TMatrix &>(data.tm); }
    ObjCoordData &cel() { return reinterpret_cast<ObjCoordData &>(data.cel); }

    Component() : defColorIdx(0x400000), type(0), labelId(-1), realIdx(-1)
    {
      memset(&p, 0, sizeof(p));
      memset(&data, 0, sizeof(data));
    }

    const Obj &selectEnt(int &rnd_seed) const
    {
      if (entList.size() == 1)
      {
        _rnd(rnd_seed);
        return entList[0];
      }

      float w = _frnd(rnd_seed);
      for (int i = 0; i < entList.size(); ++i)
      {
        w -= entList[i].weight;
        if (w <= 0)
          return entList[i];
      }

      return entList.back();
    }

    bool empty() const { return entList.empty(); }
  };
  struct Requirement
  {
    struct Cond
    {
      int srcRealIdx, subLabelId;
      int assetNid;
    };
    int targetRealIdx;
    bool checkAll;
    Tab<Cond> cond;
  };

  class QuantedEntitiesPool
  {
  public:
    QuantedEntitiesPool() : ent(midmem), entUuIdx(midmem) {}

    int addEntities(int quant, int step = 128)
    {
      if (!entUuIdx.size())
        return append_items(ent, quant);
      else
      {
        int idx = entUuIdx.back();
        entUuIdx.pop_back();
        return idx;
      }
    }
    void delEntities(int idx)
    {
      G_ASSERT(idx >= 0 && idx < ent.size());
      entUuIdx.push_back(idx);
    }

    void reset()
    {
      ent.clear();
      entUuIdx.clear();
    }

  public:
    Tab<IObjEntity *> ent;

  protected:
    Tab<int> entUuIdx;
  };

  struct BboxBuildPairs
  {
    TMatrix m;
    IObjEntity *e;
  };

  VirtualCompositEntity virtualEntity;
  QuantedEntitiesPool ePool;
  Tab<Component> comp;
  Tab<Requirement> req;
  int emptyComponents;
  Tab<int> beginInd, endInd;
  Tab<BboxBuildPairs> bboxBuildPairs;
  DagorAssetMgr *aMgr;
  int assetNameId;
  int defPlaceTypeOverride = -1;
  unsigned generated : 1, forceInstSeed0 : 1, quantizeTm : 1;

  enum
  {
    DST_TYPE_EILER,
    DST_TYPE_MATRIX,
  };

protected:
  __forceinline real getRandom(const Point2 &r, int &rnd_seed) { return r.y ? r.x + (_frnd(rnd_seed) * 2 - 1) * r.y : r.x; }

  inline void getTm(TMatrix &tm, const ObjCoordData &gen, int rnd_seed)
  {
    tm = TMatrix::IDENT;

    Point3 dx(1, 0, 0);
    Point3 dy(0, 1, 0);
    Point3 dz(0, 0, 1);

    Point3 ang(getRandom(gen.rotX, rnd_seed), getRandom(gen.rotY, rnd_seed), getRandom(gen.rotZ, rnd_seed));

    TMatrix rot = rotyTM(ang.y) * rotzTM(ang.z);

    rot *= rotxTM(ang.x);
    tm = rot * tm;
    tm.setcol(3,
      dx * getRandom(gen.xOffset, rnd_seed) + dy * getRandom(gen.yOffset, rnd_seed) + dz * getRandom(gen.zOffset, rnd_seed));

    TMatrix scaleTm = TMatrix::IDENT;
    real scaleVal = getRandom(gen.scale, rnd_seed);
    if (scaleVal < 0)
      scaleVal = 0;

    real yScaleVal = getRandom(gen.yScale, rnd_seed);
    if (yScaleVal < 0)
      yScaleVal = 0;

    scaleTm.setcol(0, Point3(scaleVal, 0, 0));
    scaleTm.setcol(1, Point3(0, scaleVal * yScaleVal, 0));
    scaleTm.setcol(2, Point3(0, 0, scaleVal));

    tm *= scaleTm;
  }

  inline void getStTm(TMatrix &tm, const ObjCoordData &gen)
  {
    tm = TMatrix::IDENT;

    const Point3 dx(1, 0, 0);
    const Point3 dy(0, 1, 0);
    const Point3 dz(0, 0, 1);

    const Point3 ang(gen.rotX.x, gen.rotY.x, gen.rotZ.x);

    const TMatrix rot = (rotyTM(ang.y) * rotzTM(ang.z)) * rotxTM(ang.x);

    tm = rot * tm;

    tm.setcol(3, dx * gen.xOffset.x + dy * gen.yOffset.x + dz * gen.zOffset.x);

    TMatrix scaleTm = TMatrix::IDENT;
    const real scaleVal = gen.scale.x < 0 ? 0 : gen.scale.x;
    const real yScaleVal = gen.yScale.x < 0 ? 0 : gen.yScale.x;

    scaleTm.setcol(0, Point3(scaleVal, 0, 0));
    scaleTm.setcol(1, Point3(0, scaleVal * yScaleVal, 0));
    scaleTm.setcol(2, Point3(0, 0, scaleVal));

    tm *= scaleTm;
  }

  bool loadAddEnt(Component &c, const char *a_name, float wt, unsigned dataBlockId)
  {
    DagorAsset *a = *a_name ? DAEDITOR3.getGenObjAssetByName(a_name) : NULL;
    if (!a && *a_name)
      DAEDITOR3.conError("cannot find asset <%s> in composit <%s>", a_name, aMgr->getAssetName(assetNameId));

    Component::Obj &obj = c.entList.push_back();
    obj.weight = wt;
    obj.ent = a ? DAEDITOR3.createEntity(*a, true) : NULL;
    obj.dataBlockId = dataBlockId;
    return obj.ent != NULL;
  }

  void loadAssetData(const DataBlock &blk, unsigned &nextDataBlockId, const TMatrix &ptm = TMatrix::IDENT, bool need_generate = false,
    bool set_perinst_seed0 = false)
  {
    int bi_pos = beginInd.size();
    beginInd.push_back(comp.size());
    endInd.push_back(comp.size());
    int node_nid = blk.getNameId("node");
    int spline_nid = blk.getNameId("spline");
    int polygon_nid = blk.getNameId("polygon");
    int colors_nid = blk.getNameId("colors");
    int ent_nid = blk.getNameId("ent");

    // NOTE: the logic in CompositeEditorTreeData::setDataBlockIds and in CompositEntityPool::loadAssetData must match,
    // otherwise the data block IDs will not be in sync.
    for (int i = 0; i < blk.blockCount(); i++)
    {
      const DataBlock &b = *blk.getBlock(i);

      if (b.getBlockNameId() == colors_nid || b.getBlockNameId() == ent_nid || b.getBlockNameId() == spline_nid ||
          b.getBlockNameId() == polygon_nid)
        continue;
      if (b.getBlockNameId() != node_nid)
        DAEDITOR3.conError("unsupported block name: '%s'", b.getBlockName());

      TMatrix sztm(ptm);

      Component c;
      bool has_ent = false;

      // process entity list
      for (int j = 0, nid = b.getNameId("name"); j < b.paramCount(); j++)
        if (b.getParamType(j) == DataBlock::TYPE_STRING && b.getParamNameId(j) == nid)
          if (loadAddEnt(c, b.getStr(j), 1, nextDataBlockId++))
            has_ent = true;
      for (int j = 0; j < b.blockCount(); j++)
        if (b.getBlock(j)->getBlockNameId() == ent_nid)
          if (loadAddEnt(c, b.getBlock(j)->getStr("name", ""), b.getBlock(j)->getReal("weight", 1), nextDataBlockId++))
            has_ent = true;
      for (int j = 0; j < b.blockCount(); j++)
        if (b.getBlock(j)->getBlockNameId() == spline_nid || b.getBlock(j)->getBlockNameId() == polygon_nid)
          if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
            if (IObjEntity *e = splSrv->createVirtualSplineEntity(*b.getBlock(j)))
            {
              Component::Obj &obj = c.entList.push_back();
              obj.ent = e;
              obj.weight = b.getBlock(j)->getReal("weight", 1);
              has_ent = true;
            }

      if (has_ent)
      {
        // normalize weight
        float wt = 0;
        for (int j = 0; j < c.entList.size(); j++)
          wt += c.entList[j].weight;
        if (wt <= 0)
          wt = 1;
        for (int j = 0; j < c.entList.size(); j++)
          c.entList[j].weight /= wt;
        c.setInstSeed0 = b.getBool("ignoreParentInstSeed", set_perinst_seed0);
      }
      else
        clear_and_shrink(c.entList);

      if (c.entList.size() > 1)
        need_generate = generated = true;

      if (const char *label = b.getStr("label", NULL))
        c.labelId = labelId.addNameId(label);

      if (b.getBlockByName("colors") || blk.getBlockByName("colors"))
        if (IColorRangeService *colRangeSrv = EDITORCORE->queryEditorInterface<IColorRangeService>())
        {
          const DataBlock &colBlk = *blk.getBlockByNameEx("colors");
          const DataBlock &colBlk2 = *b.getBlockByNameEx("colors");
          c.defColorIdx = colRangeSrv->addColorRange(colBlk2.getE3dcolor("colorFrom", colBlk.getE3dcolor("colorFrom", 0x80808080U)),
            colBlk2.getE3dcolor("colorTo", colBlk.getE3dcolor("colorto", 0x80808080U)));
        }

      if (b.paramExists("tm"))
      {
        c.ctm() = b.getTm("tm", TMatrix::IDENT);
        if (c.ctm().getcol(0).lengthSq() < 1e-12 || c.ctm().getcol(1).lengthSq() < 1e-12 || c.ctm().getcol(2).lengthSq() < 1e-12)
        {
          DynamicMemGeneralSaveCB blk_txt(tmpmem, 0, 4 << 10);
          dblk::print_to_text_stream_limited(b, blk_txt, 10, 0, 2);
          DAEDITOR3.conError("%s: bad matrix in %s\n%.*s", aMgr->getAssetName(assetNameId), blk.resolveFilename(), blk_txt.size(),
            blk_txt.data());

          clear_and_shrink(c.entList);
          c.ctm() = TMatrix::IDENT;
          has_ent = false;
        }

        c.type = DST_TYPE_MATRIX;
      }
      else
      {
        ObjCoordData &cel = c.cel();

        cel.rotX = b.getPoint2("rot_x", Point2(0, 0)) * DEG_TO_RAD;
        cel.rotY = b.getPoint2("rot_y", Point2(0, 0)) * DEG_TO_RAD;
        cel.rotZ = b.getPoint2("rot_z", Point2(0, 0)) * DEG_TO_RAD;
        cel.xOffset = b.getPoint2("offset_x", Point2(0, 0));
        cel.yOffset = b.getPoint2("offset_y", Point2(0, 0));
        cel.zOffset = b.getPoint2("offset_z", Point2(0, 0));
        cel.scale = b.getPoint2("scale", Point2(1, 0));
        cel.xScale = b.getPoint2("xScale", Point2(1, 0));
        cel.yScale = b.getPoint2("yScale", Point2(1, 0));

        bool generatable = (cel.rotX.y != 0) || (cel.rotY.y != 0) || (cel.rotZ.y != 0) || (cel.xOffset.y != 0) ||
                           (cel.yOffset.y != 0) || (cel.zOffset.y != 0) || (cel.scale.y != 0) || (cel.xScale.y != 0) ||
                           (cel.yScale.y != 0);

        if (!generatable)
        {
          TMatrix tmpTm;
          getStTm(tmpTm, c.cel());

          c.type = DST_TYPE_MATRIX;
          c.ctm() = tmpTm;
        }
        else
        {
          c.type = DST_TYPE_EILER;
          need_generate = generated = true;
        }
      }

      bool placeOnCollision = b.getBool("placeOnCollision", blk.getBool("placeOnCollision", false));
      bool useCollisionNorm = b.getBool("useCollisionNormal", blk.getBool("useCollisionNormal", false));
      placeOnCollision = b.getBool("place_on_collision", placeOnCollision);
      useCollisionNorm = b.getBool("use_collision_norm", useCollisionNorm);

      c.p.placeType = c.p.PT_none;
      if (!placeOnCollision)
        c.p.placeType = c.p.PT_none;
      else if (useCollisionNorm)
        c.p.placeType = c.p.PT_collNorm;
      else
        c.p.placeType = c.p.PT_coll;
      c.p.placeType = b.getInt("place_type", blk.getInt("place_type", c.p.placeType));
      c.p.aboveHt = b.getReal("aboveHt", blk.getReal("aboveHt", c.p.placeType ? 5 : 0));

      if (!generated && has_ent)
      {
        sztm = ptm * c.ctm();
        for (int i = 0; i < c.entList.size(); i++)
          if (c.entList[i].ent)
            bboxBuildPairs.push_back(BboxBuildPairs{sztm, c.entList[i].ent});
      }

      int prev_comp = comp.size(), prev_empty = emptyComponents;

      {
        if (c.empty())
          emptyComponents++;
        comp.push_back(c);
      }

      if (b.blockCount())
        loadAssetData(b, nextDataBlockId, sztm, need_generate, set_perinst_seed0);
      if (comp.size() == prev_comp + emptyComponents - prev_empty)
      {
        emptyComponents -= comp.size() - prev_comp;
        comp.resize(prev_comp);
      }
    }

    if (beginInd[bi_pos] < comp.size())
      endInd[bi_pos] = comp.size();
    else
    {
      beginInd.resize(bi_pos);
      endInd.resize(bi_pos);
    }
  }
  inline void afterDeleteEntity(CompositEntity *e)
  {
    if (e->boxId >= 0)
      boxCache.del(e->boxId);
    e->boxId = -2;
  }
};

BBox3 CompositEntity::getBbox() const
{
  CompositEntityPool *cPool = pool->getPools()[poolIdx];
  if (boxId == -1)
    return cPool->getBbox();
  else if (boxId >= 0)
    return boxCache[boxId].bbox;
  else
  {
    boxId = cPool->calcBox(entIdx, rndSeed);
    return boxId >= 0 ? boxCache[boxId].bbox : cPool->getBbox();
  }
}

BSphere3 CompositEntity::getBsph() const
{
  CompositEntityPool *cPool = pool->getPools()[poolIdx];
  if (boxId == -1)
    return cPool->getBsph();
  else if (boxId >= 0)
    return boxCache[boxId].getBsph();
  else
  {
    boxId = cPool->calcBox(entIdx, rndSeed);
    return boxId >= 0 ? boxCache[boxId].getBsph() : cPool->getBsph();
  }
}

void CompositEntity::setSeed(int new_seed)
{
  autoRndSeed = false;
  if (rndSeed == new_seed)
    return;
  rndSeed = new_seed;
  if (!gizmoEnabled)
    recreateSubent();
}

void CompositEntity::recreateSubent()
{
  CompositEntityPool *cPool = pool->getPools()[poolIdx];
  cPool->releaseSubEnt(this);
  cPool->createSubEnt(this);
  cPool->setTm(tm, entIdx, this);
  cPool->setSubtype(subType, entIdx);
  cPool->setEditLayerIdx(editLayerIdx, entIdx);

  if (boxId >= 0)
    boxCache.del(boxId);
  boxId = -2;
}

void CompositEntity::setTm(const TMatrix &_tm)
{
  tm = _tm;
  if (autoRndSeed)
    rndSeed = *(int *)&tm.m[3][0] + *(int *)&tm.m[3][1] + *(int *)&tm.m[3][2];

#define QUANTIZE_R(V) (V) = floorf((V)*256.0f) / 256.0f
#define QUANTIZE_P(V) (V) = floorf((V)*32.0f) / 32.0f
  if (pool->getPools()[poolIdx]->getQuantizeTm())
  {
    QUANTIZE_R(tm[0][0]);
    QUANTIZE_R(tm[0][1]);
    QUANTIZE_R(tm[0][2]);
    QUANTIZE_P(tm[0][3]);
    QUANTIZE_R(tm[1][0]);
    QUANTIZE_R(tm[1][1]);
    QUANTIZE_R(tm[1][2]);
    QUANTIZE_P(tm[1][3]);
    QUANTIZE_R(tm[2][0]);
    QUANTIZE_R(tm[2][1]);
    QUANTIZE_R(tm[2][2]);
    QUANTIZE_P(tm[2][3]);
  }
#undef QUANTIZE_R
#undef QUANTIZE_P

  pool->getPools()[poolIdx]->setTm(tm, entIdx, this);
}
void CompositEntity::setPerInstanceSeed(int seed)
{
  instSeed = seed;
  pool->getPools()[poolIdx]->setTm(tm, entIdx, this);
}
void CompositEntity::setSubtype(int t)
{
  subType = t;
  pool->getPools()[poolIdx]->setSubtype(t, entIdx);
}
void CompositEntity::setEditLayerIdx(int t)
{
  editLayerIdx = t;
  pool->getPools()[poolIdx]->setEditLayerIdx(t, entIdx);
}
void CompositEntity::setGizmoTranformMode(bool enabled)
{
  if (gizmoEnabled == enabled)
    return;
  gizmoEnabled = enabled;
  pool->getPools()[poolIdx]->setGizmoTranformMode(entIdx, enabled);
  if (!gizmoEnabled)
    recreateSubent();
}
void CompositEntity::setColor(unsigned int in_color) { pool->getPools()[poolIdx]->setColor(in_color, entIdx); }
bool CompositEntity::supportsColor() { return pool->getPools()[poolIdx]->supportsColor(entIdx); }

int CompositEntity::getCompositSubEntityCount() { return pool->getPools()[poolIdx]->getCompositSubEntityCount(entIdx); }
IObjEntity *CompositEntity::getCompositSubEntity(int idx) { return pool->getPools()[poolIdx]->getCompositSubEntity(entIdx, idx); }
const ICompositObj::Props &CompositEntity::getCompositSubEntityProps(int idx)
{
  return pool->getPools()[poolIdx]->getCompositSubEntityProps(idx);
}
int CompositEntity::getCompositSubEntityIdxByLabel(const char *label)
{
  return pool->getPools()[poolIdx]->getCompositSubEntityIdxByLabel(label);
}
void CompositEntity::setCompositPlaceTypeOverride(int placeType)
{
  pool->getPools()[poolIdx]->setCompositPlaceTypeOverride(placeTypeOverride = placeType, entIdx);
}

class CompositEntityManagementService : public IEditorService, public IObjEntityMgr
{
public:
  CompositEntityManagementService() { compositEntityClassId = IDaEditor3Engine::get().getAssetTypeId("composit"); }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_compEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return NULL; }

  virtual void setServiceVisible(bool /*vis*/) {}
  virtual bool getServiceVisible() const { return true; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    if (!IObjEntityFilter::getShowInvalidAsset())
      return;

    dag::ConstSpan<CompositEntityPool *> p = cPool.getPools();
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

    for (int i = 0; i < p.size(); i++)
      p[i]->renderInvalidEntities(subtype_mask);
  }
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      DAEDITOR3.conNote("CompositMgr: %d entities", cPool.getUsedEntityCount());
    }
    else if (event_huid == HUID_BeforeMainLoop)
    {
      for (CompositEntityPool *p : cPool.getPools())
        p->recalcPoolBbox();
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    return NULL;
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return compositEntityClassId >= 0 && compositEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    CircularDependenceChecker chk;

    if (chk.isCyclicError(asset))
      return NULL;

    int pool_idx = cPool.findPool(asset);
    if (pool_idx < 0)
      pool_idx = cPool.addPool(new CompositEntityPool(asset));

    if (virtual_ent)
      return cPool.getVirtualEnt(pool_idx);

    if (!cPool.canAddEntity(pool_idx))
      return NULL;

    CompositEntity *ent = cPool.allocEntity();
    if (!ent)
      ent = new CompositEntity(compositEntityClassId);

    ent->rndSeed = 123;

    cPool.addEntity(ent, pool_idx);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    CompositEntity *o = reinterpret_cast<CompositEntity *>(origin);
    if (!cPool.canAddEntity(o->poolIdx))
      return NULL;

    CompositEntity *ent = cPool.allocEntity();
    if (!ent)
      ent = new CompositEntity(o->getAssetTypeId());

    ent->rndSeed = 124;

    cPool.addEntity(ent, o->poolIdx);
    // debug("clone ent: %p", ent);
    return ent;
  }

protected:
  MultiCompositEntityPool cPool;

  struct CircularDependenceChecker
  {
    CircularDependenceChecker()
    {
      if (depthCnt == 0)
        assets.clear();
      depthCnt++;
    }
    ~CircularDependenceChecker()
    {
      G_ASSERT(depthCnt > 0);
      depthCnt--;
      assets.pop_back();
    }

    bool isCyclicError(const DagorAsset &a)
    {
      for (int i = assets.size() - 1; i >= 0; i--)
        if (assets[i] == &a)
        {
          DAEDITOR3.conError("asset <%s> contains circular dependencies, e.g. to %s", assets[0]->getNameTypified(),
            a.getNameTypified());
          assets.push_back(NULL);
          return true;
        }
      assets.push_back(&a);
      return false;
    }

    static int depthCnt;
    static Tab<const DagorAsset *> assets;
  };
};
int CompositEntityManagementService::CircularDependenceChecker::depthCnt = 0;
Tab<const DagorAsset *> CompositEntityManagementService::CircularDependenceChecker::assets(tmpmem);


void init_composit_mgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  IDaEditor3Engine::get().registerService(new (inimem) CompositEntityManagementService);
}
