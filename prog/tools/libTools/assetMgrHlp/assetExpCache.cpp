// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetExpCache.h>
#include <sys/stat.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <hash/md5.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <sys/types.h>
#include <libTools/util/makeBindump.h>
#include <util/dag_string.h>

static SpinLockReadWriteLock sharedDataRWSL;
struct AssetExportCache::SharedData
{
  struct Hash
  {
    unsigned char hash[HASH_SZ];
    unsigned timeStamp, size;
  };

  OAHashNameMap<true> fnames;
  Tab<Hash> rec;
  dag::AtomicInteger<uint64_t> totalHashCalc, totalHashReused;
  dag::AtomicInteger<bool> changed;
  Tab<int> rebuildTypes;
  FastNameMap rebuildAssets;
  void *jobMem;

  SharedData() : rec(midmem), rebuildTypes(midmem)
  {
    reset();
    jobMem = NULL;
  }

  void reset()
  {
    sharedDataRWSL.lockWrite();
    fnames.reset();
    rec.clear();
    changed.store(false);
    totalHashCalc.store(0);
    totalHashReused.store(0);
    sharedDataRWSL.unlockWrite();
  }
  bool load(const char *fname);
  bool save(const char *fname);
  void getFileHash(const char *fn, unsigned time, unsigned sz, unsigned char out_hash[HASH_SZ]);

  void resetAlwaysRebuildTypes() { rebuildTypes.clear(); }
  void addAlwaysRebuildType(int a_type)
  {
    if (a_type < 0 || a_type > 16384)
      return;
    while (a_type >= rebuildTypes.size())
      rebuildTypes.push_back(0);
    rebuildTypes[a_type] = 1;
  }
  void removeAlwaysRebuildType(int a_type)
  {
    if (a_type < 0 || a_type > 16384)
      return;
    if (a_type < rebuildTypes.size())
      rebuildTypes[a_type] = 0;
  }
  bool isAlwaysRebuildType(int a_type)
  {
    if (a_type < 0 || a_type >= rebuildTypes.size())
      return false;
    return rebuildTypes[a_type];
  }
};

// sharedData can be managed not here by setSharedDataPtr. Need to create separate storage which will be free
static eastl::unique_ptr<AssetExportCache::SharedData> sharedDataStorage;
static AssetExportCache::SharedData *sharedData = nullptr;
static SimpleString sharedDataFilename;

inline void getFileHashX(const char *fn, unsigned time, unsigned size, unsigned char *out_hash)
{
  if (sharedData)
    sharedData->getFileHash(fn, time, size, out_hash);
  else
    AssetExportCache::getFileHash(fn, out_hash);
}

TwoStepRelPath AssetExportCache::sdkRoot;

void AssetExportCache::reset()
{
  fnames.reset();
  rec.clear();
  anames.reset();
  adPos.clear();
  assetTypeVer.clear();
  warnings.clear();
  resetTouchMark();
  memset(&target, 0, sizeof(target));
  timeChanged = 0;
}

bool AssetExportCache::load(const char *cache_fname, const DagorAssetMgr &mgr, int *end_pos)
{
  reset();
  FullFileLoadCB crd(cache_fname, DF_READ | DF_IGNORE_MISSING);
  DAGOR_TRY
  {
    if (!crd.fileHandle)
      return false;
    if (crd.readInt() != _MAKE4C('fC1'))
      return false;

    const int cacheFileVersion = crd.readInt();
    if (cacheFileVersion != 2 && cacheFileVersion != 3)
      return false;

    // read target file data hash
    crd.read(&target, sizeof(target));

    // read file data hashes
    rec.resize(crd.readInt());
    String nm;
    for (int i = 0; i < rec.size(); i++)
    {
      crd.readString(nm);
      G_VERIFY(i == fnames.addNameId(nm));
    }
    crd.readTabData(rec);
    for (auto &r : rec)
      r.changed = 0, r._resv0 = 0; // clear 'changed flag and reserved bits after loading

    // read asset exported data references
    adPos.resize(crd.readInt());
    for (int i = 0; i < adPos.size(); i++)
    {
      crd.readString(nm);
      G_VERIFY(i == anames.addNameId(nm));
    }
    crd.readTabData(adPos);
    if (sharedData && sharedData->rebuildAssets.nameCount())
    {
      for (int i = 0; i < adPos.size(); i++)
        if (sharedData->rebuildAssets.getNameId(anames.getName(i)) >= 0)
          adPos[i].ofs = -1;

      if (adPos.empty() && fnames.nameCount() && strstr(cache_fname, ".ddsx.")) // ddsx~cvt cache?
        if (const char *fn = fnames.getName(0))
          if (strstr(fn, ":tex*") && sharedData->rebuildAssets.getNameId(fn, strlen(fn) - 1) >= 0)
            return false;
    }

    // read versions of exporters
    crd.beginBlock();
    String typestr;
    while (crd.getBlockRest())
    {
      int cls = crd.readInt();
      int ver = crd.readInt();
      crd.readString(typestr);
      setAssetExpVer(mgr.getAssetTypeId(typestr), cls, ver);
    }
    crd.endBlock();

    resetTouchMark();

    if (cacheFileVersion >= 3)
    {
      warnings.resize(crd.readInt());
      for (String &warning : warnings)
        crd.readString(warning);
    }

    if (crd.readInt() != _MAKE4C('.end'))
      return false;

    if (end_pos)
      *end_pos = crd.tell();
  }
  DAGOR_CATCH(const IGenLoad::LoadException &exc)
  {
    logerr("failed to read '%s'", cache_fname);
    debug_flush(false);
    return false;
  }
  return true;
}
bool AssetExportCache::save(const char *cache_fname, const DagorAssetMgr &mgr, mkbindump::BinDumpSaveCB *a_data, int *header_size)
{
  FullFileSaveCB cwr(cache_fname);
  if (!cwr.fileHandle)
    return false;
  cwr.writeInt(_MAKE4C('fC1'));
  cwr.writeInt(3);
  cwr.write(&target, sizeof(target));

  // write file data hashes
  for (auto &r : rec)
    r.changed = 0, r._resv0 = 0; // clear 'changed flag and reserved bits before saving

  cwr.writeInt(rec.size());
  iterate_names(fnames, [&](int, const char *name) { cwr.writeString(name); });
  iterate_names(fnames, [&](int id, const char *) { cwr.write(rec.data() + id, elem_size(rec)); });

  // write asset exported data references
  int anum = 0;
  for (int i = 0; i < adPos.size(); i++)
    if (adPos[i].ofs >= 0)
      anum++;

  cwr.writeInt(anum);
  iterate_names(anames, [&](int id, const char *name) {
    if (adPos[id].ofs >= 0)
      cwr.writeString(name);
  });
  iterate_names(anames, [&](int id, const char *) {
    if (adPos[id].ofs >= 0)
      cwr.write(adPos.data() + id, elem_size(adPos));
  });

  // write version of exporters
  cwr.beginBlock();
  for (int i = 0; i < assetTypeVer.size(); i++)
  {
    if (assetTypeVer[i].ver < 0)
      continue;
    const char *typestr = mgr.getAssetTypeName(i);
    if (!typestr)
      continue;
    cwr.writeInt(assetTypeVer[i].classId);
    cwr.writeInt(assetTypeVer[i].ver);
    cwr.writeString(typestr);
  }
  cwr.endBlock();

  cwr.writeInt(warnings.size());
  for (const String &warning : warnings)
    cwr.writeString(warning);

  cwr.writeInt(_MAKE4C('.end'));

  if (header_size)
    *header_size = cwr.tell();

  if (a_data)
  {
    cwr.beginBlock();
    a_data->copyDataTo(cwr);
    cwr.endBlock();
  }

  return true;
}

void AssetExportCache::resetTouchMark()
{
  touchMarkSz = fnames.nameCount();
  touchMark.resize(touchMarkSz);
}
bool AssetExportCache::checkAllTouched()
{
  for (int i = 0; i < touchMarkSz; i++)
    if (!touchMark.get(i))
      return false;
  return true;
}
void AssetExportCache::removeUntouched()
{
  OAHashNameMap<true> n_fn;
  Tab<FileDataHash> n_rec(tmpmem);

  for (int i = 0; i < touchMarkSz; i++)
    if (touchMark.get(i))
    {
      G_VERIFY(n_fn.addNameId(fnames.getName(i)) == n_rec.size());
      n_rec.push_back(rec[i]);
    }

  for (int i = touchMarkSz; i < fnames.nameCount(); i++)
  {
    G_VERIFY(n_fn.addNameId(fnames.getName(i)) == n_rec.size());
    n_rec.push_back(rec[i]);
  }

  fnames = n_fn;
  rec = n_rec;
  resetTouchMark();
  touchMark.setAll();
}

void AssetExportCache::updateFileHash(const char *fname)
{
  TwoStepRelPath::storage_t tmp_stor;
  int id = fnames.getNameId(mkRelPath(fname, tmp_stor));
  if (id >= 0)
  {
    unsigned fsz, ts = getFileTime(fname, fsz);
    if (ts && rec[id].timeStamp == ts)
      return;

    unsigned char hash[HASH_SZ];
    getFileHashX(fname, ts, fsz, hash);
    rec[id].timeStamp = ts;
    if (memcmp(rec[id].hash, hash, HASH_SZ) == 0)
      timeChanged = 1;
    else
    {
      memcpy(rec[id].hash, hash, sizeof(hash));
      rec[id].changed = 1;
    }
  }
}

bool AssetExportCache::checkFileChanged(const char *fname)
{
  TwoStepRelPath::storage_t tmp_stor;
  int id = fnames.addNameId(mkRelPath(fname, tmp_stor));
  if (id < rec.size())
  {
    if (rec[id].changed)
      return true;

    touch(id);
    unsigned fsz, ts = getFileTime(fname, fsz);
    if (ts && rec[id].timeStamp == ts)
      return false;

    unsigned char hash[HASH_SZ];
    getFileHashX(fname, ts, fsz, hash);
    if (memcmp(rec[id].hash, hash, HASH_SZ) == 0)
    {
      rec[id].timeStamp = ts;
      timeChanged = 1;
      return false;
    }

    memcpy(rec[id].hash, hash, sizeof(hash));
    rec[id].timeStamp = ts;
    rec[id].changed = 1;
    return true;
  }

  append_items(rec, 1);
  G_ASSERT(rec.size() == fnames.nameCount());
  unsigned fsz;
  rec.back().timeStamp = getFileTime(fname, fsz);
  rec.back().changed = 1;
  rec.back()._resv0 = 0;
  getFileHashX(fname, rec.back().timeStamp, fsz, rec.back().hash);
  return true;
}
bool AssetExportCache::checkDataBlockChanged(const char *fname_ref, DataBlock &blk, int test_mode)
{
  TwoStepRelPath::storage_t tmp_stor;
  int id = fnames.addNameId(strcat(const_cast<char *>(mkRelPath(fname_ref, tmp_stor)), "*"));

  if (id < rec.size() && rec[id].changed && test_mode == 0)
    return true;

  DynamicMemGeneralSaveCB cwr(tmpmem, 4 << 10, 4 << 10);
  blk.saveToTextStream(cwr);
  if (id < rec.size())
  {
    touch(id);
    unsigned char hash[HASH_SZ];
    getDataHash(cwr.data(), cwr.size(), hash);

    if (memcmp(rec[id].hash, hash, HASH_SZ) == 0)
      return false;
    if (test_mode < 0)
      return true;

    memcpy(rec[id].hash, hash, sizeof(hash));
    rec[id].changed = (test_mode == 0);
    if (test_mode > 0)
      timeChanged = 1;
    return true;
  }

  append_items(rec, 1);
  G_ASSERT(rec.size() == fnames.nameCount());
  rec.back().timeStamp = 'DB';
  if (test_mode > 0)
    timeChanged = 1;
  rec.back().changed = (test_mode == 0);
  rec.back()._resv0 = 0;
  getDataHash(cwr.data(), cwr.size(), rec.back().hash);
  return true;
}
bool AssetExportCache::checkAssetExpVerChanged(int asset_type, unsigned cls, int ver)
{
  if (asset_type < 0 || asset_type >= assetTypeVer.size())
    return true;
  if (sharedData && sharedData->isAlwaysRebuildType(asset_type))
    return true;
  return assetTypeVer[asset_type].ver != ver || assetTypeVer[asset_type].classId != cls;
}

void AssetExportCache::setAssetDataPos(const char *aname, int data_ofs, int data_len)
{
  int id = anames.addNameId(aname);
  if (id < adPos.size())
  {
    adPos[id].ofs = data_ofs;
    adPos[id].len = data_len;
    return;
  }

  append_items(adPos, 1);
  G_ASSERT(adPos.size() == anames.nameCount());
  adPos.back().ofs = data_ofs;
  adPos.back().len = data_len;
}
bool AssetExportCache::getAssetDataPos(const char *aname, int &data_ofs, int &data_len)
{
  int id = anames.getNameId(aname);
  if (id < 0)
  {
    data_ofs = -1;
    data_len = 0;
    return false;
  }
  data_ofs = adPos[id].ofs;
  data_len = adPos[id].len;
  return data_ofs >= 0;
}

bool AssetExportCache::resetExtraAssets(dag::ConstSpan<DagorAsset *> assets)
{
  bitarray_t used;
  used.resize(anames.nameCount());
  for (int i = 0; i < assets.size(); i++)
  {
    int id = anames.getNameId(assets[i]->getNameTypified());
    if (id != -1)
      used.set(id);
  }

  bool changed = false;
  for (int i = 0; i < anames.nameCount(); i++)
    if (!used.get(i))
    {
      changed = true;
      adPos[i].ofs = -1;
      adPos[i].len = 0;
      debug("reset %s", anames.getName(i));
    }
  return changed;
}

void AssetExportCache::setAssetExpVer(int asset_type, unsigned cls, int ver)
{
  if (asset_type < 0)
    return;

  if (asset_type >= assetTypeVer.size())
  {
    int st = assetTypeVer.size();
    assetTypeVer.resize(asset_type + 1);
    mem_set_ff(make_span(assetTypeVer).subspan(st));
  }

  assetTypeVer[asset_type].classId = cls;
  assetTypeVer[asset_type].ver = ver;
}

bool AssetExportCache::checkTargetFileChanged(const char *fname)
{
  if (!target.timeStamp)
    return true;

  unsigned fsz, ts = getFileTime(fname, fsz);
  if (ts && target.timeStamp == ts)
    return false;

  unsigned char hash[HASH_SZ];
  getFileHashX(fname, ts, fsz, hash);
  if (memcmp(target.hash, hash, HASH_SZ) == 0)
  {
    target.timeStamp = ts;
    timeChanged = 1;
    return false;
  }
  target.timeStamp = 0;
  return true;
}
void AssetExportCache::setTargetFileHash(const char *fname)
{
  unsigned fsz;
  target.timeStamp = getFileTime(fname, fsz);
  getFileHashX(fname, target.timeStamp, fsz, target.hash);
}
bool AssetExportCache::getTargetFileHash(unsigned char out_hash[HASH_SZ])
{
  if (!target.timeStamp)
    return false;
  memcpy(out_hash, target.hash, sizeof(target.hash));
  return true;
}


bool AssetExportCache::getFileHash(const char *fname, unsigned char out_hash[HASH_SZ])
{
  FullFileLoadCB crd(fname);
  memset(out_hash, 0, HASH_SZ);

  if (!crd.fileHandle)
    return false;

  DAGOR_TRY
  {
    md5_state_t md5s;
    ::md5_init(&md5s);

    unsigned char buf[32768];
    int len, size = df_length(crd.fileHandle);
    while (size > 0)
    {
      len = size > 32768 ? 32768 : size;
      int rd = crd.tryRead(buf, len);
      for (int retry = 16; len && !rd && retry; retry--)
      {
        logwarn("failed to read any of %d bytes at %d, wait and retry, rest=%d->%d", len, crd.tell(), size,
          df_length(crd.fileHandle) - crd.tell());
        size = df_length(crd.fileHandle) - crd.tell();
        if (!size)
          goto finish;
        sleep_msec(1);
        rd = crd.tryRead(buf, len = size > 32768 ? 32768 : size);
      }
      if (rd != len)
        logwarn("tryRead(%d)=%d, rest=%d", len, rd, size);
      if (!rd)
      {
        logerr("failed to read '%s'", fname);
        return false;
      }
      size -= rd;
      ::md5_append(&md5s, buf, rd);
    }

  finish:
    ::md5_finish(&md5s, out_hash);
  }
  DAGOR_CATCH(const IGenLoad::LoadException &exc)
  {
    logerr("failed to read '%s', exc", fname);
    return false;
  }
  return true;
}
void AssetExportCache::getDataHash(const void *data, int data_len, unsigned char out_hash[HASH_SZ])
{
  md5_state_t md5s;
  memset(out_hash, 0, HASH_SZ);
  ::md5_init(&md5s);
  ::md5_append(&md5s, (const unsigned char *)data, data_len);
  ::md5_finish(&md5s, out_hash);
}

#define STAT64 struct __stat64

unsigned AssetExportCache::getFileTime(const char *fname, unsigned &out_sz)
{
  DagorStat buf;
  if (df_stat(fname, &buf) != 0)
  {
    out_sz = 0;
    return 0;
  }
  out_sz = buf.size;
  return buf.mtime;
}

void AssetExportCache::createSharedData(const char *fname)
{
  if (sharedData)
    return;

  sharedDataFilename = fname;
  sharedDataStorage = eastl::make_unique<AssetExportCache::SharedData>();
  sharedData = sharedDataStorage.get();
  if (!sharedData->load(sharedDataFilename))
    sharedData->reset();
}
void AssetExportCache::reloadSharedData()
{
  if (!sharedData)
    return;
  if (!sharedData->load(sharedDataFilename))
    sharedData->reset();
}
void *AssetExportCache::getSharedDataPtr() { return sharedData; }
void AssetExportCache::setSharedDataPtr(void *p)
{
  if (sharedDataFilename.empty())
    sharedData = (SharedData *)p;
}
bool AssetExportCache::saveSharedData()
{
  if (!sharedDataFilename.empty())
    return sharedData->save(sharedDataFilename);
  return false;
}
void AssetExportCache::sharedDataResetRebuildTypesList()
{
  if (sharedData)
    sharedData->resetAlwaysRebuildTypes();
}
void AssetExportCache::sharedDataAddRebuildType(int a_type)
{
  if (a_type < 0)
    return;
  if (sharedData)
    sharedData->addAlwaysRebuildType(a_type);
}
void AssetExportCache::sharedDataRemoveRebuildType(int a_type)
{
  if (a_type < 0)
    return;
  if (sharedData)
    sharedData->removeAlwaysRebuildType(a_type);
}
void AssetExportCache::sharedDataResetForceRebuildAssetsList()
{
  if (sharedData)
    sharedData->rebuildAssets.reset(true);
}
void AssetExportCache::sharedDataAddForceRebuildAsset(const char *asset_name_typified)
{
  if (sharedData)
    sharedData->rebuildAssets.addNameId(asset_name_typified);
}
bool AssetExportCache::sharedDataIsAssetInForceRebuildList(const DagorAsset &a)
{
  if (sharedData && sharedData->rebuildAssets.nameCount())
    return sharedData->rebuildAssets.getNameId(a.getNameTypified()) >= 0;
  return false;
}
void AssetExportCache::setJobSharedMem(void *p)
{
  if (sharedData)
    sharedData->jobMem = p;
}
void *AssetExportCache::getJobSharedMem() { return sharedData ? sharedData->jobMem : NULL; }
bool AssetExportCache::sharedDataGetFileHash(const char *fname, unsigned char out_hash[HASH_SZ])
{
  unsigned fsz, ts = getFileTime(fname, fsz);
  if (!ts)
    return false;
  getFileHashX(fname, ts, fsz, out_hash);
  return true;
}
void AssetExportCache::sharedDataAppendHash(const void *data, size_t data_len, unsigned char inout_hash[HASH_SZ])
{
  md5_state_t md5s;
  ::md5_init(&md5s);
  ::md5_append(&md5s, inout_hash, HASH_SZ);
  ::md5_append(&md5s, (const md5_byte_t *)data, data_len);
  ::md5_finish(&md5s, inout_hash);
}

bool AssetExportCache::SharedData::load(const char *cache_fname)
{
  reset();
  FullFileLoadCB crd(cache_fname);
  if (!crd.fileHandle)
    return false;
  if (crd.readInt() != _MAKE4C('sdC'))
    return false;
  if (crd.readInt() != 1)
    return false;

  sharedDataRWSL.lockWrite();
  // read file data hashes
  rec.resize(crd.readInt());
  String nm;
  for (int i = 0; i < rec.size(); i++)
  {
    crd.readString(nm);
    G_VERIFY(i == fnames.addNameId(nm));
  }
  crd.readTabData(rec);
  sharedDataRWSL.unlockWrite();

  if (crd.readInt() != _MAKE4C('.end'))
    return false;

  changed.store(false);
  return true;
}
bool AssetExportCache::SharedData::save(const char *cache_fname)
{
  debug("hash data computed: %dM;  reused: %dM", unsigned(totalHashCalc.load() >> 20), unsigned(totalHashReused.load() >> 20));
  if (!changed.load())
    return true;

  FullFileSaveCB cwr(cache_fname);
  if (!cwr.fileHandle)
    return false;
  sharedDataRWSL.lockRead();
  cwr.writeInt(_MAKE4C('sdC'));
  cwr.writeInt(1);

  // write file data hashes
  cwr.writeInt(rec.size());
  iterate_names(fnames, [&](int, const char *name) { cwr.writeString(name); });
  iterate_names(fnames, [&](int id, const char *) { cwr.write(rec.data() + id, elem_size(rec)); });
  cwr.writeInt(_MAKE4C('.end'));
  sharedDataRWSL.unlockRead();
  changed.store(false);
  return true;
}
void AssetExportCache::SharedData::getFileHash(const char *fn, unsigned time, unsigned sz, unsigned char out_hash[HASH_SZ])
{
  TwoStepRelPath::storage_t tmp_stor;
  const char *rel_fn = AssetExportCache::mkRelPath(fn, tmp_stor);
  sharedDataRWSL.lockRead();
  int id = fnames.getNameId(rel_fn);
  if (id >= 0 && time == rec[id].timeStamp && sz == rec[id].size)
  {
    // debug("reused old hash");
    memcpy(out_hash, rec[id].hash, HASH_SZ);
    sharedDataRWSL.unlockRead();
    totalHashReused.fetch_add(sz);
    return;
  }
  sharedDataRWSL.unlockRead();

  AssetExportCache::getFileHash(fn, out_hash);
  // debug("calc hash: %s", fn);
  sharedDataRWSL.lockWrite();
  id = fnames.addNameId(rel_fn);
  if (id >= rec.size())
  {
    G_ASSERT(id == rec.size());
    rec.resize(id + 1);
  }
  memcpy(rec[id].hash, out_hash, HASH_SZ);
  rec[id].timeStamp = time;
  rec[id].size = sz;
  sharedDataRWSL.unlockWrite();

  changed.store(true);
  totalHashCalc.fetch_add(sz);
}
