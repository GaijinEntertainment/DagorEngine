//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <assets/asset.h>
#include <generic/dag_tab.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h> // TMatrix::IDENT
#include <util/dag_globDef.h>


template <class Entity>
class EntityPool
{
public:
  EntityPool() : ent(midmem), entUuIdx(midmem) {}
  ~EntityPool() { clear_all_ptr_items(ent); }

  void addEntityRaw(Entity *e)
  {
    if (!entUuIdx.size())
    {
      e->idx = ent.size();
      ent.push_back(e);
    }
    else
    {
      e->idx = entUuIdx.back();
      entUuIdx.pop_back();
      ent[e->idx] = e;
    }
  }
  void delEntityRaw(Entity *e)
  {
    G_ASSERT(e->idx >= 0 && e->idx < ent.size() && e == ent[e->idx]);
    ent[e->idx] = NULL;
    entUuIdx.push_back(e->idx);
    e->idx = Entity::MAX_ENTITIES;
  }

  dag::ConstSpan<Entity *> getEntities() const { return ent; }

  const BSphere3 &getBsph() const { return bsph; }
  const BBox3 &getBbox() const { return bbox; }

  bool canAddEntity() const { return entUuIdx.size() || ent.size() < Entity::MAX_ENTITIES; }

  bool isEmpty() const { return ent.size() == 0 || ent.size() == entUuIdx.size(); }
  int getUsedEntityCount() const { return ent.size() - entUuIdx.size(); }

  int calcEntities(int subtype_mask)
  {
    if (isEmpty())
      return 0;

    int cnt = 0;
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->checkSubtypeMask(subtype_mask))
        cnt++;

    return cnt;
  }
  virtual bool dumpUsedEntitesCount(int /*idx*/) { return false; }

protected:
  Tab<Entity *> ent;
  Tab<int> entUuIdx;
  BSphere3 bsph;
  BBox3 bbox;
};


template <class Entity>
class SingleEntityPool : public EntityPool<Entity>
{
public:
  SingleEntityPool() : entUu(midmem) {}
  ~SingleEntityPool() { freeUnusedEntities(); }

  Entity *allocEntity()
  {
    if (!entUu.size())
      return NULL;
    Entity *e = entUu.back();
    entUu.pop_back();
    return e;
  }

  void addEntity(Entity *e)
  {
    EntityPool<Entity>::addEntityRaw(e);
    e->pool = this;
  }
  void delEntity(Entity *e)
  {
    EntityPool<Entity>::delEntityRaw(e);
    entUu.push_back(e);
  }

  void freeUnusedEntities() { clear_all_ptr_items(entUu); }

protected:
  Tab<Entity *> entUu;
};


template <class Entity, class Pool>
class MultiEntityPool
{
public:
  MultiEntityPool() : pool(midmem), poolUuIdx(midmem), entUu(midmem) {}
  ~MultiEntityPool()
  {
    clear_all_ptr_items(pool);
    freeUnusedEntities();
  }

  Entity *allocEntity()
  {
    if (!entUu.size())
      return NULL;
    Entity *e = entUu.back();
    entUu.pop_back();
    return e;
  }

  bool canAddEntity(int pool_idx) const { return pool[pool_idx]->canAddEntity(); }


  void addEntity(Entity *e, int pool_idx)
  {
    pool[pool_idx]->addEntityRaw(e);
    if (pool_idx >= Entity::MAX_POOLS)
      dumpUsedPools();
    G_ASSERTF(pool_idx < Entity::MAX_POOLS, "pool_idx=%d MAX_POOLS=%d", pool_idx, Entity::MAX_POOLS);
    e->poolIdx = pool_idx;
    e->pool = this;
  }
  void delEntity(Entity *e)
  {
    G_ASSERT(e->poolIdx >= 0 && e->poolIdx < pool.size() && pool[e->poolIdx]);

    pool[e->poolIdx]->delEntityRaw(e);
    e->poolIdx = Entity::MAX_POOLS;
    entUu.push_back(e);
  }

  int findPool(const DagorAsset &asset)
  {
    for (int i = 0; i < pool.size(); i++)
      if (pool[i] && pool[i]->checkEqual(asset))
        return i;
    return -1;
  }

  int addPool(Pool *p)
  {
    int idx;
    if (!poolUuIdx.size())
    {
      idx = pool.size();
      pool.push_back(p);
    }
    else
    {
      idx = poolUuIdx.back();
      poolUuIdx.pop_back();
      pool[idx] = p;
    }

    p->setupVirtualEnt(this, idx);
    return idx;
  }
  void delPool(int pool_idx)
  {
    G_ASSERT(pool_idx >= 0 && pool_idx < pool.size() && pool[pool_idx]);
    poolUuIdx.push_back(pool_idx);
    del_it(pool[pool_idx]);
  }

  void freeUnusedEntities() { clear_all_ptr_items(entUu); }

  dag::ConstSpan<Pool *> getPools() const { return pool; }

  IObjEntity *getVirtualEnt(int pool_idx) { return pool[pool_idx]->getVirtualEnt(); }

  int getUsedEntityCount() const
  {
    int cnt = 0;
    for (int i = 0; i < pool.size(); i++)
      if (pool[i])
        cnt += pool[i]->getUsedEntityCount();
    return cnt;
  }
  unsigned getUsedPoolsCount() const
  {
    unsigned pool_cnt = 0;
    for (const auto *p : pool)
      if (p && p->getUsedEntityCount())
        pool_cnt++;
    return pool_cnt;
  }
  unsigned getEmptyPoolsCount() const
  {
    unsigned pool_cnt = 0;
    for (const auto *p : pool)
      if (p && !p->getUsedEntityCount())
        pool_cnt++;
    return pool_cnt;
  }
  void dumpUsedPools()
  {
    int used_pools = 0;
    for (int i = 0; i < pool.size(); i++)
      if (pool[i])
        if (pool[i]->dumpUsedEntitesCount(i))
          used_pools++;
    debug("%d pools used (of %d)", used_pools, pool.size());
  }

protected:
  Tab<Pool *> pool;
  Tab<Entity *> entUu;
  Tab<int> poolUuIdx;
};


template <class Pool>
class VirtualMpEntity : public IObjEntity
{
public:
  VirtualMpEntity(int cls) : IObjEntity(cls) {}

  void setTm(const TMatrix &_tm) override {}
  void getTm(TMatrix &_tm) const override { _tm = TMatrix::IDENT; }
  void destroy() override {}
  BSphere3 getBsph() const override { return pool->getPools()[poolIdx]->getBsph(); }
  BBox3 getBbox() const override { return pool->getPools()[poolIdx]->getBbox(); }
  auto *getPool() { return pool && poolIdx < pool->getPools().size() ? pool->getPools()[poolIdx] : nullptr; }
  const auto *getPool() const { return pool && poolIdx < pool->getPools().size() ? pool->getPools()[poolIdx] : nullptr; }

  void *queryInterfacePtr(unsigned huid) override { return NULL; }

  const char *getObjAssetName() const override { return pool->getPools()[poolIdx]->getObjAssetName(); }

  Pool *pool = nullptr;
  unsigned idx = MAX_ENTITIES;
  unsigned poolIdx = MAX_POOLS;

  enum
  {
    MAX_POOLS = 0xFFFFu,
    MAX_ENTITIES = 0x7FFFFFFFu
  };
};
