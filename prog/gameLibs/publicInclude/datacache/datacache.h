//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <util/dag_stdint.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <streamIO/streamIO.h>

class IGenLoad;
class IGenSave;
class DataBlock;

namespace datacache
{

enum BackendType
{
  BT_FILE, // simple synchronous file-based read-write cache
  BT_WEB   // read-only asynchronous web-based cache
};

struct FileBackendConfig
{
  FileBackendConfig() = default;
  FileBackendConfig(const DataBlock &blk);

  const char *mountPath = ".";
  const char *roMountPath = nullptr;
  int64_t maxSize = 0;
  bool manualEviction = false;
  int traceLevel = 1;
  int aioJobId = -1;
};

struct WebBackendConfig : FileBackendConfig
{
  WebBackendConfig() = default;
  WebBackendConfig(const DataBlock &blk);

  const char *skipKey = nullptr;
  const char *jobMgrName = nullptr;
  int timeoutSec = 4;
  int connectTimeoutSec = -1;
  int lowSpeedTimeSec = 0;
  int lowSpeedLimitBps = 0;
  int maxSyncCheckSizeKb = 1024;
  bool noIndex = false;
  bool smartMultiThreading = false;
  bool allowReturnStaleData = true;

  struct BaseUrl
  {
    const char *url;
    float weight;
  };
  eastl::vector<BaseUrl> baseUrls;
};

enum ErrorCode
{
  ERR_UNKNOWN = -1,
  ERR_OK = 0,
  ERR_PENDING = 40, // this particular number is selected to avoid intersection with libcurl error codes
  ERR_IO = 44,      // same
  ERR_ABORTED = 48,
  ERR_MEMORY_LIMIT = 49,
};

class Entry;
typedef void (*completion_cb_t)(const char *key, ErrorCode error, Entry *entry, void *arg);
typedef void (*resp_headers_cb_t)(const char *key, streamio::StringMap const &headers, void *arg);
class Backend
{
public:
  virtual ~Backend() {}

  // If ERR_PENDING is returned in 'error' param then callback will be called upon completion
  virtual Entry *getWithRespHeaders(const char *, ErrorCode * = NULL, completion_cb_t = NULL, resp_headers_cb_t = NULL, void * = NULL)
  {
    return nullptr;
  };
  virtual Entry *get(const char *key, ErrorCode *error = NULL, completion_cb_t cb = NULL, void *cb_arg = NULL) = 0;
  virtual Entry *set(const char *key, int64_t modtime = -1) = 0;
  virtual bool del(const char *key) = 0;
  virtual void delAll() = 0;
  virtual int getEntriesCount() = 0;
  virtual void control(int opcode, void *param0 = NULL, void *param1 = NULL) = 0;
  virtual Entry *nextEntry(void **iter) = 0;
  virtual void endEnumeration(void **iter) = 0;
  virtual void poll() = 0;
  virtual void abortActiveRequests() {}
  virtual bool hasFreeSpace() const { return true; }
};

class Entry
{
public:
  virtual ~Entry() {}

  // metadata
  virtual const char *getKey() const = 0; // Note: key is not necessary the same that was passed to Backend::set()
  virtual const char *getPath() const = 0;
  virtual int getDataSize() const = 0;
  virtual int64_t getLastUsed() const = 0;
  virtual int64_t getLastModified() const = 0;
  virtual void setLastModified(int64_t) = 0;

  // data
  virtual dag::ConstSpan<uint8_t> getData() = 0; // not recommended for big chunks
  virtual IGenLoad *getReadStream() = 0;         // streams destroyed automatically
  virtual IGenSave *getWriteStream() = 0;
  virtual void closeStream() = 0;

  // misc
  virtual void del() = 0;  // delete both entry & corresponding file
  virtual void free() = 0; // this must be called after work with entry is completed (or just use EntryHolder)
};

struct EntryDeleter
{
  void operator()(Entry *e) const { e->free(); }
};

using EntryHolder = eastl::unique_ptr<Entry, EntryDeleter>;

Backend *create(BackendType bt, const DataBlock &blk);

Backend *create(FileBackendConfig const &config);
Backend *create(WebBackendConfig &config);

}; // namespace datacache
