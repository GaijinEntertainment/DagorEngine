// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <EASTL/string.h>

namespace drv3d_vulkan
{
class PipelineCacheFile
{
public:
  struct UUID
  {
    uint32_t dword;   //-V730_NOINIT
    uint16_t word[3]; //-V730_NOINIT
    uint8_t byte[6];  //-V730_NOINIT

    UUID() {}
    UUID(uint32_t a, uint16_t b0, uint16_t b1, uint16_t b2, uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4, uint8_t c5)
    {
      dword = a;
      word[0] = b0;
      word[1] = b1;
      word[2] = b2;
      byte[0] = c0;
      byte[1] = c1;
      byte[2] = c2;
      byte[3] = c3;
      byte[4] = c4;
      byte[5] = c5;
    }
    UUID(const uint8_t *bytes) { memcpy(this, bytes, 16); }
    const uint8_t *data() const { return reinterpret_cast<const uint8_t *>(this); }

    inline friend bool operator==(const UUID &l, const UUID &r) { return 0 == memcmp(&l, &r, sizeof(UUID)); }
    inline friend bool operator!=(const UUID &l, const UUID &r) { return 0 != memcmp(&l, &r, sizeof(UUID)); }
  };

private:
  struct FileHeader
  {
    UUID magic;
    uint8_t hash[20];
    uint32_t appVer;
  };

  struct EntryHeader
  {
    UUID id;
    uint8_t hash[20];
  };

  /**
   * File layout:
   * <File Header>
   * <Entry Header>[entryCount]
   * <data>
   *
   * FileHeader.hash -> sha1 hash of the Entry Header block
   * EntryHeader.hash -> sha1 hash of the entry data block
   */
  struct LocalCacheEntry
  {
    UUID id;
    Tab<uint8_t> data;
  };

  Tab<LocalCacheEntry> localCache;

protected:
  void load(const char *path);
  void store();
  void onLoadError(const char *path, const char *reason);

  const char *getDataPath();

public:
  // tries to load the data from ram cache
  bool getCacheFor(const UUID &id, Tab<uint8_t> &target);
  // replaces the ram cache data
  void storeCacheFor(const UUID &id, dag::ConstSpan<uint8_t> data);
};

struct PipelineCacheOutFile : public PipelineCacheFile
{
  ~PipelineCacheOutFile() { store(); }
};

struct PipelineCacheInFile : public PipelineCacheFile
{
  PipelineCacheInFile(const char *path = nullptr) { load(path); }
};
} // namespace drv3d_vulkan