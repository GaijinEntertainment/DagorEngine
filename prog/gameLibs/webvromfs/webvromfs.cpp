// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webvromfs/webvromfs.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_urlLike.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_base32.h>
#include <hash/sha1.h>
#include <debug/dag_debug.h>

#define ENT_PTR_STATUS_PENDING   ((datacache::Entry *)((char *)NULL + 0))
#define ENT_PTR_STATUS_FAILED    ((datacache::Entry *)((char *)NULL + 1))
#define ENT_PTR_STATUS_ABORTED   ((datacache::Entry *)((char *)NULL + 2))
#define IS_ENT_PTR_STATUS_BAD(E) ((E) == ENT_PTR_STATUS_FAILED || (E) == ENT_PTR_STATUS_ABORTED)

Tab<SimpleString> WebVromfsDataCache::bannedUrlPrefix;
int WebVromfsDataCache::banUnresponsiveUrlForTimeoutInSec = 9;

bool WebVromfsDataCache::init(const DataBlock &params, const char *cache_dir)
{
  DataBlock wcParams;
  wcParams = params;

  webcachePrefix = cache_dir ? cache_dir : params.getStr("cacheFolder", "");
  if (webcachePrefix.empty())
  {
    logerr("web-vromfs: [disk] cache_dir=%s and params.getStr(\"cacheFolder\")=%s", cache_dir, params.getStr("cacheFolder", NULL));
    return false;
  }
  simplify_fname(webcachePrefix);
  wcParams.setStr("mountPath", webcachePrefix);
  webcachePrefix.append("/");
  simplify_fname(webcachePrefix);

  banUnresponsiveUrlForTimeoutInSec = params.getInt("banUnresponsiveUrlForTimeoutInSec", 10);
  wcParams.setBool("noIndex", true);
  wcParams.setInt64("maxSize", params.getInt64("maxSize", 32 << 20));
  wcParams.setInt("timeoutSec", params.getInt("timeoutSec", 12));
  wcParams.setInt("traceLevel", params.getInt("traceLevel", 0));
  wcParams.setBool("smartMultiThreading", true);
  wcParams.setStr("jobMgrName", "vromfsWebCache");
  mainThreadSyncWaitTimeoutSec = params.getInt("mainThreadTimeoutSec", mainThreadSyncWaitTimeoutSec);

  const DataBlock *substFileTypes = wcParams.getBlockByNameEx("substFileTypes");
  for (int i = 0; i < substFileTypes->paramCount(); i++)
  {
    const char *fn = substFileTypes->getStr(i);
    file_ptr_t fp = df_open(fn[0] == '*' ? fn + 1 : fn, DF_READ | DF_IGNORE_MISSING);
    if (!fp)
    {
      logerr("web-vromfs: [disk] subst file %s not found, mentioned in substFileTypes{}", substFileTypes->getStr(i));
      continue;
    }
    int fsz = df_length(fp);
    df_close(fp);

    if (substFileExt.addNameId(substFileTypes->getParamName(i)) != substFilePath.size())
    {
      logerr("web-vromfs: [disk] duplicate ext=%s in substFileTypes{}", substFileTypes->getParamName(i));
      continue;
    }
    substFilePath.push_back() = fn;
    substFileSize.push_back() = fsz;
  }
  allowCacheVromFallback = wcParams.getBool("allowCacheVromFallback", true);

  const DataBlock *baseUrls = wcParams.getBlockByNameEx("baseUrls");
  debug("webfs::init");
  for (int i = 0; i < baseUrls->paramCount(); i++)
    debug("  URL %s", baseUrls->getStr(i));
  for (int i = 0, nid = baseUrls->getNameId("url"); i < baseUrls->blockCount(); i++)
    if (baseUrls->getBlock(i)->getBlockNameId() == nid)
      debug("  URL %s (weight %g)", baseUrls->getBlock(i)->getStr("url", "?"), baseUrls->getBlock(i)->getReal("weight", 1));
  if (!baseUrls->paramCount() && baseUrls->findBlock("url") < 0)
    debug("  no URLs specified");

  for (int i = 0; i < substFilePath.size(); i++)
    debug("  subst[%s]=%s  %d bytes", substFileExt.getName(i), substFilePath[i], substFileSize[i]);
  if (!substFilePath.size())
    debug("  no typed file substitutes");

  webcache = datacache::create(datacache::BT_WEB, wcParams);
  if (!webcache)
  {
    logerr("web-vromfs: failed to init webfs");
    debug_print_datablock("WebVromfsDataCache", &wcParams);
    return false;
  }
  return true;
}

void WebVromfsDataCache::term()
{
  datacache::Backend *wc = webcache;
  webcache = NULL;
  del_it(wc);
  clear_and_shrink(webcachePrefix);
  clear_and_shrink(bannedUrlPrefix);

  if (waitReqSumCnt[0])
    debug("web-vromfs: spent %d msec for %d sync download and %d fetch requests (main thread)", waitTimeSumUsec[0] / 1000,
      waitReqSumCnt[0], waitReqFetchSumCnt);
  if (waitReqSumCnt[1])
    debug("web-vromfs: spent %d msec in threads for %d async download requests", waitTimeSumUsec[1] / 1000, waitReqSumCnt[1]);
  waitTimeSumUsec[0] = waitTimeSumUsec[1] = waitReqSumCnt[0] = waitReqSumCnt[1] = waitReqFetchSumCnt = 0;
}

bool WebVromfsDataCache::vromfs_get_file_data(const char *rel_fn, const char * /*mount_path*/, String &out_fn, int &out_base_ofs,
  bool sync_wait_ready, bool dont_load, const char *src_fn, void *arg)
{
  int64_t reft = ref_time_ticks();
  WebVromfsDataCache *wb = reinterpret_cast<WebVromfsDataCache *>(arg);
  if (!wb->webcache)
    return false;

  G_ASSERTF(sync_wait_ready || is_main_thread(), "mainThread=%d sync_wait_ready=%d", sync_wait_ready, is_main_thread());

  datacache::ErrorCode err = datacache::ERR_UNKNOWN;
  volatile datacache::Entry *sync_wait_entry = ENT_PTR_STATUS_PENDING;
  datacache::EntryHolder ent;
  if (!dont_load)
    for (const auto &p : bannedUrlPrefix)
      if (strncmp(rel_fn, p, p.length()) == 0)
      {
        logwarn("skip loading %s due to bannedUrlPrefix[]=%s", rel_fn, p);
        dont_load = true;
        break;
      }
  if (dont_load)
  {
    ent.reset(wb->webcache->get(rel_fn, &err));
    if (err == datacache::ERR_OK)
      goto return_data;
    return false;
  }

  ent.reset(wb->webcache->get(rel_fn, &err, sync_wait_ready ? &onFileLoadedSync : &onFileLoaded, (void *)&sync_wait_entry));

  if (err == datacache::ERR_OK)
  {
    if (!dd_file_exist(ent->getPath()))
    {
      err = datacache::ERR_PENDING;
      if (sync_wait_ready)
        for (int wait_thres = is_main_thread() ? 1000000 : 10000000; get_time_usec(reft) < wait_thres;)
        {
          if (is_main_thread())
            cpujobs::release_done_jobs();
          sleep_msec(1);
          if (dd_file_exist(ent->getPath()))
          {
            err = datacache::ERR_OK;
            break;
          }
        }
    }
    if (err == datacache::ERR_PENDING)
      logwarn("web-vromfs: cache has reported that %s (for %s) is ready, but file is still missing after waiting %d msec",
        ent->getPath(), rel_fn, get_time_usec(reft) / 1000);

  return_data:
    out_fn = ent->getPath();
    out_base_ofs = 0;
    wb->measureWaitTime(get_time_usec(reft), rel_fn, (void *)sync_wait_entry);
    return err == datacache::ERR_OK;
  }

  if (err == datacache::ERR_PENDING)
  {
    if (!sync_wait_ready)
    {
      wb->waitReqFetchSumCnt++;
      return false;
    }
    if (!is_main_thread())
      logwarn("web-vromfs: waiting for pending get(%s) from non-main thread", rel_fn);

    int max_timeout_usec = wb->mainThreadSyncWaitTimeoutSec * 1000000;
    wb->webcache->poll();
    while (!sync_wait_entry) //-V1044
    {
      sleep_msec(1);
      if (is_main_thread())
        cpujobs::release_done_jobs();
      if (get_time_usec(reft) > max_timeout_usec)
      {
        logwarn("web-vromfs: [network] get(%s), waited %.01f sec, still waiting...", rel_fn, get_time_usec(reft) / 1e6f);
        max_timeout_usec += wb->mainThreadSyncWaitTimeoutSec * 1000000;
        //== we cannot stop waiting here since onFileLoadedSync() will later write to sync_wait_entry (on the stack)!
        // sync_wait_entry = ENT_PTR_STATUS_ABORTED;
      }

      wb->webcache->poll();
    }

    if (sync_wait_entry && !IS_ENT_PTR_STATUS_BAD(sync_wait_entry))
    {
      ent.reset(const_cast<datacache::Entry *>(sync_wait_entry));
      err = datacache::ERR_OK;
      debug("web-vromfs: downloaded %s", ent->getPath());
      goto return_data;
    }
    err = datacache::ERR_UNKNOWN;
    if (banUnresponsiveUrlForTimeoutInSec && get_time_usec(reft) / 1000000 >= banUnresponsiveUrlForTimeoutInSec)
      if (df_is_fname_url_like(rel_fn) && !memchr(rel_fn, '\0', 9))
        if (const char *prefix_end = strchr(rel_fn + 8, '/'))
        {
          SimpleString s(rel_fn, prefix_end + 1 - rel_fn);
          if (find_value_idx(bannedUrlPrefix, s) < 0)
          {
            bannedUrlPrefix.emplace_back(s);
            logerr("load (%s) failed for %.1f sec -> add '%s' to bannedUrlPrefix (%d total)", rel_fn, get_time_usec(reft) / 1e6f, s,
              bannedUrlPrefix.size());
          }
        }
  }

  wb->measureWaitTime(get_time_usec(reft), rel_fn, (void *)sync_wait_entry);
  logerr("web-vromfs: [network] get(%s): err=%d", rel_fn, err);
  if (const char *ext = dd_get_fname_ext(src_fn))
  {
    int idx = wb->substFileExt.getNameId(ext);
    if (idx >= 0)
    {
      out_fn = wb->substFilePath[idx];
      out_base_ofs = wb->substFileSize[idx];
    }
    else if (wb->allowCacheVromFallback)
      out_fn = "*";
  }
  else if (wb->allowCacheVromFallback)
    out_fn = "*";
  return false;
}

void WebVromfsDataCache::measureWaitTime(int usec, const char *rel_fn, void *sync_wait_entry)
{
  int &wait_time = waitTimeSumUsec[is_main_thread() ? 0 : 1];
  int &wait_cnt = waitReqSumCnt[is_main_thread() ? 0 : 1];
  int zero = 0, &wait_fetch_cnt = is_main_thread() ? waitReqFetchSumCnt : zero;

  wait_time += usec;
  wait_cnt++;

  if (usec > (is_main_thread() ? 40 : 1000) * 1000 || IS_ENT_PTR_STATUS_BAD(sync_wait_entry))
    logwarn("web-vromfs: spent %d msec to download %s, status=%p", usec / 1000, rel_fn, (void *)sync_wait_entry);

  if (wait_time > 5000000)
  {
    debug("web-vromfs: spent another %d msec for %d sync download and %d fetch requests%s", wait_time / 1000, wait_cnt, wait_fetch_cnt,
      is_main_thread() ? " (main thread)" : "");
    wait_time = 0;
    wait_cnt = 0;
    wait_fetch_cnt = 0;
  }
}

static void remove_entry(datacache::Entry *ent)
{
  ent->closeStream();
  ent->del();
  ent->free();
}
static bool check_file_hash_and_erase_if_broken(const char *key, datacache::Entry *ent)
{
  IGenLoad *crd = ent->getReadStream();
  if (crd && df_is_fname_url_like(key))
    return true;
  if (strlen(key) < 35 || key[2] != '/' || key[33] != '-' || !crd)
  {
    logerr("web-vromfs: [disk] file(%s) bad name format%s, removing (key=%s)", ent->getPath(), crd ? "" : " or crd=NULL", key);
    remove_entry(ent);
    return false;
  }

  int64_t req_file_sz = str_b32_to_uint64(key + 34);
  if (req_file_sz != ent->getDataSize())
  {
    logerr("web-vromfs: [disk] file(%s) size %d != %d (%s), removing", ent->getPath(), ent->getDataSize(), req_file_sz, key + 34);
    remove_entry(ent);
    return false;
  }

  static constexpr int BUF_SZ = 8 << 10;
  char buf[BUF_SZ];
  sha1_context sha1c;

  sha1_starts(&sha1c);
  while (int len = crd->tryRead(buf, BUF_SZ))
    sha1_update(&sha1c, (unsigned char *)buf, len);
  unsigned char digest[20];
  sha1_finish(&sha1c, digest);
  ent->closeStream();

  char out_hash[32 + 1];
  data_to_str_b32_buf(out_hash, sizeof(out_hash), digest, sizeof(digest));
  if (dd_strnicmp(out_hash, key, 2) == 0 && dd_strnicmp(out_hash + 2, key + 3, sizeof(out_hash) - 3) == 0)
    return true;
  logerr("web-vromfs: [disk] file(%s) SHA1 %s != %.2s%.30s, removing", ent->getPath(), out_hash, key, key + 3);
  remove_entry(ent);
  return false;
}

void WebVromfsDataCache::onFileLoaded(const char *key, datacache::ErrorCode err, datacache::Entry *ent, void * /*arg*/)
{
  datacache::EntryHolder holder(ent);
  if (err == datacache::ERR_OK && ent)
    check_file_hash_and_erase_if_broken(key, ent);
}
void WebVromfsDataCache::onFileLoadedSync(const char *key, datacache::ErrorCode err, datacache::Entry *ent, void *arg)
{
  if (err == datacache::ERR_OK && ent && check_file_hash_and_erase_if_broken(key, ent))
    *(datacache::Entry **)arg = ent;
  else
  {
    *(datacache::Entry **)arg = ENT_PTR_STATUS_FAILED;
    logwarn("web-vromfs: [network] failed to download %s, err=%d", key, err);
  }
}
