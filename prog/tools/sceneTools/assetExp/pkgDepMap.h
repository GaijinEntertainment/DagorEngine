// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assets/assetRefs.h>
#include <util/dag_bitArray.h>
#include <util/dag_oaHashNameMap.h>
#include "daBuild.h"

struct PkgDepMap
{
  FastNameMapEx nm;
  Bitarray dep;
  int stride;
  static constexpr unsigned PACK_IDX_MASK = 0x1FFFu;
  static constexpr unsigned NON_EXPORT_BIT = 0x2000u;

  PkgDepMap(const DataBlock &pkgBlk)
  {
    stride = 0;
    nm.addNameId("*");
    for (int i = 0; i < pkgBlk.blockCount(); i++)
    {
      nm.addNameId(pkgBlk.getBlock(i)->getBlockName());
      if (const DataBlock *depBlk = pkgBlk.getBlock(i)->getBlockByName("dependencies"))
        for (int j = 0; j < depBlk->blockCount(); j++)
          nm.addNameId(depBlk->getBlock(j)->getBlockName());
    }

    rebuildMap(pkgBlk);
  }

  void rebuildMap(const DataBlock &pkgBlk)
  {
    if (stride == nm.nameCount())
      return;
    stride = nm.nameCount();
    dep.resize(stride * stride);
    for (int i = 0; i < nm.nameCount(); i++)
      dep.set(i * stride + i);

    for (int i = 0; i < pkgBlk.blockCount(); i++)
    {
      int id = nm.getNameId(pkgBlk.getBlock(i)->getBlockName());
      if (const DataBlock *depBlk = pkgBlk.getBlock(i)->getBlockByName("dependencies"))
        for (int j = 0; j < depBlk->blockCount(); j++)
          dep.set(id * stride + nm.getNameId(depBlk->getBlock(j)->getBlockName()));
    }

    for (int pass = 0; pass < nm.nameCount(); pass++)
    {
      bool map_changed = false;
      for (int i = 0; i < nm.nameCount(); i++)
        for (int j = 0; j < nm.nameCount(); j++)
          if (i != j && dep[i * stride + j])
            for (int k = 0; k < nm.nameCount(); k++)
              if (k != i && k != j && dep[j * stride + k] && !dep[i * stride + k])
              {
                dep.set(i * stride + k);
                map_changed = true;
              }

      if (!map_changed)
        break;
    }
  }

  void dumpDepMap()
  {
    for (int i = 0; i < nm.nameCount(); i++)
    {
      debug_("pkg[%d] %s dep: ", i, nm.getName(i));
      for (int j = 0; j < nm.nameCount(); j++)
        if (i != j && dep[i * stride + j])
          debug_(" %s", nm.getName(j));
      debug("");
    }
  }

  bool setPkgToUserFlags(DagorAsset *a, const char *pkg, bool allow_add = false)
  {
    a->resetUserFlags();
    if (!pkg || strcmp(pkg, "*") == 0)
      return true;

    a->setUserFlags((allow_add && nm.nameCount() < PACK_IDX_MASK) ? nm.addNameId(pkg) : nm.getNameId(pkg));
    return a->testUserFlags(PACK_IDX_MASK) != PACK_IDX_MASK;
  }
  void resetUserFlags(DagorAssetMgr &mgr)
  {
    dag::ConstSpan<DagorAsset *> assets = mgr.getAssets();
    for (int i = 0; i < assets.size(); i++)
      assets[i]->resetUserFlags();
  }

  bool validateAssetDeps(DagorAssetMgr &mgr, const DataBlock &expblk, const FastNameMap &pkg_list, unsigned targetCode,
    const char *profile, ILogWriter &log, bool strict)
  {
    String ts(mkbindump::get_target_str(targetCode));
    dag::ConstSpan<DagorAsset *> assets = mgr.getAssets();
    Tab<bool> expTypesMask(tmpmem);
    Tab<AssetPack *> tex_pack, res_pack;
    Tab<SimpleString> src_files;
    FastNameMapEx addPackages;
    Bitarray dep0;
    int err_cnt = 0;

    if (pkg_list.nameCount())
    {
      dep0.resize(stride);
      iterate_names(pkg_list, [&](int, const char *name) {
        int id = nm.getNameId(name);
        if (id >= 0)
          dep0.set(id);
      });
    }

    // mark all as not exported
    for (int i = 0; i < assets.size(); i++)
      assets[i]->setUserFlags(NON_EXPORT_BIT);

    make_exp_types_mask(expTypesMask, mgr, expblk, log);
    preparePacks(mgr, mgr.getAssets(), expTypesMask, expblk, tex_pack, res_pack, addPackages, log, true, true, ts, profile);

    // clear marks for exported assets
    Tab<DagorAsset *> tmp_list(tmpmem);
    for (int i = 0; i < tex_pack.size(); i++)
    {
      if (!tex_pack[i]->exported)
        continue;
      tmp_list.clear();
      if (!get_exported_assets(tmp_list, tex_pack[i]->assets, ts, profile))
        continue;
      for (int j = 0; j < tmp_list.size(); j++)
        tmp_list[j]->clrUserFlags(NON_EXPORT_BIT);
    }
    for (int i = 0; i < res_pack.size(); i++)
    {
      if (!res_pack[i]->exported)
        continue;
      tmp_list.clear();
      if (!get_exported_assets(tmp_list, res_pack[i]->assets, ts, profile))
        continue;
      for (int j = 0; j < tmp_list.size(); j++)
        tmp_list[j]->clrUserFlags(NON_EXPORT_BIT);
    }

    // validate references
    for (int i = 0; i < assets.size(); i++)
    {
      bool exportable = expTypesMask[assets[i]->getType()];
      if (exportable && assets[i]->testUserFlags(NON_EXPORT_BIT)) // exportable but not exported, skip
        continue;
      if (!exportable && !strict)
        continue;

      int pkg = assets[i]->testUserFlags(PACK_IDX_MASK);
      if (pkg == PACK_IDX_MASK)
        continue;
      if (dep0.size() && !dep0.get(pkg))
        continue;

      if (IDagorAssetRefProvider *refProvider = mgr.getAssetRefProvider(assets[i]->getType()))
      {
        dag::ConstSpan<IDagorAssetRefProvider::Ref> refs = refProvider->getAssetRefsEx(*assets[i], targetCode, profile);
        for (int j = 0; j < refs.size(); j++)
        {
          ILogWriter::MessageType l = log.NOTE;
          bool opt_ref = (refs[j].flags & refProvider->RFLG_OPTIONAL);
          bool ext_ref = (refs[j].flags & refProvider->RFLG_EXTERNAL) ||
                         (exportable && refs[j].getAsset() && refs[j].getAsset()->getType() == mgr.getTexAssetTypeId());

          if (refs[j].flags & refProvider->RFLG_BROKEN)
            log.addMessage(l = ((opt_ref && !strict) ? log.WARNING : log.ERROR), "asset %s:%s has %s broken ref[%d]=<%s>",
              assets[i]->getName(), assets[i]->getTypeStr(), opt_ref ? "optional" : "mandatory", j, refs[j].getBrokenRef());
          else if (!refs[j].getAsset())
          {
            if (!opt_ref)
              log.addMessage(l = log.ERROR, "asset %s:%s missing mandatory ref[%d]", assets[i]->getName(), assets[i]->getTypeStr(), j);
          }
          else
          {
            int ref = refs[j].getAsset()->testUserFlags(PACK_IDX_MASK);
            if (ref == PACK_IDX_MASK)
              continue;
            if (!dep.get(pkg * stride + ref))
              log.addMessage(l = (ext_ref ? log.ERROR : log.WARNING),
                "asset %s:%s pkg=\"%s\" doesn't depend on \"%s\" for ref[%d]=<%s:%s>", assets[i]->getName(), assets[i]->getTypeStr(),
                nm.getName(pkg), nm.getName(ref), j, refs[j].getAsset()->getName(), refs[j].getAsset()->getTypeStr());
            else if (refs[j].getAsset()->testUserFlags(NON_EXPORT_BIT) && (ext_ref || expTypesMask[refs[j].getAsset()->getType()]))
              log.addMessage(l = (ext_ref ? log.ERROR : log.WARNING), "asset %s:%s ref[%d]=<%s:%s> is not exported",
                assets[i]->getName(), assets[i]->getTypeStr(), j, refs[j].getAsset()->getName(), refs[j].getAsset()->getTypeStr());
          }
          if (l == log.ERROR)
            err_cnt++;
        }
      }
      if (IDagorAssetExporter *exp = mgr.getAssetExporter(assets[i]->getType()))
      {
        ILogWriter::MessageType l = log.NOTE;
        src_files.clear();
        exp->gatherSrcDataFiles(*assets[i], src_files);
        for (const auto &s : src_files)
          if (!dd_file_exists(s))
            log.addMessage(l = log.ERROR, "asset %s:%s missing source file: %s", assets[i]->getName(), assets[i]->getTypeStr(), s);
        if (l == log.ERROR)
          err_cnt++;
      }
    }
    return err_cnt == 0;
  }
};
