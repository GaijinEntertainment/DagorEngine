//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <datacache/datacache.h>
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>
#include <generic/dag_tab.h>

class WebVromfsDataCache
{
  datacache::Backend *webcache = NULL;
  String webcachePrefix;
  int waitTimeSumUsec[2] = {0, 0};
  int waitReqSumCnt[2] = {0, 0}, waitReqFetchSumCnt = 0;
  int mainThreadSyncWaitTimeoutSec = 20;
  FastNameMap substFileExt;
  Tab<SimpleString> substFilePath;
  Tab<int> substFileSize;
  bool allowCacheVromFallback = true;
  static Tab<SimpleString> bannedUrlPrefix;
  static int banUnresponsiveUrlForTimeoutInSec; //< 9 sec (default)

public:
  bool init(const DataBlock &params, const char *cache_dir = NULL);
  void term();

  bool isInited() const { return webcache != NULL; }

  static bool vromfs_get_file_data(const char *rel_fn, const char *mount_path, String &out_fn, int &out_base_ofs, bool sync_wait_ready,
    bool dont_load, const char *src_fn, void *arg);

protected:
  static void onFileLoaded(const char *key, datacache::ErrorCode err, datacache::Entry *ent, void *arg);
  static void onFileLoadedSync(const char *key, datacache::ErrorCode err, datacache::Entry *ent, void *arg);

  void measureWaitTime(int usec, const char *rel_fn, void *result_entry);
};
