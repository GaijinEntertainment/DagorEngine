// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "filebackend.h"
#include "fileentry.h"
#include "fileutil.h"
#include "../common/util.h"
#include "../common/trace.h"
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_baseDef.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <memory/dag_framemem.h>
#include <stdio.h> // snprintf
#include <time.h>
#include <string.h>
#include <ioSys/dag_dataBlock.h>
#include <errno.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>

namespace datacache
{

const char *FileBackend::tmpFilePref = ".#";

FileBackend::FileBackend() : curSize(0), numOpened(0), populateStatus(NOT_POPULATED), aioJobId(-1), ffJob(NULL) {}

FileBackendConfig::FileBackendConfig(const DataBlock &blk)
{
  mountPath = blk.getStr("mountPath", ".");
  traceLevel = blk.getInt("traceLevel", DEFAULT_TRACE_LEVEL);
  mountPath = blk.getStr("roMountPath", mountPath);
  maxSize = blk.getInt64("maxSize", 0);
  aioJobId = blk.getInt("aioJobId", -1);
}

Backend *FileBackend::create(const FileBackendConfig &config)
{
  if (!dd_mkdir(config.mountPath))
  {
    DOTRACE1("error creating file cache '%s' (can't create directory), errno = %d", config.mountPath, errno);
    return NULL;
  }

  set_trace_level(config.traceLevel);
  const char *roMountPath = config.roMountPath ? config.roMountPath : config.mountPath;
  bool hasRoPath = strcmp(config.mountPath, roMountPath) != 0;
  int mntLen = (int)strlen(config.mountPath), roMntLen = hasRoPath ? (int)strlen(roMountPath) : 0;
  // trim ending slashes
  if (mntLen && (config.mountPath[mntLen - 1] == '/' || config.mountPath[mntLen - 1] == '\\'))
    mntLen--;
  if (hasRoPath && (roMountPath[roMntLen - 1] == '/' || roMountPath[roMntLen - 1] == '\\'))
    roMntLen--;
  FileBackend *bck = (FileBackend *)memalloc(sizeof(FileBackend) + mntLen + roMntLen + 1);
  new (bck, _NEW_INPLACE) FileBackend();
  memcpy(bck->mountPath, config.mountPath, mntLen);
  bck->mountPath[mntLen] = '\0';
  if (hasRoPath)
  {
    bck->roMountPath = &bck->mountPath[mntLen + 1];
    memcpy((void *)bck->roMountPath, roMountPath, roMntLen);
    ((char *)bck->roMountPath)[roMntLen] = '\0';
  }
  else
    bck->roMountPath = bck->mountPath;
  bck->maxSize = config.maxSize;
  bck->manualEviction = config.manualEviction;
  bck->aioJobId = config.aioJobId;
  if (bck->maxSize != 0) // not inifity storage, do populate for eviction
    bck->doPopulate();

  DOTRACE1("file cache mounted to '%s', maxSize = %dK", bck->mountPath, (int)bck->maxSize >> 10);

  return bck;
}

struct FindFilesAsyncJob : public cpujobs::IJob
{
  FileBackend *back;
  carray<Tab<alefind_t>, 2> foundFiles;

  FindFilesAsyncJob(FileBackend *fback) : back(fback) {}
  const char *getJobName(bool &) const override { return "FindFilesAsyncJob"; }
  virtual void doJob()
  {
    char tmpPath[DAGOR_MAX_PATH];
    for (int i = 0, cnt = (back->mountPath == back->roMountPath ? 1 : 2); i < cnt; ++i)
      find_files_recursive(i == 0 ? back->roMountPath : back->mountPath, foundFiles[i], tmpPath);
  }
  virtual void releaseJob()
  {
    if (back)
    {
      back->onFoundFiles(foundFiles);
      G_ASSERT(back->ffJob == this);
      back->ffJob = NULL;
    }
    delete this;
  }
};

FileBackend::~FileBackend()
{
  if (ffJob)
    ffJob->back = NULL;
  for (EntriesMap::iterator it = entries.begin(); it != entries.end(); ++it)
  {
    G_ASSERTF(it->second->refCount == 1, "leaked file entry '%s' (free() wasn't called properly)", it->second->key);
    it->second->detach(this);
  }
}

void FileBackend::onFoundFiles(dag::ConstSpan<Tab<alefind_t>> fnd_results)
{
  DOTRACE2("found %d/%d files", fnd_results[0].size(), fnd_results[1].size());
  G_ASSERT(populateStatus == POPULATION_IN_PROGRESS);

  char tmpPath[DAGOR_MAX_PATH];
  for (int i = 0, cnt = (mountPath == roMountPath ? 1 : 2); i < cnt; ++i)
  {
    const char *mntPath = i == 0 ? roMountPath : mountPath;
    const Tab<alefind_t> &foundFiles = fnd_results[i];
    if (foundFiles.empty())
      continue;

    int mountPathLen = (int)strlen(mntPath);
    for (const alefind_t *fnd = foundFiles.cbegin(); fnd && fnd < foundFiles.cend(); ++fnd)
    {
      G_ASSERT(strlen(fnd->fmask) > mountPathLen + 1);
      strncpy(tmpPath, fnd->fmask, sizeof(tmpPath));
      tmpPath[strlen(tmpPath) - 1] = '\0'; // trim '*'
      strncat(tmpPath, fnd->name, sizeof(tmpPath) - strlen(tmpPath) - 1);
      const char *fname = tmpPath + mountPathLen + 1; // skip mount path & '/'

      const char *tmp = strstr(fname, tmpFilePref);
      if (tmp && (tmp == fname || tmp[-1] == '/')) // ignore temp files
      {
        if (!numOpened)
        {
          dd_erase(tmpPath);
          DOTRACE1("remove temp file '%s'", tmpPath);
        }
        continue;
      }

      int keyLen = 0;
      size_t hashKey = get_entry_hash_key(fname, &keyLen);
      EntriesMap::iterator eit = entries.find(hashKey);
      if (eit != entries.end())                                          // key already exists
        if (!(cnt > 1 && eit->second->readOnly && mntPath == mountPath)) // allow replacing RO to non-RO
          continue;

      createNewEntry(mntPath, fname, keyLen, hashKey, fnd->size, fnd->atime, fnd->mtime);
    }
  }

  populateStatus = POPULATED;
  doEviction();
}

static int lru_cmp(FileEntry *const *ea, FileEntry *const *eb)
{
  if ((*eb)->lastUsed > (*ea)->lastUsed)
    return 1;
  else if ((*eb)->lastUsed < (*ea)->lastUsed)
    return -1;
  else if (int64_t dsz = (*eb)->dataSize - (*ea)->dataSize)
    return dsz > 0 ? -1 : 1;
  else
    return 0;
}

void FileBackend::doEviction()
{
  WinAutoLockOpt lock(csMgr);
  if (maxSize == 0) // infinite
    return;

  if (manualEviction)
    return;

  switch (populateStatus)
  {
    case NOT_POPULATED: doPopulate(); [[fallthrough]];
    case POPULATION_IN_PROGRESS: return; // it's will be called in the future
    case POPULATED: break;
  };
  if (curSize <= maxSize)
    return;

  DOTRACE1("curSize (%d bytes) > maxSize (%d bytes) -> trigger cache eviction", (int)curSize, (int)maxSize);

  Tab<FileEntry *> sentries(framemem_ptr());
  sentries.reserve(entries.size());
  for (EntriesMap::iterator it = entries.begin(); it != entries.end(); ++it)
    sentries.push_back(it->second);
  sort(sentries, &lru_cmp);

  for (int i = 0; i < sentries.size(); ++i)
    DOTRACE3("%d: '%s' - %d  %d", i, sentries[i]->key, (int)sentries[i]->lastUsed, sentries[i]->dataSize);

  int64_t realCurSize = curSize;
  int del_cnt = 0;
  while (sentries.size() && realCurSize > maxSize)
  {
    FileEntry *ent = sentries.back();
    if (!ent->readOnly && ent->refCount == 1)
    {
      realCurSize -= ent->dataSize;
      G_ASSERT(realCurSize >= 0);
      DOTRACE1("evict '%s': size=%d, lastUsed=%d, refCnt=%d", ent->key, ent->dataSize, (int)ent->lastUsed, ent->refCount);
      ent->remove();
      del_cnt++;
    }
    sentries.pop_back();
  }
  DOTRACE1("eviction stopped with realCurSize (%d bytes), %d files removed, curSize=%d bytes", (int)realCurSize, del_cnt,
    (int)curSize);
}

FileBackend::EntriesMap::iterator FileBackend::createNewEntry(const char *mnt, const char *key, int key_len, size_t hash, int size,
  int64_t atime, int64_t mtime)
{
  WinAutoLockOpt lock(csMgr);
  FileEntry *entry = FileEntry::create(this, mnt, key, key_len);
  entry->dataSize = size;
  entry->lastUsed = atime;
  entry->lastModified = mtime;
  entry->readOnly = mountPath != roMountPath && mnt == roMountPath;

  if (!entry->readOnly)
    curSize += entry->dataSize; // add directly to avoid population inside of doEviction()

  eastl::pair<EntriesMap::iterator, bool> ins = entries.insert(EntriesMap::value_type(hash, entry));
  if (ins.second)
    DOTRACE2("created new entry '%s', size=%d, atime=%d, mtime=%d, cache curSize=%d bytes", key, size, (int)atime, (int)mtime,
      (int)curSize);
  else // entries already contains record with this hash
  {
    FileEntry *oldEntry = ins.first->second;
    FileEntry *toFree = NULL;
    if (strcmp(key, oldEntry->key) == 0)
    {
      if (entry->lastModified > oldEntry->lastModified || (oldEntry->readOnly && !entry->readOnly))
      {
        DOTRACE2("replaced entry '%s', size=%d, atime=%d, mtime=%d, cache curSize=%d bytes", key, size, (int)atime, (int)mtime,
          (int)curSize);
        toFree = oldEntry;
        ins.first->second = entry;
      }
      else
      {
        toFree = entry;
        DOTRACE2("ignored attempt to replace entry '%s' because it's older %d < %d", key, (int)entry->lastModified,
          oldEntry->lastModified);
      }
    }
    else // hash collision
    {
      DOTRACE1("key collision '%s' vs '%s' : %p, evict '%s'", key, oldEntry->key, oldEntry->key, (void *)hash);
      toFree = oldEntry;
      ins.first->second = entry;
    }
    toFree->detach(this);
  }
  return ins.first;
}

FileBackend::EntriesMap::iterator FileBackend::getInternal(const char *key)
{
  WinAutoLockOpt lock(csMgr);
  int keyLen = 0;
  size_t hashKey = get_entry_hash_key(key, &keyLen);
  EntriesMap::iterator it = entries.find(hashKey);
  if (it != entries.end()) // hit
  {
    if (strcmp(key, it->second->key) == 0)
      return it;
    else
      DOTRACE1("hash collision '%s' vs '%s' : %p", key, it->second->key, (void *)hashKey);
  }

  // probe fs
  DagorStat st[2]; // rw, ro
  int found = 0;
  for (int i = 0, cnt = (mountPath == roMountPath ? 1 : 2); i < cnt; ++i)
  {
    const char *mntPath = i == 0 ? mountPath : roMountPath;
    char path[DAGOR_MAX_PATH];
    SNPRINTF(path, sizeof(path), "%s/%s", mntPath, key);
    bool fnd = df_stat(path, &st[i]) == 0;
    DOTRACE3("probed file '%s' %sfound", path, fnd ? "" : "not ");
    found |= fnd ? (1 << i) : 0;
  }

  int mntIdx = 0;
  switch (found)
  {
    case 0: // not found
      return entries.end();
    case 1: // rw
    case 2: // ro
      mntIdx = found - 1;
      break;
    case 3:                                       // both
      mntIdx = st[0].mtime > st[1].mtime ? 0 : 1; // select most new one
      break;
    default: G_ASSERT(0);
  }

  return createNewEntry(mntIdx == 0 ? mountPath : roMountPath, key, keyLen, hashKey, st[mntIdx].size, st[mntIdx].atime,
    st[mntIdx].mtime);
}

void FileBackend::doPopulate()
{
  if (ffJob)
    return;

  G_ASSERT(populateStatus != POPULATION_IN_PROGRESS);
  populateStatus = POPULATION_IN_PROGRESS;
  FindFilesAsyncJob *job = new FindFilesAsyncJob(this);
  if (aioJobId >= 0)
  {
    ffJob = job;
    cpujobs::add_job(aioJobId, job);
  }
  else
  {
    job->doJob();
    job->releaseJob();
  }
}

Entry *FileBackend::get(const char *key, ErrorCode *error, completion_cb_t, void *)
{
  WinAutoLockOpt lock(csMgr);
  FileEntry *ret = NULL;
  if (key && *key)
  {
    char tmp[DAGOR_MAX_PATH];
    EntriesMap::iterator it = getInternal(get_real_key(key, tmp));
    DOTRACE3("get '%s' -> %d", key, it == entries.end() ? 0 : 1);
    if (it != entries.end())
      ret = it->second;
  }
  if (error)
    *error = ret ? ERR_OK : ERR_UNKNOWN;
  return ret ? ret->get() : NULL;
}

Entry *FileBackend::set(const char *key, int64_t modtime)
{
  WinAutoLockOpt lock(csMgr);
  G_ASSERT(key && *key);
  int64_t curTime = (int64_t)time(NULL);
  int64_t mtime = modtime >= 0 ? modtime : curTime;

  DOTRACE3("set '%s' %d", key, (int)modtime);

  char tmp[DAGOR_MAX_PATH];
  key = get_real_key(key, tmp);
  EntriesMap::iterator it = getInternal(key);
  FileEntry *entry = it == entries.end() ? NULL : it->second;
  if (!entry || entry->readOnly) // create new
  {
    int keyLen = 0;
    size_t hashKey = get_entry_hash_key(key, &keyLen);
    entry = createNewEntry(mountPath, key, keyLen, hashKey, 0, curTime, mtime)->second;
  }
  else if (modtime >= 0)
    entry->lastModified = modtime;
  entry->flushFileTimes = mtime != curTime;
  return entry->get(false);
}

bool FileBackend::del(const char *key)
{
  WinAutoLockOpt lock(csMgr);
  char tmp[DAGOR_MAX_PATH];
  EntriesMap::iterator it = getInternal(get_real_key(key, tmp));
  DOTRACE3("del '%s' -> %d", key, it == entries.end() ? 0 : 1);
  if (it == entries.end())
    return false;
  FileEntry *ent = it->second;
  if (!ent->remove())
  {
    ent->detach(this);
    entries.erase(it);
  }
  return true;
}

void FileBackend::delAll()
{
  DOTRACE3("delAll", __FUNCTION__);
  WinAutoLockOpt lock(csMgr);
  Tab<FileEntry *> ents(framemem_ptr());
  ents.reserve(entries.size());
  for (EntriesMap::iterator it = entries.begin(); it != entries.end(); ++it)
    ents.push_back(it->second);
  for (int i = 0; i < ents.size(); ++i)
    if (!ents[i]->remove())
      ents[i]->detach(this);
  entries.clear();
}

void FileBackend::onSizeChange(int delta)
{
  curSize += delta;
  DOTRACE3("%s %d -> %d", __FUNCTION__, delta, (int)curSize);
  if (delta <= 0)
    G_ASSERT(curSize >= 0);
  else
    doEviction();
}

Entry *FileBackend::nextEntry(void **iter)
{
  WinAutoLockOpt lock(csMgr);
  if (!*iter)
    *iter = new EntriesMap::iterator(entries.begin());
  EntriesMap::iterator &it = *(EntriesMap::iterator *)*iter;
  while (it != entries.end() && it->second->readOnly) // skip read-only entries (either that or change Entry interface, but does it
                                                      // really needed?)
    ++it;
  DOTRACE3("nextEntry '%s'", it == entries.end() ? "<end>" : it->second->key);
  if (it == entries.end())
    return NULL;
  else
  {
    FileEntry *ret = it->second;
    ++it;
    G_ASSERT(!ret->readOnly);
    return ret->get();
  }
}

void FileBackend::endEnumeration(void **iter)
{
  WinAutoLockOpt lock(csMgr);
  delete (EntriesMap::iterator *)*iter;
  DOTRACE3("endEnumeration");
}

int FileBackend::getEntriesCount() { return (int)entries.size(); }

}; // namespace datacache
