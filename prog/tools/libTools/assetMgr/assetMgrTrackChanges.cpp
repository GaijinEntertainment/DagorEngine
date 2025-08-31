// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#define _WIN32_WINNT 0x400
#include <windows.h>
#undef ERROR

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetExporter.h>
#include <libTools/util/strUtil.h>
#include "assetMgrPrivate.h"
#include "assetCreate.h"
#include "vAssetRule.h"
#include <ioSys/dag_msgIo.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_fastIntList.h>
#include <util/dag_globDef.h>
#include <process.h>
#include <stdio.h>
#include <io.h>


struct DagorAssetMgr::ChangesTracker
{
  struct FolderData
  {
    static const int BUF_SZ = (32 << 10) - 20;
    char buf[BUF_SZ]; //-V730
    HANDLE hFolder = nullptr;
    ThreadSafeMsgIo *io = nullptr;
    SimpleString fname;
    int rootIdx = -1;
    volatile int terminateReq = 0;
  };
  Tab<FolderData *> f;
  ThreadSafeMsgIoEx io;
  static const int IO_BUF_SZ = 256 << 10;

public:
  ChangesTracker() : io(IO_BUF_SZ), f(midmem) {}
  ~ChangesTracker() { startMonitoring({}); }

  void addFolderMonitor(const char *foldername, int rootIdx)
  {
    if (rootIdx > 255)
      return;

    HANDLE hDir = CreateFile(foldername, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_LIST_DIRECTORY,
      NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (!hDir)
      return;

    FolderData *d = new (midmem) FolderData;
    d->hFolder = hDir;
    d->io = &io;
    d->fname = String(260, "%s/tmp.$$$", foldername);
    d->rootIdx = rootIdx;

    uintptr_t handle = _beginthread(monitorThread, 4096, d);
    if (handle == -1)
    {
      CloseHandle(d->hFolder);
      delete d;
      return;
    }

    f.push_back(d);
  }

  void removeFolderMonitor(int idx)
  {
    G_ASSERT(idx >= 0 && idx < f.size());
    interlocked_release_store(f[idx]->terminateReq, 1);

    if (FILE *fp = fopen(f[idx]->fname, "wb"))
    {
      fwrite("1", 1, 1, fp);
      fclose(fp);
      unlink(f[idx]->fname);

      while (interlocked_acquire_load_ptr(f[idx]->hFolder))
        Sleep(10);
    }
    else if (interlocked_acquire_load_ptr(f[idx]->hFolder))
      DAG_FATAL("cannot create file: %s hFolder=%p", f[idx]->fname.str(), f[idx]->hFolder);
    erase_ptr_items(f, idx, 1);
  }

  void startMonitoring(dag::ConstSpan<RootEntryRec> roots)
  {
    for (int i = f.size() - 1; i >= 0; i--)
      removeFolderMonitor(i);
    for (int i = 0; i < roots.size(); i++)
      addFolderMonitor(roots[i].folder, i);
  }

  static void __cdecl monitorThread(void *p)
  {
    FolderData &fd = *(FolderData *)p;

    while (!interlocked_acquire_load(fd.terminateReq))
    {
      FILE_NOTIFY_INFORMATION *pNotify;
      DWORD offset = 0;
      TCHAR szFile[MAX_PATH];
      DWORD bytesret;

      if (!ReadDirectoryChangesW(fd.hFolder, fd.buf, fd.BUF_SZ, TRUE,
            FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesret, NULL, NULL))
        continue;

      if (interlocked_acquire_load(fd.terminateReq))
        break;

      G_STATIC_ASSERT(IO_BUF_SZ >= fd.BUF_SZ * 3);
      while (fd.io->getWriteAvailableSize() < fd.BUF_SZ * 3)
      {
        sleep_msec(10);
        logwarn("waited for ChangesTracker buffer processed, avail=%dK", fd.io->getWriteAvailableSize() >> 10);
      }

      int avail_sz = fd.io->getWriteAvailableSize();
      IGenSave *cwr = fd.io->startWrite();
      do
      {
        pNotify = (FILE_NOTIFY_INFORMATION *)(fd.buf + offset);
        offset += pNotify->NextEntryOffset;

        int count =
          WideCharToMultiByte(CP_ACP, 0, pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR), szFile, MAX_PATH - 1, NULL, NULL);
        szFile[count] = TEXT('\0');

        if (avail_sz < cwr->tell() + count + 32)
        {
          logerr("insufficient ChangesTracker buffer, avail=%d pos=%d, break...", avail_sz, cwr->tell());
          break;
        }
        cwr->beginBlock();
        cwr->writeInt(get_time_msec_qpc());
        cwr->writeIntP<1>(fd.rootIdx);
        cwr->writeIntP<1>(pNotify->Action);
        cwr->writeIntP<2>(count);
        cwr->write(szFile, count);
        cwr->alignOnDword(count + 2);
        cwr->endBlock();

      } while (pNotify->NextEntryOffset != 0);
      fd.io->endWrite();
    }

    CloseHandle(fd.hFolder);
    interlocked_release_store_ptr(fd.hFolder, (HANDLE) nullptr);
  }
};

void DagorAssetMgr::enableChangesTracker(bool en)
{
  if (!en)
  {
    del_it(tracker);
    return;
  }
  if (tracker)
    return;

  tracker = new (midmem) ChangesTracker;
  syncTracker();
  tracker->startMonitoring(baseRoots);
}

bool DagorAssetMgr::rescanBase() { return false; }

static int detectFolder(dag::ConstSpan<DagorAssetFolder *> f, const char *root, char *fname, char *fdir)
{
  dd_simplify_fname_c(fname);
  strcpy(fdir, root);
  char *folderName = dd_get_fname_location(fdir + strlen(fdir) + 1, fname);
  folderName[-1] = '/';
  if (folderName[strlen(folderName) - 1] == '/')
    folderName[strlen(folderName) - 1] = '\0';
  folderName = (char *)dd_get_fname(fdir);

  for (int i = 0; i < f.size(); i++)
    if (dd_stricmp(f[i]->folderName, folderName) == 0 && dd_stricmp(f[i]->folderPath, fdir) == 0)
      return i;
  return -1;
}

bool DagorAssetMgr::trackChangesContinuous(int assets_to_check)
{
  if (!tracker)
    return false;

  int msg_count;
  IGenLoad *crd = tracker->io.startRead(msg_count);

  FastIntList changed_asset_idx, sec_changed_asset_idx;
  Tab<DagorAsset *> added_assets(tmpmem), removed_assets(tmpmem);
  int last_fidx = -1, last_file_id = -1, last_msec = 0;
  char fname[260], fdir[520];

  Tab<SimpleString> files(tmpmem);

  for (; msg_count > 0; msg_count--)
  {
    crd->beginBlock();
    int time_msec = crd->readInt();
    int rootIdx = crd->readIntP<1>();
    int type = crd->readIntP<1>();
    int len = crd->readIntP<2>();
    if (len >= sizeof(fname))
      len = sizeof(fname) - 1;
    crd->read(fname, len);
    fname[len] = '\0';
    crd->endBlock();

    int fidx = detectFolder(folders, baseRoots[rootIdx].folder, fname, fdir);
    if (fidx == -1)
      continue;
    const char *fn = dd_get_fname(fname);
    int a_start, a_end;
    getFolderAssetIdxRange(fidx, a_start, a_end);
    int fname_id = assetFileNames.getNameId(fn);

    if (type == FILE_ACTION_REMOVED)
    {
      if (trail_stricmp(fn, ".blk") && fname_id >= 0)
      {
        int relevant_asset_idx = -1;
        for (int i = a_start; i < a_end; i++)
          if (assets[i]->getFileNameId() == fname_id)
          {
            relevant_asset_idx = i;
            break;
          }

        if (relevant_asset_idx >= 0)
        {
          removed_assets.push_back(assets[relevant_asset_idx]);
          erase_items(assets, relevant_asset_idx, 1);
          for (int i = fidx + 1; i < perFolderStartAssetIdx.size(); i++)
            perFolderStartAssetIdx[i]--;

          changed_asset_idx.delInt(relevant_asset_idx);
          for (int i = 0; i < changed_asset_idx.size(); i++)
            if (changed_asset_idx[i] > relevant_asset_idx)
              const_cast<int &>(changed_asset_idx.getList()[i])--;
          sec_changed_asset_idx.delInt(relevant_asset_idx);
          for (int i = 0; i < sec_changed_asset_idx.size(); i++)
            if (sec_changed_asset_idx[i] > relevant_asset_idx)
              const_cast<int &>(sec_changed_asset_idx.getList()[i])--;
          a_end--;

          post_msg(*msgPipe, msgPipe->REMARK, "ASSETS: <%s/%s> file was removed", folders[fidx]->folderPath.str(), fn);
          continue;
        }
      }

      if (!trail_stricmp(fn, ".tmp"))
        post_msg(*msgPipe, msgPipe->WARNING,
          "ASSETS: <%s/%s> file was removed, but changeTracker doesn't handle this case yet."
          " To view actual asset base you need to restart application",
          folders[fidx]->folderPath.str(), fn);
      // continue;
    }
    if (type == FILE_ACTION_RENAMED_OLD_NAME)
    {
      post_msg(*msgPipe, msgPipe->WARNING,
        "ASSETS: <%s/%s> file was renamed, but changeTracker doesn't handle this case yet."
        " To view actual asset base you need to restart application",
        folders[fidx]->folderPath.str(), fn);
      continue;
    }
    if (type == FILE_ACTION_ADDED)
    {
      if (trail_stricmp(fn, ".blk"))
      {
        DagorAssetPrivate *ca = NULL;
        int start_rule_idx = folders[fidx]->vaRuleCount;
        if (addAsset(folders[fidx]->folderPath, fn, -1, ca, folders[fidx], fidx, start_rule_idx, false))
        {
          if (!DagorAssetMgr::findAsset(ca->getName(), ca->getType()))
          {
            added_assets.push_back(ca);
            insert_item_at(assets, a_end, ca);

            for (int i = fidx + 1; i < perFolderStartAssetIdx.size(); i++)
              perFolderStartAssetIdx[i]++;

            for (int i = 0; i < changed_asset_idx.size(); i++)
              if (changed_asset_idx[i] >= a_end)
                const_cast<int &>(changed_asset_idx.getList()[i])++;
            for (int i = 0; i < sec_changed_asset_idx.size(); i++)
              if (sec_changed_asset_idx[i] >= a_end)
                const_cast<int &>(sec_changed_asset_idx.getList()[i])++;
            a_end++;

            post_msg(*msgPipe, msgPipe->REMARK, "ASSETS: <%s/%s> file was added", folders[fidx]->folderPath.str(), fn);
            continue;
          }
        }
        del_it(ca);
      }

      int relevant_asset_idx = -1;
      if (fname_id != -1)
        for (int i = a_start; i < a_end; i++)
          if (assets[i]->getFileNameId() == fname_id)
          {
            relevant_asset_idx = i;
            break;
          }

      if (relevant_asset_idx < 0 && !trail_stricmp(fn, ".tmp"))
        post_msg(*msgPipe, msgPipe->WARNING,
          "ASSETS: <%s/%s> file was added, but changeTracker doesn't handle this case yet."
          " To view actual asset base you need to restart application",
          folders[fidx]->folderPath.str(), fn);
    }

    if (fname_id == -1)
      if (dd_stricmp(fn, ".folder.blk") == 0)
      {
        if (last_fidx != fidx || last_file_id != -2 || time_msec >= last_msec + 10)
          post_msg(*msgPipe, msgPipe->WARNING,
            "ASSETS: <%s/%s> file was changed, but changeTracker doesn't handle this case yet."
            " To view actual asset base you need to restart application",
            folders[fidx]->folderPath.str(), fn);
        fname_id = -2;
        last_fidx = fidx;
        last_msec = time_msec;
        last_file_id = fname_id;
        continue;
      }

    if (last_fidx == fidx && last_file_id == fname_id && time_msec < last_msec + 10)
      continue; // skip outdated/duplicate notifications

    last_fidx = fidx;
    last_file_id = fname_id;
    last_msec = time_msec;

    if (fname_id != -1)
      for (int i = a_start; i < a_end; i++)
        if (assets[i]->getFileNameId() == fname_id)
          changed_asset_idx.addInt(i);
    for (int i = a_start; i < a_end; i++)
    {
      if (changed_asset_idx.hasInt(i))
        continue;
      IDagorAssetExporter *exp = getAssetExporter(assets[i]->getType());
      if (exp)
      {
        exp->gatherSrcDataFiles(*assets[i], files);
        for (int j = 0; j < files.size(); j++)
          if (stricmp(fn, dd_get_fname(files[j])) == 0)
            sec_changed_asset_idx.addInt(i);
      }
    }
  }
  tracker->io.endRead();

  int changed =
    added_assets.size() + removed_assets.size() + changed_asset_idx.getList().size() + sec_changed_asset_idx.getList().size();
  if (changed && updBaseNotify.size())
  {
    static Tab<DagorAsset *> tmpAssetList(tmpmem);
    int added = added_assets.size(), removed = removed_assets.size();
    int changed = changed_asset_idx.getList().size() + sec_changed_asset_idx.getList().size();
    tmpAssetList.clear();

    // added assets
    for (int i = 0; i < added; i++)
      tmpAssetList.push_back(added_assets[i]);

    // removed assets
    for (int i = 0; i < removed; i++)
      tmpAssetList.push_back(removed_assets[i]);

    // add changed assets
    for (int i = 0; i < changed_asset_idx.getList().size(); i++)
      tmpAssetList.push_back(assets[changed_asset_idx.getList()[i]]);

    // add changed assets due to dependencies
    for (int i = 0; i < sec_changed_asset_idx.getList().size(); i++)
      tmpAssetList.push_back(assets[sec_changed_asset_idx.getList()[i]]);

    callAssetBaseChangeNotifications(make_span_const(tmpAssetList).subspan(added + removed, changed),
      make_span_const(tmpAssetList).first(added), make_span_const(tmpAssetList).subspan(added, removed));

    tmpAssetList.clear();
  }

  for (int i = 0; i < removed_assets.size(); i++)
  {
    int atype = removed_assets[i]->getType(), aname = removed_assets[i]->getNameId();
    if (atype < perTypeNotify.size())
    {
      dag::Span<DagorAssetMgr::PerAssetIdNotifyTab::Rec> upd = make_span(perTypeNotify[atype].notify);
      for (int j = upd.size() - 1; j >= 0; j--)
        if (upd[j].assetId == -1 || upd[j].assetId == aname)
          upd[j].client->onAssetRemoved(aname, atype);
    }

    for (int j = updNotify.size() - 1; j >= 0; j--)
      updNotify[j]->onAssetRemoved(aname, atype);
  }

  for (int i = 0; i < changed_asset_idx.getList().size(); i++)
  {
    DagorAsset &a = *assets[changed_asset_idx.getList()[i]];
    int atype = a.getType(), aname = a.getNameId();
    int split_ver = a.props.setInt("splitVer", 0);
    int split_at = a.props.getInt("splitAt", 0);
    bool forceZlib = a.props.getBool("forceZlib", false);

    post_msg(*msgPipe, msgPipe->REMARK, "asset <%s> changed", a.getNameTypified());
    if (!a.isVirtual())
    {
      strcpy(fname, a.getSrcFilePath());
      if (!a.props.load(fname))
        post_error(*msgPipe, "can't load asset <%s> blk: %s", a.getNameTypified(), fname);
    }
    else if (a.getFolderIndex() >= 0)
    {
      DagorAssetFolder *f = folders[a.getFolderIndex()];
      DataBlock va_blk;
      String va_name;
      bool updated = false;
      strcpy(fname, a.getSrcFilePath());

    process_virtual_asset_rules:
      for (int i = 0; i < f->vaRuleCount; i++)
        if (vaRule[f->startVaRuleIdx + i]->testRule(::dd_get_fname(fname), va_name))
        {
          if (stricmp(va_name, a.getName()) == 0)
          {
            if (vaRule[f->startVaRuleIdx + i]->shouldAddSrcBlk())
            {
              va_blk.load(fname);
              vaRule[f->startVaRuleIdx + i]->applyRule(va_blk, fname);

              const char *type_name = vaRule[f->startVaRuleIdx + i]->getTypeName();
              if (!type_name)
                type_name = va_blk.getStr("className", NULL);
              if (type_name && a.getType() == typeNames.getNameId(type_name))
              {
                a.props.setFrom(&va_blk);
                a.props.compact();
                post_msg(*msgPipe, msgPipe->REMARK, "virtual asset <%s> reloaded", a.getNameTypified());
                updated = true;
              }
            }
          }

          if (vaRule[f->startVaRuleIdx + i]->shouldStopProcessing())
            break;
        }

      if (!updated && (f->flags & f->FLG_INHERIT_RULES) && f->parentIdx != -1)
      {
        f = folders[f->parentIdx];
        goto process_virtual_asset_rules;
      }
    }

    if (atype == texAssetType && !strstr(a.getName(), "$hq") && !strstr(a.getName(), "$uhq"))
      if (DagorAsset *hq_a = findAsset(String(0, "%s$hq", a.getName()), texAssetType))
      {
        // re-split and update HQ part
        SimpleString hq_a_pkg(hq_a->props.getStr("forcePackage", NULL));
        SimpleString hq_a_texpack(hq_a->props.getStr("ddsxTexPack", NULL));

        a.props.setInt("splitAt", split_at);

        hq_a->props = a.props;
        hq_a->props.setBool("splitHigh", true);
        hq_a->props.removeBlock("package");
        hq_a->props.removeParam("package");
        hq_a->props.removeParam("rtMipGenBQ");
        hq_a->props.removeParam("splitVer");
        hq_a->props.removeParam("forceZlib");
        if (hq_a_pkg.empty())
          hq_a->props.removeParam("forcePackage");
        else
          hq_a->props.setStr("forcePackage", hq_a_pkg);
        if (hq_a_texpack.empty())
          hq_a->props.removeParam("ddsxTexPack");
        else
          hq_a->props.setStr("ddsxTexPack", hq_a_texpack);
        hq_a->props.compact();

        a.props.setInt("splitVer", split_ver);
        if (forceZlib)
          a.props.setBool("forceZlib", true);
        a.props.compact();
      }

    callAssetChangeNotifications(a, aname, atype);
  }

  for (int i = 0; i < added_assets.size(); i++)
    callAssetChangeNotifications(*added_assets[i], added_assets[i]->getNameId(), added_assets[i]->getType());

  clear_all_ptr_items(removed_assets);

  for (int i = 0; i < sec_changed_asset_idx.getList().size(); i++)
  {
    DagorAsset &a = *assets[sec_changed_asset_idx.getList()[i]];
    if (!a.isVirtual())
    {
      post_msg(*msgPipe, msgPipe->REMARK, "dependant asset <%s> changed", a.getNameTypified());
      strcpy(fname, a.getSrcFilePath());
      if (!a.props.load(fname))
        post_error(*msgPipe, "can't load asset <%s> blk: %s", a.getNameTypified(), fname);
    }
    int atype = a.getType(), aname = a.getNameId();
    callAssetChangeNotifications(a, aname, atype);
  }

  return changed > 0;
}


void DagorAssetMgr::callAssetChangeNotifications(const DagorAsset &a, int aname, int atype) const
{
  if (atype < perTypeNotify.size())
  {
    dag::ConstSpan<DagorAssetMgr::PerAssetIdNotifyTab::Rec> upd = perTypeNotify[atype].notify;
    for (int j = upd.size() - 1; j >= 0; j--)
      if (upd[j].assetId == -1 || upd[j].assetId == aname)
        upd[j].client->onAssetChanged(a, aname, atype);
  }

  for (int j = updNotify.size() - 1; j >= 0; j--)
    updNotify[j]->onAssetChanged(a, aname, atype);
}

void DagorAssetMgr::callAssetBaseChangeNotifications(dag::ConstSpan<DagorAsset *> changed_assets,
  dag::ConstSpan<DagorAsset *> added_assets, dag::ConstSpan<DagorAsset *> removed_assets) const
{
  for (int i = 0; i < updBaseNotify.size(); ++i)
    updBaseNotify[i]->onAssetBaseChanged(changed_assets, added_assets, removed_assets);
}

void DagorAssetMgr::syncTracker() {}

void DagorAssetMgr::subscribeUpdateNotify(IDagorAssetChangeNotify *notify, int asset_name_id, int asset_type)
{
  if (asset_type == -1)
  {
    for (int i = updNotify.size() - 1; i >= 0; i--)
      if (updNotify[i] == notify)
        return;
    updNotify.push_back(notify);
  }
  else
  {
    if (asset_type >= perTypeNotify.size())
      perTypeNotify.resize(asset_type + 1);

    Tab<DagorAssetMgr::PerAssetIdNotifyTab::Rec> &upd = perTypeNotify[asset_type].notify;

    if (asset_name_id == -1)
    {
      for (int i = upd.size() - 1; i >= 0; i--)
        if (upd[i].client == notify)
        {
          if (upd[i].assetId == -1)
            return; // already registered on ANY (-1)
          else
            erase_items(upd, i, 1); // unregister SPECIFIC
        }

      DagorAssetMgr::PerAssetIdNotifyTab::Rec &r = upd.push_back();
      r.client = notify;
      r.assetId = -1;
    }
    else
    {
      for (int i = upd.size() - 1; i >= 0; i--)
        if (upd[i].client == notify)
          if (upd[i].assetId == -1 || upd[i].assetId == asset_name_id)
            return; // already registered on ANY (-1)

      DagorAssetMgr::PerAssetIdNotifyTab::Rec &r = upd.push_back();
      r.client = notify;
      r.assetId = asset_name_id;
    }
  }
}
void DagorAssetMgr::subscribeBaseUpdateNotify(IDagorAssetBaseChangeNotify *notify)
{
  for (int i = updBaseNotify.size() - 1; i >= 0; i--)
    if (updBaseNotify[i] == notify)
      return;
  updBaseNotify.push_back(notify);
}

void DagorAssetMgr::unsubscribeUpdateNotify(IDagorAssetChangeNotify *notify)
{
  for (int i = updNotify.size() - 1; i >= 0; i--)
    if (updNotify[i] == notify)
    {
      erase_items(updNotify, i, 1);
      return;
    }

  for (int asset_type = perTypeNotify.size() - 1; asset_type >= 0; asset_type--)
  {
    Tab<DagorAssetMgr::PerAssetIdNotifyTab::Rec> &upd = perTypeNotify[asset_type].notify;
    for (int i = upd.size() - 1; i >= 0; i--)
      if (upd[i].client == notify)
      {
        int id = upd[i].assetId;
        erase_items(upd, i, 1);
        if (id == -1)
          break;
      }
  }
}
void DagorAssetMgr::unsubscribeBaseUpdateNotify(IDagorAssetBaseChangeNotify *notify)
{
  for (int i = updBaseNotify.size() - 1; i >= 0; i--)
    if (updBaseNotify[i] == notify)
    {
      erase_items(updBaseNotify, i, 1);
      return;
    }
}
#else
#include <assets/assetMgr.h>
void DagorAssetMgr::enableChangesTracker(bool en) {}
bool DagorAssetMgr::rescanBase() { return false; }
bool DagorAssetMgr::trackChangesContinuous(int assets_to_check) { return false; }
void DagorAssetMgr::callAssetChangeNotifications(const DagorAsset &a, int aname, int atype) const {}
void DagorAssetMgr::callAssetBaseChangeNotifications(dag::ConstSpan<DagorAsset *> changed_assets,
  dag::ConstSpan<DagorAsset *> added_assets, dag::ConstSpan<DagorAsset *> removed_assets) const
{}
void DagorAssetMgr::syncTracker() {}
void DagorAssetMgr::subscribeUpdateNotify(IDagorAssetChangeNotify *notify, int asset_name_id, int asset_type) {}
void DagorAssetMgr::subscribeBaseUpdateNotify(IDagorAssetBaseChangeNotify *notify) {}
void DagorAssetMgr::unsubscribeUpdateNotify(IDagorAssetChangeNotify *notify) {}
void DagorAssetMgr::unsubscribeBaseUpdateNotify(IDagorAssetBaseChangeNotify *notify) {}
#endif
