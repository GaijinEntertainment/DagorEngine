#include <libTools/util/genericCache.h>
#include <sys/stat.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <hash/md5.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <sys/types.h>
#include <libTools/util/makeBindump.h>
#include <util/dag_string.h>

#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_C1 | _TARGET_C2 | _TARGET_ANDROID | _TARGET_C3
#define __stat64 stat
#define _stat64  stat
#endif

inline void getFileHashX(const char *fn, unsigned time, unsigned size, unsigned char *out_hash)
{
  G_UNUSED(time);
  G_UNUSED(size);
  GenericBuildCache::getFileHash(fn, out_hash);
}

TwoStepRelPath GenericBuildCache::sdkRoot;

void GenericBuildCache::reset()
{
  fnames.reset();
  rec.clear();
  resetTouchMark();
  memset(&target, 0, sizeof(target));
  timeChanged = 0;
}

bool GenericBuildCache::load(const char *cache_fname, int *end_pos)
{
  reset();
  FullFileLoadCB crd(cache_fname);
  DAGOR_TRY
  {
    if (!crd.fileHandle)
      return false;
    if (crd.readInt() != _MAKE4C('fC2'))
      return false;
    if (crd.readInt() != 2)
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

    resetTouchMark();

    if (crd.readInt() != _MAKE4C('.end'))
      return false;

    if (end_pos)
      *end_pos = crd.tell();
  }
  DAGOR_CATCH(IGenLoad::LoadException exc)
  {
    logerr("failed to read '%s'", cache_fname);
    debug_flush(false);
    return false;
  }
  return true;
}
bool GenericBuildCache::save(const char *cache_fname, mkbindump::BinDumpSaveCB *a_data)
{
  FullFileSaveCB cwr(cache_fname);
  if (!cwr.fileHandle)
    return false;
  cwr.writeInt(_MAKE4C('fC2'));
  cwr.writeInt(2);
  cwr.write(&target, sizeof(target));

  // write file data hashes
  for (int i = 0; i < rec.size(); i++)
    if (rec[i].changed)
      rec[i].changed = 0;

  cwr.writeInt(rec.size());
  iterate_names(fnames, [&](int, const char *name) { cwr.writeString(name); });
  iterate_names(fnames, [&, this](int id, const char *) { cwr.write(rec.data() + id, elem_size(rec)); });

  cwr.writeInt(_MAKE4C('.end'));

  if (a_data)
  {
    cwr.beginBlock();
    a_data->copyDataTo(cwr);
    cwr.endBlock();
  }

  return true;
}

void GenericBuildCache::resetTouchMark()
{
  touchMarkSz = fnames.nameCount();
  touchMark.resize(touchMarkSz);
}
bool GenericBuildCache::checkAllTouched()
{
  for (int i = 0; i < touchMarkSz; i++)
    if (!touchMark.get(i))
      return false;
  return true;
}
void GenericBuildCache::removeUntouched()
{
  NameMapCI n_fn;
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

void GenericBuildCache::updateFileHash(const char *fname)
{
  int id = fnames.getNameId(mkRelPath(fname));
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

bool GenericBuildCache::checkFileChanged(const char *fname)
{
  int id = fnames.addNameId(mkRelPath(fname));
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
  getFileHashX(fname, rec.back().timeStamp, fsz, rec.back().hash);
  return true;
}
bool GenericBuildCache::checkDataBlockChanged(const char *fname_ref, DataBlock &blk)
{
  int id = fnames.addNameId(String(260, "%s*", mkRelPath(fname_ref)));

  if (id < rec.size() && rec[id].changed)
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

    memcpy(rec[id].hash, hash, sizeof(hash));
    rec[id].changed = 1;
    return true;
  }

  append_items(rec, 1);
  G_ASSERT(rec.size() == fnames.nameCount());
  rec.back().timeStamp = 'DB';
  rec.back().changed = 1;
  getDataHash(cwr.data(), cwr.size(), rec.back().hash);
  return true;
}

bool GenericBuildCache::checkTargetFileChanged(const char *fname)
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
void GenericBuildCache::setTargetFileHash(const char *fname)
{
  unsigned fsz;
  target.timeStamp = getFileTime(fname, fsz);
  getFileHashX(fname, target.timeStamp, fsz, target.hash);
}


bool GenericBuildCache::getFileHash(const char *fname, unsigned char out_hash[HASH_SZ])
{
  FullFileLoadCB crd(fname);
  memset(out_hash, 0, HASH_SZ);

  if (!crd.fileHandle)
    return false;

  md5_state_t md5s;
  ::md5_init(&md5s);

  unsigned char buf[32768];
  int len, size = df_length(crd.fileHandle);
  while (size > 0)
  {
    len = size > 32768 ? 32768 : size;
    size -= len;
    crd.read(buf, len);
    ::md5_append(&md5s, buf, len);
  }

  ::md5_finish(&md5s, out_hash);
  return true;
}
void GenericBuildCache::getDataHash(const void *data, int data_len, unsigned char out_hash[HASH_SZ])
{
  md5_state_t md5s;
  memset(out_hash, 0, HASH_SZ);
  ::md5_init(&md5s);
  ::md5_append(&md5s, (const unsigned char *)data, data_len);
  ::md5_finish(&md5s, out_hash);
}

#define STAT64 struct __stat64

unsigned GenericBuildCache::getFileTime(const char *fname, unsigned &out_sz)
{
  STAT64 buf;
  if (_stat64(fname, &buf) != 0)
  {
    out_sz = 0;
    return 0;
  }
  out_sz = buf.st_size;
  return buf.st_mtime;
}
