//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gameRes/dag_gameResProxyTable.h>
#include <gameRes/dag_gameResSystem.h>
#include <osApiWrappers/dag_critSec.h>

struct ProxyGameResFactory : public GameResourceFactory
{
  ProxyGameResFactory(const GameResProxyTable &ptable, unsigned cls_id, bool allow_free_unused) : resClsId(cls_id)
  {
    proxyF = ptable.find_gameres_factory(resClsId);
    if (allow_free_unused)
      proxyGetCS = ptable.get_gameres_main_cs;
  }
  ~ProxyGameResFactory() override {}

  unsigned getResClassId() override { return resClsId; }
  const char *getResClassName() override { return proxyF->getResClassName(); }

  bool isResLoaded(int res_id) override { return proxyF->isResLoaded(res_id); }
  bool checkResPtr(GameResource *res) override { return proxyF->checkResPtr(res); }

  GameResource *getGameResource(RRL /*rrl*/, int res_id) override { return proxyF->getGameResource(nullptr, res_id); }

  bool addRefGameResource(GameResource *res) override { return proxyF->addRefGameResource(res); }
  void releaseGameResource(int res_id) override { proxyF->releaseGameResource(res_id); }
  bool releaseGameResource(GameResource *res) override { return proxyF->releaseGameResource(res); }

  bool freeUnusedResources(RRL rrl, bool ff, bool once) override
  {
    if (!proxyGetCS)
    {
      debug("skipped %s(rrl=%p, ff=%d, once=%d) for proxy clsID=0x%x", __FUNCTION__, rrl, ff, once, resClsId);
      return false;
    }
    WinAutoLock lock(proxyGetCS());
    return proxyF->freeUnusedResources(rrl, ff, once);
  }

  void loadGameResourceData(int, IGenLoad &) override { G_ASSERT(0); }
  void createGameResource(RRL, int, const int *, int) override { G_ASSERT(0); }

  void reset() override {}

  void dumpResourcesRefCount() override {}
  GameResource *discardOlderResAfterUpdate(int res_id) override { return proxyF->discardOlderResAfterUpdate(res_id); }

  static GameResourceFactory *create_and_add_proxy_factory(const GameResProxyTable &ptable, unsigned cls_id, bool allow_free_unused)
  {
    if (!ptable.apiVersion || find_gameres_factory(cls_id))
      return nullptr;

    auto *f = new ProxyGameResFactory(ptable, cls_id, allow_free_unused);
    if (!f->proxyF)
    {
      delete f;
      return nullptr;
    }
    add_factory(f);
    return f;
  }

protected:
  unsigned resClsId = 0;
  GameResourceFactory *proxyF = nullptr;
  WinCritSec &(*proxyGetCS)() = nullptr;
};
