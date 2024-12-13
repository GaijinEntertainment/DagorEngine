// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daBuild.h"
#include <libTools/util/conLogWriter.h>
#include <libTools/util/progressInd.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/binDumpReader.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/assetExpCache.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_direct.h>

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_debug.h>
#include <time.h>
#include <sys/stat.h>
#include <perfMon/dag_cpuFreq.h>

int dabuild_grp_write_ver = 3;

static void make_start_ts(String &build_ts)
{
  time_t rawtime;
  tm *t;

  time(&rawtime);
  rawtime -= get_time_msec() / 1000;
  t = localtime(&rawtime);
  build_ts.printf(64, "%04d.%02d.%02d-%02d.%02d.%02d", 1900 + t->tm_year, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
}

class GrpExporter
{
public:
  struct ResData
  {
    int nameId, classId, realResNameId, order;
    SmallTab<int, TmpmemAlloc> refNameIds;

    static int cmp(const ResData *a, const ResData *b)
    {
      if (a->order == b->order)
        return dd_stricmp(cmpMgr->getAssetName(a->nameId), cmpMgr->getAssetName(b->nameId));
      return a->order - b->order;
    }
    static DagorAssetMgr *cmpMgr;
  };

  struct RealResData
  {
    IDagorAssetExporter *exp;
    DagorAsset *asset;

    int realResNameId;

    int nameId; // for logging

    int headerOffset, dataOffset;


    RealResData(IDagorAssetExporter *p, DagorAsset *a, int rid, int nid) :
      exp(p), asset(a), realResNameId(rid), nameId(nid), headerOffset(0), dataOffset(0)
    {}
  };


  Tab<ResData> resData;
  Tab<RealResData> realResData;
  Tab<int> tmpInt;

  DagorAssetMgr &mgr;
  int texType;


  GrpExporter(DagorAssetMgr &db) : mgr(db), resData(tmpmem), realResData(tmpmem), tmpInt(tmpmem) { texType = mgr.getTexAssetTypeId(); }


  bool addResource(DagorAsset &res, IDagorAssetExporter *exp, IDagorAssetRefProvider *refResv, ILogWriter &log)
  {
    if (!res.isGloballyUnique())
    {
      int nid = res.getNameId();
      bool unique = true;
      for (DagorAsset *a : res.getMgr().getAssets())
        if (a != &res && a->getNameId() == nid && a->getType() != mgr.getTexAssetTypeId())
          if (auto *e2 = res.getMgr().getAssetExporter(a->getType()))
            if (e2->isExportableAsset(*a) && a->props.getBool("export", true))
            {
              if (unique)
              {
                log.addMessage(ILogWriter::ERROR, "%s: asset is not unique", res.getNameTypified());
                unique = false;
              }
              log.addMessage(ILogWriter::ERROR, "  ^^^ asset nameclash with %s", a->getNameTypified());
            }
      if (!unique)
        return false;
    }

    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].nameId == res.getNameId())
        return true;

    Tab<IDagorAssetRefProvider::Ref> refList;
    if (refResv)
      refList = refResv->getAssetRefs(res);

    ResData &rd = resData.push_back();

    rd.nameId = res.getNameId();
    rd.classId = exp->getGameResClassId();
    rd.realResNameId = res.getNameId();

    tmpInt.clear();
    tmpInt.reserve(refList.size());

    bool success = true;
    for (int i = 0; i < refList.size(); ++i)
    {
      if (refList[i].flags & refResv->RFLG_BROKEN)
      {
        if (refList[i].flags & refResv->RFLG_OPTIONAL)
          log.addMessage(ILogWriter::WARNING, "%s: optional ref <%s> is broken", res.getName(), refList[i].getBrokenRef());
        else
        {
          log.addMessage(ILogWriter::ERROR, "%s: required ref <%s> is broken", res.getName(), refList[i].getBrokenRef());
          success = false;
        }
        continue;
      }

      if (!(refList[i].flags & refResv->RFLG_EXTERNAL))
        continue;

      if (refList[i].refAsset)
      {
        // check that external ref is to be exported too
        IDagorAssetExporter *e2 = mgr.getAssetExporter(refList[i].refAsset->getType());
        if ((e2 && !e2->isExportableAsset(*refList[i].refAsset)) || (!e2 && refList[i].refAsset->getType() != texType))
        {
          log.addMessage(ILogWriter::ERROR, "%s: external ref <%s> is not exportable", res.getName(),
            refList[i].refAsset->getNameTypified());
          success = false;
        }
      }
      tmpInt.push_back(refList[i].refAsset ? refList[i].refAsset->getNameId() : -1);
    }
    rd.refNameIds = tmpInt;

    return success;
  }


  bool addRealResource(DagorAsset &res, IDagorAssetExporter *exp, IDagorAssetRefProvider *refResv, ILogWriter &log)
  {
    for (int i = 0; i < realResData.size(); ++i)
      if (realResData[i].exp == exp && realResData[i].realResNameId == res.getNameId())
        return true;

    realResData.push_back(RealResData(exp, &res, res.getNameId(), res.getNameId()));

    return true;
  }

  void reorderResData(ILogWriter &log)
  {
    int left = resData.size();
    Tab<int> xmap(tmpmem);

    for (int i = 0; i < resData.size(); ++i)
    {
      ResData &rd = resData[i];
      rd.order = -1;
      if (rd.nameId >= xmap.size())
      {
        int base = xmap.size();
        xmap.resize(rd.nameId + 1);
        for (int j = base; j < (int)xmap.size() - 1; j++)
          xmap[j] = -1;
      }
      xmap[rd.nameId] = i;

      for (int j = 0; j < rd.refNameIds.size(); ++j)
        if (rd.refNameIds[j] >= 0 && rd.refNameIds[j] == resData[i].nameId)
          log.addMessage(log.ERROR, "recursive dependance in asset %s (ref[%d] is %s)", mgr.getAssetName(rd.nameId), j,
            mgr.getAssetName(rd.refNameIds[j]));
    }

    while (left > 0)
    {
      int last_left = left;
      for (int i = 0; i < resData.size(); ++i)
      {
        ResData &rd = resData[i];
        int order = 0;
        if (rd.order != -1)
          continue;

        for (int j = 0; j < rd.refNameIds.size(); ++j)
          if (rd.refNameIds[j] >= 0 && rd.refNameIds[j] < xmap.size() && xmap[rd.refNameIds[j]] != -1)
          {
            int o2 = resData[xmap[rd.refNameIds[j]]].order;
            if (o2 == -1)
            {
              order = -1;
              break;
            }
            o2++;

            if (o2 > order)
              order = o2;
          }

        if (order != -1)
        {
          // debug ( "%d: %s - order %d", i, mgr.getAssetName(rd.nameId), order );
          rd.order = order;
          left--;
        }
      }
      if (last_left == left)
      {
        logwarn("reorderResData() deadloop, left=%d, resData=%d", left, resData.size());
        /*
        for (int i=0; i<resData.size(); ++i)
        {
          ResData &rd=resData[i];
          debug("%d: %s -> %d", i, mgr.getAssetName(rd.nameId), rd.order);
          for (int j=0; j<rd.refNameIds.size(); ++j)
            if (rd.refNameIds[j] >= 0)
              debug("  ref[%d] %d [%d]",
                j, rd.refNameIds[j], rd.refNameIds[j] < xmap.size() ? xmap[rd.refNameIds[j]] : -1);
        }
        */
        break;
      }
    }

    ResData::cmpMgr = &mgr;
    sort(resData, &ResData::cmp);
    ResData::cmpMgr = NULL;
  }

  bool saveFile(mkbindump::BinDumpSaveCB &cwr, AssetExportCache &c4, file_ptr_t fp, ILogWriter &log, IGenericProgressIndicator &pbar,
    const char *pack_fname, const DataBlock &blk_export_props)
  {
    using namespace mkbindump;
    OAHashNameMap<true> nm;
    PatchTabRef nm_pt, rrt_pt, rd_pt;
    SharedStorage<uint16_t> refIds;
    Tab<Ref> refs(tmpmem);
    Tab<uint16_t> tmpRefs(tmpmem);
    Tab<int> nameMapOfs(tmpmem);
    Tab<char> strData(tmpmem);
    int desc_sz = 0, data_sz = 0;
    int build_errors = 0;

    reorderResData(log);

    for (int i = 0; i < realResData.size(); ++i)
    {
      RealResData &rrd = realResData[i];
      nm.addNameId(mgr.getAssetName(rrd.realResNameId));
    }

    refs.resize(resData.size());
    for (int i = 0; i < resData.size(); ++i)
    {
      ResData &rd = resData[i];

      nm.addNameId(mgr.getAssetName(rd.nameId));
      nm.addNameId(mgr.getAssetName(rd.realResNameId));

      tmpRefs.resize(rd.refNameIds.size());
      for (int j = 0; j < rd.refNameIds.size(); ++j)
        if (rd.refNameIds[j] == -1)
          tmpRefs[j] = 0xFFFF;
        else
          tmpRefs[j] = nm.addNameId(mgr.getAssetName(rd.refNameIds[j]));
      refIds.getRef(refs[i], tmpRefs.data(), tmpRefs.size());
      // debug ( "+%d: %s - order %d", i, mgr.getNameMap().getName(rd.nameId), rd.order );
    }

    nameMapOfs.resize(nm.nameCount());
    for (int i = 0; i < nameMapOfs.size(); i++)
      nameMapOfs[i] = append_items(strData, strlen(nm.getStringDataUnsafe(i)) + 1, nm.getStringDataUnsafe(i));

    cwr.writeFourCC(MAKE4C('G', 'R', 'P', '0' + dabuild_grp_write_ver));
    cwr.writeInt32e(0);
    cwr.writeInt32e(0);
    cwr.beginBlock();

    // prepare binary dump header (root data)
    nm_pt.reserveTab(cwr);  //  PatchableTab<int> nameMap;
    rrt_pt.reserveTab(cwr); //  PatchableTab<ResEntry> resTable;
    rd_pt.reserveTab(cwr);  //  PatchableTab<ResData> resData;

    // write name map
    for (int i = 0; i < nameMapOfs.size(); i++)
      nameMapOfs[i] += cwr.tell();
    cwr.writeTabDataRaw(strData);

    cwr.align16();
    nm_pt.startData(cwr, nameMapOfs.size());
    cwr.writeTabData32ex(nameMapOfs);
    nm_pt.finishTab(cwr);

    // write real-res table
    rrt_pt.startData(cwr, realResData.size());
    for (int i = 0; i < realResData.size(); ++i)
    {
      RealResData &rrd = realResData[i];

      // ResEntry:
      //   uint32_t classId;
      //   uint32_t offset;
      //   uint16_t resId, _resv;
      cwr.writeInt32e(rrd.exp->getGameResClassId());

      rrd.headerOffset = cwr.tell();
      cwr.writeInt32e(0);

      cwr.writeInt16e(nm.getNameId(mgr.getAssetName(rrd.realResNameId)));
      cwr.writeInt16e(0);
    }
    rrt_pt.finishTab(cwr);

    cwr.align16();
    rd_pt.reserveData(cwr, resData.size(), 8 + (dabuild_grp_write_ver == 2 ? cwr.TAB_SZ : 8));
    rd_pt.finishTab(cwr);
    desc_sz = cwr.tell() - 16;

    cwr.writeStorage16e(refIds);
    data_sz = cwr.tell() - 16;
    cwr.align16();
    int data_pos = cwr.tell();

    cwr.seekto(rd_pt.resvDataPos);
    for (int i = 0; i < resData.size(); ++i)
    {
      ResData &rd = resData[i];

      refIds.refIndexToOffset(refs[i]);

      // ResData:
      //   uint32_t classId;
      // V2:                                   V3:
      //   uint16_t resId, _resv;                uint16_t resId, refResIdCnt;
      //   PatchableTab<uint16_t> refResId;      PatchablePtr<uint16_t> refResIdPtr;
      cwr.writeInt32e(rd.classId);
      cwr.writeInt16e(nm.getNameId(mgr.getAssetName(rd.nameId)));
      if (dabuild_grp_write_ver == 2)
      {
        cwr.writeInt16e(nm.getNameId(mgr.getAssetName(rd.realResNameId)));
        cwr.writeRef(refs[i]);
      }
      else
      {
        cwr.writeInt16e(refs[i].count);
        cwr.writeInt64e(refs[i].start);
      }
    }
    cwr.seekto(data_pos);

    pbar.setActionDesc(String(256, "%sExporting gameRes pack: %s...", dabuild_progress_prefix_text, pack_fname));
    pbar.setDone(0);
    pbar.setTotal(realResData.size());

    if (log.hasErrors())
    {
      log.addMessage(ILogWriter::ERROR, "Errors while exporting gameres pack \"%s\"", pack_fname);
      return false;
    }

    // write real-res data
    for (int i = 0; i < realResData.size(); ++i)
    {
      RealResData &rrd = realResData[i];
      String nameTypified(rrd.asset->getNameTypified());
      pbar.incDone();

      cwr.align16();
      int dataOfs = cwr.tell();

      int data_ofs, data_len = 0;

      cwr.setOrigin();
      if (fp && c4.getAssetDataPos(nameTypified, data_ofs, data_len))
      {
        df_seek_to(fp, data_ofs);
        LFileGeneralLoadCB crd(fp);
        copy_stream_to_stream(crd, cwr.getRawWriter(), data_len);
      }
      else
      {
        static const int CMP_ATTEMPTS_COUNT = 9;
        bool comparison_passed = true;
        int checks_count = blk_export_props.getBlockByNameEx(rrd.asset->getTypeStr())->getInt("checks", 0);

        debug("export %s (%d checks)", nameTypified, checks_count);
        for (int attempt = 0; attempt < CMP_ATTEMPTS_COUNT; attempt++)
        {
          mkbindump::BinDumpSaveCB cwr0(1 << 20, cwr.getTarget(), cwr.WRITE_BE);
          cwr0.setProfile(cwr.getProfile());
          comparison_passed = true;
          for (int check = 0; check < checks_count + 1; check++)
          {
            mkbindump::BinDumpSaveCB cwr1(1 << 20, cwr.getTarget(), cwr.WRITE_BE);
            cwr1.setProfile(cwr.getProfile());
            FATAL_CONTEXT_AUTO_SCOPE(nameTypified);
            if (!rrd.exp->exportAsset(*rrd.asset, check == 0 ? cwr0 : cwr1, log) || log.hasErrors())
            {
              log.addMessage(ILogWriter::ERROR, "Errors while exporting asset \"%s\"", mgr.getAssetName(rrd.nameId));

              build_errors++;
              if (!dabuild_stop_on_first_error)
                goto finalize_label;
              cwr.popOrigin();
              return false;
            }

            if (check > 0)
              if (!cwr0.getMem()->cmpEq(cwr1.getMem()))
              {
                log.addMessage(ILogWriter::WARNING, "Check %d (attempt %d) failed while exporting asset \"%s\"", check, attempt + 1,
                  mgr.getAssetName(rrd.nameId));

                const char *sp = pack_fname;
                while (strchr("./:\\", *sp))
                  sp++;

                String packFn(sp);
                for (int c = 0, ce = (int)strlen(packFn); c < ce; c++)
                  if (strchr("/:\\", packFn[c]))
                    packFn[c] = '_';

                static String build_ts;
                if (build_ts.empty())
                  make_start_ts(build_ts);

                debug("pack_fname=<%s> <%s> <%s>", packFn.str(), sp, pack_fname);
                String copy0(260, ".dabuild_check_failures/%s/%s/%s.a%d.bin", build_ts.str(), packFn.str(), rrd.asset->getName(),
                  attempt + 1);
                String copy1(260, ".dabuild_check_failures/%s/%s/%s.a%dc%d.bin", build_ts.str(), packFn.str(), rrd.asset->getName(),
                  attempt + 1, check);

                dd_mkpath(copy0);
                FullFileSaveCB fcwr0(copy0);
                cwr0.copyDataTo(fcwr0);
                fcwr0.close();
                dd_mkpath(copy1);
                FullFileSaveCB fcwr1(copy1);
                cwr1.copyDataTo(fcwr1);
                fcwr1.close();

                comparison_passed = false;
                break;
              }
          }

          if (comparison_passed)
          {
            cwr0.copyDataTo(cwr.getRawWriter());
            break;
          }
        }

        if (!comparison_passed)
        {
          log.addMessage(ILogWriter::ERROR, "Failed %d comparison attempts while exporting asset \"%s\"", CMP_ATTEMPTS_COUNT,
            mgr.getAssetName(rrd.nameId));

          build_errors++;
          if (!dabuild_stop_on_first_error)
            goto finalize_label;
          cwr.popOrigin();
          return false;
        }

        Tab<SimpleString> a_files(tmpmem);
        rrd.exp->gatherSrcDataFiles(*rrd.asset, a_files);
        for (int j = 0; j < a_files.size(); j++)
          c4.updateFileHash(a_files[j]);
      }
      data_len = cwr.tell();
      c4.setAssetDataPos(nameTypified, dataOfs, data_len);
    finalize_label:
      cwr.popOrigin();

      rrd.dataOffset = dataOfs;
    }
    if (build_errors)
      return false;

    // patch real-res data offsets in header
    for (int i = 0; i < realResData.size(); ++i)
    {
      RealResData &rrd = realResData[i];

      if (rrd.dataOffset == 0)
        continue;

      cwr.seekto(rrd.headerOffset);
      cwr.writeInt32e(rrd.dataOffset);
    }

    cwr.seekto(4);
    cwr.writeInt32e(desc_sz);
    cwr.writeInt32e(data_sz);
    cwr.seekToEnd();
    cwr.endBlock();

    return true;
  }
};

DagorAssetMgr *GrpExporter::ResData::cmpMgr = NULL;

bool buildGameResPack(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4, const char *pack_fname,
  ILogWriter &log, IGenericProgressIndicator &pbar, bool &up_to_date, const DataBlock &blk_export_props)
{
  if (dabuild_skip_any_build)
    return up_to_date = dd_file_exist(pack_fname);

  up_to_date = false;

  GrpExporter exp(assets[0]->getMgr());
  bool ok = true;

  bool changed = false;
  Tab<SimpleString> a_files(tmpmem);

  for (int i = 0; i < assets.size(); ++i)
  {
    DagorAsset &a = *assets[i];
    String asset_name(a.getNameTypified());
    bool a_changed = false;

    IDagorAssetExporter *e = exp.mgr.getAssetExporter(a.getType());
    G_ASSERT(e);
    G_ANALYSIS_ASSUME(e);

    if (assets[i]->isVirtual())
      if (c4.checkFileChanged(assets[i]->getTargetFilePath()))
        a_changed = true;
    if (c4.checkDataBlockChanged(asset_name, assets[i]->props))
      a_changed = true;

    if (c4.checkAssetExpVerChanged(a.getType(), e->getGameResClassId(), e->getGameResVersion()))
      a_changed = true;

    IDagorAssetRefProvider *refResv = exp.mgr.getAssetRefProvider(a.getType());
    if (!exp.addRealResource(a, e, refResv, log))
      ok = false;
    if (!exp.addResource(a, e, refResv, log))
      ok = false;

    e->gatherSrcDataFiles(a, a_files);
    for (int j = 0; j < a_files.size(); j++)
      if (c4.checkFileChanged(a_files[j]))
        a_changed = true;

    if (a_changed || !c4.getAssetDataPos(asset_name))
    {
      c4.setAssetDataPos(asset_name, -1, 0);
      changed = true;
    }
  }

  for (int i = 0; i < assets.size(); ++i)
  {
    DagorAsset &a = *assets[i];
    IDagorAssetExporter *e = exp.mgr.getAssetExporter(a.getType());
    if (e)
      c4.setAssetExpVer(a.getType(), e->getGameResClassId(), e->getGameResVersion());
  }
  if (c4.resetExtraAssets(assets))
    changed = true;

  if (!ok)
  {
    log.addMessage(ILogWriter::ERROR, "There were errors adding resources to packer");
    return false;
  }

  file_ptr_t prev_pack_fp = NULL;
  if (!c4.checkTargetFileChanged(pack_fname))
  {
    prev_pack_fp = df_open(pack_fname, DF_READ);
    if (prev_pack_fp)
    {
      unsigned label = 0;
      df_read(prev_pack_fp, &label, 4);
      if (label != MAKE4C('G', 'R', 'P', '0' + dabuild_grp_write_ver))
        changed = true;
    }
    if (prev_pack_fp && !changed && c4.checkAllTouched())
    {
      log.addMessage(log.NOTE, "skip up-to-date %s", pack_fname);
      up_to_date = true;
      df_close(prev_pack_fp);
      return true;
    }
  }
  c4.removeUntouched();

  if (!exp.saveFile(cwr, c4, prev_pack_fp, log, pbar, pack_fname, blk_export_props))
  {
    if (prev_pack_fp)
      df_close(prev_pack_fp);
    return false;
  }

  if (prev_pack_fp)
    df_close(prev_pack_fp);

  if (cmp_data_eq(cwr, pack_fname))
  {
    log.addMessage(log.NOTE, "built %s: %d Kb, %d resources; binary remains the same\n", pack_fname, cwr.getSize() >> 10,
      assets.size());
    stat_grp_built++;
    stat_grp_unchanged++;
    c4.setTargetFileHash(pack_fname);
    return true;
  }

  stat_grp_sz_diff -= get_file_sz(pack_fname);
  stat_grp_sz_diff += cwr.getSize();
  stat_changed_grp_total_sz += cwr.getSize();

  if (dabuild_dry_run)
  {
    log.addMessage(log.NOTE, "built(dry) %s: %d Kb, %d resources\n", pack_fname, cwr.getSize() >> 10, assets.size());
    stat_grp_built++;
    return true;
  }

  FullFileSaveCB fcwr(pack_fname);

  if (!fcwr.fileHandle)
  {
    log.addMessage(ILogWriter::ERROR, "Can't create GRP file %s", pack_fname);
    return false;
  }

  DAGOR_TRY { cwr.copyDataTo(fcwr); }
  DAGOR_CATCH(const IGenSave::SaveException &)
  {
    log.addMessage(ILogWriter::ERROR, "Error writing GRP file %s", pack_fname);
    dd_erase(pack_fname);
    return false;
  }
  fcwr.close();
  c4.setTargetFileHash(pack_fname);

  log.addMessage(log.NOTE, "built %s: %d Kb, %d resources\n", pack_fname, cwr.getSize() >> 10, assets.size());
  stat_grp_built++;
  return true;
}

bool checkGameResPackUpToDate(dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4, const char *pack_fname, int ch_bit)
{
  GrpExporter exp(assets[0]->getMgr());

  bool changed = false;
  Tab<SimpleString> a_files(tmpmem);

  for (int i = 0; i < assets.size(); ++i)
  {
    DagorAsset &a = *assets[i];
    String asset_name_typified(a.getNameTypified());
    int dataOfs = 0, dataLen = 0;

    IDagorAssetExporter *e = exp.mgr.getAssetExporter(a.getType());
    G_ASSERT(e);
    G_ANALYSIS_ASSUME(e);

    if (c4.checkAssetExpVerChanged(a.getType(), e->getGameResClassId(), e->getGameResVersion()))
      goto cur_changed;

    if (!c4.getAssetDataPos(asset_name_typified, dataOfs, dataLen))
      goto cur_changed;

    if (assets[i]->isVirtual())
      if (c4.checkFileChanged(assets[i]->getTargetFilePath()))
        goto cur_changed;
    if (c4.checkDataBlockChanged(asset_name_typified, assets[i]->props))
      goto cur_changed;

    e->gatherSrcDataFiles(a, a_files);
    // A lot of assets add the asset itself to the list of src data files.
    // checkFileChanged is quite slow and we checked this asset already above.
    // Removing this asset from gather list is faster then calling checkFileChanged one extra time
    if (a.isVirtual() && a_files.size() > 0 && strcmp(a_files[0].c_str(), assets[i]->getTargetFilePath().c_str()) == 0)
      a_files.erase(a_files.begin());
    for (int j = 0; j < a_files.size(); j++)
      if (c4.checkFileChanged(a_files[j]))
        goto cur_changed;


    assets[i]->setUserFlags(ch_bit);
    continue;

  cur_changed:
    assets[i]->clrUserFlags(ch_bit);
    changed = true;
  }
  if (!changed && c4.resetExtraAssets(assets))
    changed = true;

  return changed;
}
