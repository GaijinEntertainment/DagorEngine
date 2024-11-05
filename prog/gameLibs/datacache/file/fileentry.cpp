// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fileentry.h"
#include "filebackend.h"
#include <time.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h> // GetLastError()
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif
#if _TARGET_C1 | _TARGET_C2

#endif
#include <errno.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <stdio.h> // snprintf
#include "../common/trace.h"
#include "../common/util.h"

namespace datacache
{

class FileCacheSave : public LFileGeneralSaveCB
{
public:
  FileEntry *entry;
  FileCacheSave(file_ptr_t handle, FileEntry *e) : LFileGeneralSaveCB(handle), entry(e) {}
  void write(const void *ptr, int size)
  {
    LFileGeneralSaveCB::write(ptr, size);
    entry->dataWritten += size;
  }
  int tryWrite(const void *ptr, int size)
  {
    int ret = LFileGeneralSaveCB::tryWrite(ptr, size);
    entry->dataWritten += ret;
    return ret;
  }
  // you can delete this if you really need to, but cacheSize won't be calculated correctly (i.e. cache eviction won't be working
  // correctly)
  void seekto(int) { G_ASSERTF(0, "seeks are not supported"); }
  void seektoend(int) { G_ASSERTF(0, "seeks are not supported"); }
};

FileEntry::FileEntry(FileBackend *back) :
  refCount(1),
  opType(OT_NONE),
  mmapPtr(NULL),
  file_ptr(NULL),
  delOnFree(false),
  readOnly(false),
  flushFileTimes(false),
  lastUsed(0),
  lastModified(0),
  dataSize(0),
  dataWritten(0),
  sizeDelta(0),
  backend(back),
  key(NULL)
{}

FileEntry::~FileEntry()
{
  if (backend)
  {
    char tmp[DAGOR_MAX_PATH];
    G_VERIFY(backend->entries.erase(get_entry_hash_key(get_real_key(key, tmp))));
  }
}

/* static */
FileEntry *FileEntry::create(FileBackend *bck, const char *mnt, const char *key, int key_len)
{
  int mountPathLen = (int)strlen(mnt);
  FileEntry *entry = (FileEntry *)memalloc(sizeof(FileEntry) + mountPathLen + 1 + key_len);
  new (entry, _NEW_INPLACE) FileEntry(bck);
  memcpy(entry->filePath, mnt, mountPathLen);
  entry->filePath[mountPathLen] = '/';
  entry->key = &entry->filePath[mountPathLen + 1];
  memcpy((void *)entry->key, key, key_len + 1);
  return entry;
}

void FileEntry::del() { delOnFree = !readOnly; }

// Don't use vroms (hence DF_REALFILE_ONLY) to skip read lock within which is not free and might even cause deadlocks
static constexpr unsigned DF_READ_FLAGS = DF_READ | DF_IGNORE_MISSING | DF_REALFILE_ONLY;

#define CHECKC(cond)                                                   \
  if (!(cond))                                                         \
  {                                                                    \
    DOTRACE2(#cond " is false, file '%s', errno=%d", filePath, errno); \
    goto fail_cond;                                                    \
  }
dag::ConstSpan<uint8_t> FileEntry::getData()
{
  CHECKC(!isOpened());
  file_ptr = df_open(filePath, DF_READ_FLAGS);
  CHECKC(file_ptr);
  mmapPtr = df_mmap(file_ptr, &dataSize);
  CHECKC(mmapPtr);
  opType = OT_MMAP;
  // Note: some platforms update atime automatically (AFAIK all besides Windows),
  // but even then it can be disabled by user, so be on the safe side and update it manually
  DOTRACE3("mmap'ed '%s', len=%d", filePath, dataSize);
  return dag::ConstSpan<uint8_t>((const uint8_t *)mmapPtr, dataSize);
fail_cond:
  if (file_ptr)
    df_close(file_ptr);
  file_ptr = NULL;
  if (mmapPtr)
    df_unmap(mmapPtr, dataSize);
  mmapPtr = NULL;
  return dag::ConstSpan<uint8_t>((const uint8_t *)NULL, 0);
}

IGenLoad *FileEntry::getReadStream()
{
  CHECKC(!isOpened());
  file_ptr = df_open(filePath, DF_READ_FLAGS);
  CHECKC(file_ptr);
  dataSize = df_length(file_ptr);
  opType = OT_READ_STREAM;
  DOTRACE3("opened '%s' for read, len=%d", filePath, dataSize);
  readStream = new LFileGeneralLoadCB(file_ptr);
  return readStream;
fail_cond:
  if (file_ptr)
    df_close(file_ptr);
  file_ptr = NULL;
  return NULL;
}
#undef CHECKC

IGenSave *FileEntry::getWriteStream()
{
  G_ASSERTF(!readOnly, "Attempt to write in read-only directory to file '%s'! Use set() for that", filePath);
  G_ASSERTF(!isOpened(), "Attempt to write in already opened file '%s'", filePath);
  if (readOnly || isOpened())
    return NULL;

  dd_mkpath(filePath);

  char tmpFile[DAGOR_MAX_PATH];
  const char *delim = strrchr(filePath, '/');
  SNPRINTF(tmpFile, sizeof(tmpFile), "%.*s/%sXXXXXX", delim ? (int)(delim - filePath) : DAGOR_MAX_PATH, filePath,
    FileBackend::tmpFilePref);
  file_ptr = df_mkstemp(tmpFile);
  if (file_ptr)
  {
    opType = OT_WRITE_STREAM;
    tempFileName = tmpFile;
    writeStream = new FileCacheSave(file_ptr, this);
    dataWritten = 0;
    DOTRACE3("opened '%s' for write", filePath);
    return writeStream;
  }
  else
  {
    DOTRACE1("failed to open '%s' for write, errno=%d", filePath, errno);
    return NULL;
  }
}

void FileEntry::setLastModified(int64_t last_modified)
{
  if (last_modified < 0)
    return;
  flushFileTimes = lastModified != last_modified;
  lastModified = last_modified;
}

FileEntry *FileEntry::get(bool file_time)
{
  addRef();
  backend->numOpened++;
  if (file_time)
  {
    int64_t curTime = (int64_t)time(NULL);
    if (lastUsed != curTime)
    {
      lastUsed = curTime;
      flushFileTimes = true;
    }
  }
  return this;
}

void FileEntry::free()
{
  WinAutoLockOpt lock(backend ? backend->csMgr : NULL);
  bool delFromBackend = delOnFree && backend && refCount == 2; // marked for deletion last non detached entry? delete from backed as
                                                               // well
  if (backend)
  {
    --backend->numOpened;
    G_ASSERT(backend->numOpened >= 0);
  }
  G_ASSERT(refCount > 0);
  if (refCount <= 2) // first ref is hold by backend (except for detached ones)
    freeInternal();
  else
    delRef();
  if (delFromBackend)
    delRef();
}

void FileEntry::closeStream()
{
  if (isOpened())
  {
    switch (opType)
    {
      case OT_NONE: break; // to shut up compiler warning
      case OT_MMAP: df_unmap(mmapPtr, dataSize); break;
      case OT_READ_STREAM: delete readStream; break;
      case OT_WRITE_STREAM: delete writeStream; break;
    };
    if (file_ptr)
      df_close(file_ptr);
    file_ptr = NULL;
    mmapPtr = NULL;
    if (opType == OT_WRITE_STREAM)
    {
      if (delOnFree)
      {
        DOTRACE3("file '%s' is marked for deletion, remove tmp '%s'", filePath, tempFileName.str());
        dd_erase(tempFileName.str());
        delOnFree = false;
      }
      else if (dd_rename(tempFileName, filePath))
      {
        DOTRACE3("renamed '%s' -> '%s'", tempFileName.str(), filePath);
        sizeDelta = dataWritten - dataSize;
        dataSize = dataWritten;
      }
      else
      {
        int lastError = -1;
#if _TARGET_PC_WIN | _TARGET_XBOX
        lastError = GetLastError();
#endif
        DOTRACE1("rename '%s' -> '%s' failed (lastErr=%d)!", tempFileName.str(), filePath, lastError);
      }
      tempFileName.clear();
    }
    opType = OT_NONE;
    DOTRACE3("closed '%s'", key);
  }
}

void FileEntry::freeInternal()
{
  DOTRACE3("free '%s' refCnt=%d", key, refCount);

  closeStream();
  if (readOnly)
    ; // do nothing
  else if (delOnFree)
  {
    sizeDelta = -dataSize;
    dataSize = 0;
    delOnFree = false;
    bool ret = dd_erase(filePath);
    DOTRACE3("deleted file '%s' -> %d", filePath, (int)ret);
    G_UNUSED(ret);
  }
  else if (flushFileTimes)
  {
#if _TARGET_PC_WIN | _TARGET_XBOX
    utimbuf times = {lastUsed, lastModified};
    utime(filePath, &times);
#elif _TARGET_C3

#else
    timeval times[2] = {{(time_t)lastUsed, 0}, {(time_t)lastModified, 0}};
#if _TARGET_C1 | _TARGET_C2

#else
    utimes(filePath, times);
#endif
#endif
    DOTRACE3("flush file times for '%s' atime=%d, mtime=%d", filePath, (int)lastUsed, (int)lastModified);
  }

  WinAutoLockOpt lock(backend ? backend->csMgr : NULL);
  if (sizeDelta && backend)
    backend->onSizeChange(sizeDelta);
  sizeDelta = 0;

  delRef();
}

}; // namespace datacache
