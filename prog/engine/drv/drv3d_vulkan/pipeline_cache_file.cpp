// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_cache_file.h"
#include <hash/sha1.h>

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_string.h>
#include "driver.h"
#include "driver_config.h"
#include "globals.h"
#include <osApiWrappers/dag_direct.h>

#if _TARGET_C3



#else
#define USE_COMPRESSED_CACHE 1
#endif

using namespace drv3d_vulkan;
// time based uuid
static const PipelineCacheFile::UUID file_magic =
  PipelineCacheFile::UUID(0x8F8ED0A2, 0xC419, 0x11EE, 0xBD08, 0x32, 0x50, 0x96, 0xB3, 0x9F, 0x47);

uint32_t getPipelineSignificantAppVersion()
{
  uint32_t appVer = get_app_ver();
  // we usually change many shaders only when changing most significant ver numbers
  // mask out last significant one
  appVer &= ~0xFFUl;
  return appVer;
}

void PipelineCacheFile::load(const char *path)
{
  const DataBlock *cfgBlk = Globals::cfg.getPerDriverPropertyBlock("pipelineCompiler");
  const bool allowPipelineCache = ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBool("allowPipelineCache", true) &&
                                  cfgBlk->getBool("allowPipelineCache", true);

  if (!allowPipelineCache)
    return;

  const char *filePath = path ? path : getDataPath();
  if (!filePath || !*filePath)
    return;

  FullFileLoadCB srcFile(filePath, DF_READ | DF_IGNORE_MISSING);
  if (!srcFile.fileHandle)
  {
    debug("vulkan: missing pipeline cache %s", filePath);
    return;
  }
  if (df_length(srcFile.fileHandle) <= 4)
  {
    onLoadError(path, String(0, "corrupted file, size=%d", df_length(srcFile.fileHandle)));
    return;
  }

  int len = srcFile.beginBlock();

  // ignore file if it is totally broken
#if USE_COMPRESSED_CACHE
  if (len <= 4)
#else
  if (len <= sizeof(FileHeader))
#endif
  {
    onLoadError(path, String(0, "corrupted file, packed size=%d, file size=%d", len, df_length(srcFile.fileHandle)));
    return;
  }

#if USE_COMPRESSED_CACHE
  ZlibLoadCB src(srcFile, len, false, false);
#else
  FullFileLoadCB &src = srcFile;
#endif

  FileHeader header;
  if (src.tryRead(&header, sizeof(header)) != sizeof(header))
  {
    onLoadError(path, "can't read header");
#if USE_COMPRESSED_CACHE
    src.ceaseReading();
#endif
    return;
  }

  if (header.magic != file_magic)
  {
    onLoadError(path, "magic mismatch");
#if USE_COMPRESSED_CACHE
    src.ceaseReading();
#endif
    return;
  }

  // load cache on dev builds always
  // as well as load dev supplied cache files
  uint32_t appVer = getPipelineSignificantAppVersion();
  if (header.appVer != appVer && appVer && header.appVer)
  {
    onLoadError(path, "appVer mismatch");
#if USE_COMPRESSED_CACHE
    src.ceaseReading();
#endif
    return;
  }

  int numEntries = 0;
  Tab<EntryHeader> headers;
  if (!src.tryRead(&numEntries, sizeof(int)))
  {
    onLoadError(path, "failed to read numEntries");
#if USE_COMPRESSED_CACHE
    src.ceaseReading();
#endif
    return;
  }
  headers.resize_noinit(numEntries);
  if (numEntries > 0 && !src.tryRead(headers.data(), headers.size() * sizeof(EntryHeader)))
  {
    headers.clear();
    onLoadError(path, "failed to read headers");
#if USE_COMPRESSED_CACHE
    src.ceaseReading();
#endif
    return;
  }
  uint8_t hashValue[20];
  sha1_csum(reinterpret_cast<const uint8_t *>(headers.data()), data_size(headers), hashValue);
  if (0 != memcmp(header.hash, hashValue, sizeof(header.hash)))
  {
    onLoadError(path, "headers integrity check failed");
#if USE_COMPRESSED_CACHE
    src.ceaseReading();
#endif
    return;
  }

  localCache.reserve(headers.size());
  for (uint32_t i = 0; i < headers.size(); ++i)
  {
    LocalCacheEntry &lc = localCache.push_back();
    lc.id = headers[i].id;
    int contentSize = 0;
    if (!src.tryRead(&contentSize, sizeof(contentSize)))
    {
      onLoadError(path, "failed to read the size of local cache data");
#if USE_COMPRESSED_CACHE
      src.ceaseReading();
#endif
      return;
    }
    lc.data.resize_noinit(contentSize);
    if (contentSize > 0 && !src.tryRead(lc.data.data(), contentSize * sizeof(lc.data[0])))
    {
      lc.data.clear();
      onLoadError(path, "failed to read local cache data");
#if USE_COMPRESSED_CACHE
      src.ceaseReading();
#endif
      return;
    }
    sha1_csum(lc.data.data(), data_size(lc.data), hashValue);
    if (0 != memcmp(headers[i].hash, hashValue, sizeof(headers[i].hash)))
    {
      onLoadError(path, "data integrity check failed");
#if USE_COMPRESSED_CACHE
      src.ceaseReading();
#endif
      clear_and_shrink(localCache);
      return;
    }
  }

  src.ceaseReading();
}

void PipelineCacheFile::onLoadError(const char *path, const char *reason)
{
  debug("vulkan: pipeline cache %s load error: %s", path ? path : getDataPath(), reason);

  // remove errored out default pipeline cache file
  if (!path)
    remove(getDataPath());
}

class CacheFileSaveCB : public IBaseSave
{
  file_ptr_t fileHandle = NULL;
  bool valid = true;
  eastl::string targetName;

public:
  CacheFileSaveCB(const char *fname)
  {
    dd_mkpath(fname);
    fileHandle = df_open(fname, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING);
    targetName = fname;

    if (!fileHandle)
      valid = false;
  }

  ~CacheFileSaveCB() { close(); }

  void close()
  {
    if (fileHandle)
    {
      df_close(fileHandle);
      fileHandle = NULL;
    }
  }

  void write(const void *ptr, int size) override
  {
    if (!fileHandle || !valid)
      return;
    if (df_write(fileHandle, (void *)ptr, size) != size)
      valid = false;
  }

  int tell() override
  {
    if (!fileHandle)
      return 0;
    return df_tell(fileHandle);
  }

  void seekto(int pos) override
  {
    if (fileHandle)
      df_seek_to(fileHandle, pos);
  }

  void seektoend(int ofs = 0) override
  {
    if (fileHandle)
      df_seek_end(fileHandle, ofs);
  }

  const char *getTargetName() override { return targetName.c_str(); }

  void flush() override
  {
    if (fileHandle)
      df_flush(fileHandle);
  }

  bool check(const char *stage)
  {
    if (!valid)
      logwarn("vulkan: pipe cache store: failed at %s", stage);
    return valid;
  }
};

void PipelineCacheFile::store()
{
  const char *path = getDataPath();
  if (!path || !*path)
    return;

  debug("vulkan: saving pipeline cache to %s", path);

  Tab<EntryHeader> entries;
  entries.reserve(localCache.size());
  for (uint32_t i = 0; i < localCache.size(); ++i)
  {
    EntryHeader &ent = entries.push_back();
    ent.id = localCache[i].id;
    sha1_csum(localCache[i].data.data(), data_size(localCache[i].data), ent.hash);
  }

  FileHeader fileHeader;
  fileHeader.magic = file_magic;
  fileHeader.appVer = getPipelineSignificantAppVersion();
  sha1_csum(reinterpret_cast<const uint8_t *>(entries.data()), data_size(entries), fileHeader.hash);


#define CHECK(x)         \
  if (!dstFile.check(x)) \
  return

#if USE_COMPRESSED_CACHE
#define CHECK_COMPR(x)   \
  if (!dstFile.check(x)) \
  {                      \
    dst.finish();        \
    return;              \
  }
#else
#define CHECK_COMPR(x) CHECK(x)
#endif

  CacheFileSaveCB dstFile(path);
  CHECK("open");

  dstFile.beginBlock();
  CHECK("start block");
#if USE_COMPRESSED_CACHE
  ZlibSaveCB dst(dstFile, ZlibSaveCB::CL_BestCompression);
#else
  CacheFileSaveCB &dst = dstFile;
#endif

  dst.write(&fileHeader, sizeof(fileHeader));
  CHECK_COMPR("header");
  dst.writeTab(entries);
  CHECK_COMPR("entries");
  for (uint32_t i = 0; i < localCache.size(); ++i)
    dst.writeTab(localCache[i].data);
  CHECK_COMPR("cache");

#if USE_COMPRESSED_CACHE
  dst.finish();
  CHECK("compression");
#endif
  dstFile.endBlock();
  CHECK("end");

#undef CHECK_COMPR
#undef CHECK

  debug("vulkan: pipeline cache saved");
}

const char *drv3d_vulkan::PipelineCacheFile::getDataPath()
{
#if _TARGET_C3

#else
  return ::dgs_get_settings()->getBlockByNameEx("vulkan")->getStr("cachefile", "cache/vulkan.bin");
#endif
}

bool PipelineCacheFile::getCacheFor(const UUID &id, Tab<uint8_t> &target)
{
  for (uint32_t i = 0; i < localCache.size(); ++i)
  {
    if (localCache[i].id == id)
    {
      target = localCache[i].data;
      return true;
    }
  }
  return false;
}

void PipelineCacheFile::storeCacheFor(const UUID &id, dag::ConstSpan<uint8_t> data)
{
  for (uint32_t i = 0; i < localCache.size(); ++i)
  {
    if (localCache[i].id == id)
    {
      localCache[i].data = data;
      return;
    }
  }

  LocalCacheEntry &entry = localCache.push_back();
  entry.id = id;
  entry.data = data;
}