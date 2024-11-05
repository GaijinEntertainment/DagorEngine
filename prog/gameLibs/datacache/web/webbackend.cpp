// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "webbackend.h"
#include "../file/filebackend.h"
#include <util/dag_globDef.h>
#include <streamIO/streamIO.h>
#include <math/random/dag_random.h>
#include <EASTL/algorithm.h>
#include <ioSys/dag_dataBlock.h>
#include <stdio.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/unique_ptr.h>
#include <memory/dag_framemem.h>
#include "../common/util.h"
#include "../common/trace.h"
#include "webutil.h"
#include "gzip.h"
#include <osApiWrappers/dag_threads.h>

#define INDEX_FILE_NAME "00index.gz"

namespace datacache
{

#define URL_BUF_SIZE  512
#define INIT_UB_MAGIC 0xCAFEBABE

static const char RAW_HTTP_CACHE[] = "httpCache"; // non indexed cache

static bool is_url(const char *name) // starts with "http:" or "https:"
{
  static const char http[] = "http";
  return strncmp(name, http, sizeof(http) - 1) == 0 && (name[4] == ':' || (name[4] == 's' && name[5] == ':'));
}

WebBackend::WebBackend() :
  UBMagic(INIT_UB_MAGIC),
  indexState(INDEX_NOT_LOADED),
  streamCtx(NULL),
  jobMgr(-1),
  returnStaleData(true),
  noIndex(false),
  gcScheduled(false),
  shutdowning(false),
  inFlightJobs(NULL)
{}

WebBackend::~WebBackend()
{
  shutdowning = true;
  delete streamCtx; // should be removed before job mgr because callbacks can still hold job mgr ref (see onHttpReqCompleted())
  streamCtx = (streamio::Context *)(void *)uintptr_t(-1); // for debug
  if (jobMgr >= 0)
  {
    if (inFlightJobs)
      cpujobs::reset_job_queue(jobMgr, true);
    while (inFlightJobs)
    {
      sleep_msec(0);
      cpujobs::release_done_jobs();
    }
    cpujobs::destroy_virtual_job_manager(jobMgr, true);
  }
  delete filecache;
  del_it(csMgr);
  del_it(csJob);
  UBMagic = 0xDEADBEEF;
}

static WeightedUrlsType load_weighted_urls(const WebBackendConfig &config)
{
  WeightedUrlsType weightedUrls;
  auto pushUrl = [&weightedUrls](const char *urlc, float weight) {
    if (!urlc || !*urlc)
      return;
    eastl::string url = urlc;
    if (url.back() == '/')
      url.pop_back();
    auto fpred = [&url](const WeightedUrlsType::value_type &pair) { return pair.first == url; };
    auto it = eastl::find_if(weightedUrls.begin(), weightedUrls.end(), fpred);
    if (it == weightedUrls.end())
      weightedUrls.push_back(eastl::make_pair(eastl::move(url), weight));
    else
      it->second += weight;
  };

  for (const WebBackendConfig::BaseUrl &burl : config.baseUrls)
    pushUrl(burl.url, burl.weight);

  return weightedUrls;
}

template <typename T>
inline const T &maybe_deref(const T &x)
{
  return x;
}
template <typename T>
inline const T &maybe_deref(T *const &x)
{
  return *x;
}
template <typename T>
static int weighted_random_choice(const T &cont)
{
  if (cont.empty())
    return -1;
  float totalWeight = 0.f;
  for (auto &wu : cont)
    totalWeight += maybe_deref(wu).second;
  float rndChoice = gfrnd() * totalWeight, upto = 0.f;
  for (int i = 0; i < cont.size(); ++i)
  {
    auto &wu = cont[i];
    float weight = maybe_deref(wu).second;
    if (upto + weight >= rndChoice)
      return i;
    upto += weight;
  }
  G_ASSERT(0); // shouldn't be here
  return -1;
}

bool WebBackend::chooseNextAvailableBaseUrl()
{
  int ui = weighted_random_choice(availBaseUrls);
  bool ret = ui >= 0;
  if (!ret) // loop available urls from start
  {
    availBaseUrls.clear();
    availBaseUrls.reserve(allBaseUrls.size());
    for (auto &wu : allBaseUrls)
      availBaseUrls.push_back(&wu);
    ui = weighted_random_choice(availBaseUrls);
  }
  if (ui >= 0)
  {
    auto &url = availBaseUrls[ui]->first;
    baseUrl = eastl::string_view(url.c_str(), url.length());
    availBaseUrls.erase(availBaseUrls.begin() + ui); // not available anymore
  }
  else
    baseUrl = eastl::string_view();
  DOTRACE1("new base url '%s' is chosen, %d candidates left", baseUrl.data(), (int)availBaseUrls.size());
  return ret;
}

WebBackendConfig::WebBackendConfig(const DataBlock &blk) : FileBackendConfig(blk)
{
  noIndex = blk.getBool("noIndex", noIndex);
  allowReturnStaleData = blk.getBool("allowReturnStaleData", allowReturnStaleData);
  timeoutSec = blk.getInt("timeoutSec", timeoutSec);
  connectTimeoutSec = blk.getInt("connectTimeoutSec", connectTimeoutSec);
  lowSpeedTimeSec = blk.getInt("lowSpeedTimeSec", lowSpeedTimeSec);
  lowSpeedLimitBps = blk.getInt("lowSpeedLimitBps", lowSpeedLimitBps);
  maxSyncCheckSizeKb = blk.getInt("maxSyncCheckSizeKb", maxSyncCheckSizeKb);
  jobMgrName = blk.getStr("jobMgrName", nullptr);
  skipKey = blk.getStr("skipKey", "");

  const DataBlock &baseUrlsBlk = *blk.getBlockByNameEx("baseUrls");
  for (int i = 0; i < baseUrlsBlk.blockCount(); ++i)
  {
    const DataBlock &urlBlk = *baseUrlsBlk.getBlock(i);
    baseUrls.push_back({urlBlk.getStr("url", NULL), urlBlk.getReal("weight", 1.0f)});
  }

  for (int i = 0; i < baseUrlsBlk.paramCount(); ++i)
    if (baseUrlsBlk.getParamType(i) == DataBlock::TYPE_STRING)
      baseUrls.push_back({baseUrlsBlk.getStr(i), 1.0f});
}

Backend *WebBackend::create(WebBackendConfig &config)
{
  bool noIndex = config.noIndex;
  WeightedUrlsType weightedUrls = load_weighted_urls(config);
  if (weightedUrls.empty() && !noIndex)
  {
    DOTRACE1("can't mount webcache - no base urls are specified!");
    return NULL;
  }

  WebBackend *bck = new WebBackend();

  if (config.smartMultiThreading)
  {
    bck->csMgr = new WinCritSec("webbackend-mgr-cs");
    bck->csJob = new WinCritSec("webbackend-job-cs");
  }
  bck->allBaseUrls = eastl::move(weightedUrls);
  bck->chooseNextAvailableBaseUrl();
  bck->skipKey = config.skipKey;
  config.aioJobId = bck->getOrCreateJobMgr(config.jobMgrName);

  bck->filecache = FileBackend::create(config);
  if (bck->filecache)
    DOTRACE1("webcache mounted");
  else
  {
    delete bck;
    return NULL;
  }

  bck->returnStaleData = config.allowReturnStaleData;
  bck->noIndex = noIndex;
  bck->timeoutSec = config.timeoutSec;
  bck->connectTimeoutSec = config.connectTimeoutSec;
  bck->lowSpeedTimeSec = config.lowSpeedTimeSec;
  bck->lowSpeedLimitBps = config.lowSpeedLimitBps;
  bck->maxSyncCheckSize = config.maxSyncCheckSizeKb << 10;
  if (bck->csMgr)
    G_ASSERTF(bck->noIndex && bck->returnStaleData, "noIndex=%d returnStaleData=%d", bck->noIndex, bck->returnStaleData);
  bck->filecache->control(_MAKE4C('CS'), bck->csMgr);

  return bck;
}

const char *WebBackend::getUrl(const char *url, char *buf, int buf_size)
{
  G_ASSERT(!baseUrl.empty());
  SNPRINTF(buf, buf_size, "%s/%s", baseUrl.data(), url);
  return buf;
}

void WebBackend::collectGarbage()
{
  G_ASSERT(!noIndex); // can't be (the only place where gc scheduled is index download which should not be executed in noIndex mode)

  DOTRACE2("collect garbage");
  void *iter = NULL;
  for (Entry *entry = filecache->nextEntry(&iter); entry; entry = filecache->nextEntry(&iter))
  {
    const char *ekey = entry->getKey();
    if (strcmp(ekey, INDEX_FILE_NAME) == 0 || strncmp(ekey, RAW_HTTP_CACHE, sizeof(RAW_HTTP_CACHE) - 1) == 0) // special exceptions
    {
      entry->free();
      continue;
    }
    EntriesMap::iterator it = entries.find(get_entry_hash_key(ekey));
    if (it == entries.end())
    {
      DOTRACE1("%s remove unknown file '%s'", __FUNCTION__, ekey);
      entry->del();
    }
    entry->free();
  }
  filecache->endEnumeration(&iter);
}

struct AsyncJob : public cpujobs::IJob
{
  AsyncJob *nextInFlightJob; // single linked list
  WebBackend *backend;
  Entry *entry;
  SimpleString key;
  Tab<datacache::WebBackend::UserCb> userCb;
  bool syncJob = false;
  int numRequests = 1;

  AsyncJob(WebBackend *back, Entry *ent, const char *key_) : nextInFlightJob(NULL), backend(back), entry(ent), key(key_) {}
  ~AsyncJob()
  {
    WinAutoLockOpt lock(backend->csMgr);
    G_ASSERT(is_main_thread() || syncJob); // this condition is must be satisfied in order to be thread safe
    if (entry)
      entry->free();
    if (!syncJob)
      removeThisFromInFlightJobs();
  }
  void removeThisFromInFlightJobs()
  {
    for (AsyncJob **pjob = &backend->inFlightJobs; *pjob; pjob = &(*pjob)->nextInFlightJob)
      if (*pjob == this)
      {
        *pjob = nextInFlightJob;

        if (!backend->inFlightJobs && backend->gcScheduled && !backend->shutdowning)
        {
          backend->gcScheduled = false;
          backend->collectGarbage();
        }

        return;
      }
  }
  void callCb(const char *real_key, int error)
  {
    if (real_key)
    {
      for (int i = 0; i < userCb.size(); ++i)
      {
        ErrorCode err = error == streamio::ERR_ABORTED ? ERR_ABORTED : ERR_UNKNOWN;
        Entry *ent = (error == streamio::ERR_OK || error == streamio::ERR_NOT_MODIFIED) // do get() for each callback
                       ? backend->filecache->get(real_key, &err)
                       : NULL;
        userCb[i].cb(key, err, ent, userCb[i].arg);
      }
    }
    else
      for (int i = 0; i < userCb.size(); ++i)
        userCb[i].cb(key, ERR_UNKNOWN, NULL, userCb[i].arg);
  }

  void callRespHeadersCb(streamio::StringMap const &resp_headers)
  {
    for (int i = 0; i < userCb.size(); ++i)
      if (userCb[i].resp_headers_cb)
        userCb[i].resp_headers_cb(key, resp_headers, userCb[i].arg);
  }
};

struct DownloadRequest : public AsyncJob
{
  int error;
  IGenLoad *stream;
  int64_t lastModified;

  DownloadRequest(WebBackend *back, const char *key_) :
    AsyncJob(back, NULL, key_), stream(NULL), error(streamio::ERR_ABORTED), lastModified(-1)
  {}
  ~DownloadRequest() { delete stream; }

  virtual streamio::ProcessResult onStreamData(dag::ConstSpan<char>) { return streamio::ProcessResult::Discarded; }
};

struct IndexDownloadRequest : public DownloadRequest
{
  EntriesMap newEntries;
  datacache::WebBackend::UserCb onIndexDownloaded;

  IndexDownloadRequest(WebBackend *back, const char *key_) : DownloadRequest(back, key_)
  {
    onIndexDownloaded = datacache::WebBackend::UserCb{};
    entry = back->filecache->set(INDEX_FILE_NAME);
  }

  virtual void doJob()
  {
    WinAutoLockOpt lock(backend->csJob);
    IGenLoad *zstream = NULL;
    if (error == streamio::ERR_NOT_MODIFIED || !stream) // read already existing file
    {
      DOTRACE2("%s -> load local index", error == streamio::ERR_NOT_MODIFIED ? "index not modified" : "error loading index file");
      zstream = gzip_stream_create(entry->getReadStream());
    }
    else // read newly downloaded stream
    {
      DOTRACE2("load index from downloaded file");
      if (copy_stream(stream, entry->getWriteStream()))
      {
        stream->seekto(0);
        zstream = gzip_stream_create(stream);
      }
      else
        error = ERR_IO;
    }
    parse_index(newEntries, zstream);
    delete zstream;
  }

  virtual void releaseJob()
  {
    WinAutoLockOpt lock(backend->csMgr);
    G_ASSERT(entry);         // we created it in ctor, therefore it can't be NULL
    if (!newEntries.empty()) // was something loaded? (assume that index can't be empty)
      backend->entries.swap(newEntries);
    if (error == streamio::ERR_OK) // new index was downloaded
    {
      entry->setLastModified(lastModified);
      backend->gcScheduled = true;
    }
    else if (error != streamio::ERR_NOT_MODIFIED)
      entry->del(); // error occured, don't write empty file
    backend->setIndexState(WebBackend::INDEX_LOADED);

    removeThisFromInFlightJobs(); // before get() to avoid infinite adding callbacks to this request
    for (int i = 0; i < userCb.size(); ++i)
    {
      ErrorCode err = ERR_UNKNOWN;
      Entry *fentry = backend->get(key, &err, userCb[i].cb, userCb[i].arg);
      if (err != ERR_PENDING)
        userCb[i].cb(key, err, fentry, userCb[i].arg);
    }

    if (onIndexDownloaded.cb)
      onIndexDownloaded.cb("", (ErrorCode)error, NULL, onIndexDownloaded.arg);

    delete this;
  }
};

struct FileDownloadRequest : public DownloadRequest
{
  uint8_t downloadedHash[SHA_DIGEST_LENGTH];

  FileDownloadRequest(WebBackend *back, const char *key_) : DownloadRequest(back, key_)
  {
    memset(downloadedHash, 0, sizeof(downloadedHash));
    entry = back->filecache->set(key_);
  }

  virtual void doJob()
  {
    WinAutoLockOpt lock(backend->csJob);
    if (stream)
      if (!hashstream(stream, downloadedHash, entry->getWriteStream()))
        error = ERR_IO;
  }

  virtual void releaseJob()
  {
    WinAutoLockOpt lock(backend->csMgr);
    G_ASSERT(entry); // we created it in ctor, therefore it can't be NULL
    char buf1[SHA_DIGEST_LENGTH * 2 + 1], keyBuf[DAGOR_MAX_PATH];
    const char *realKey = get_real_key(key.str(), keyBuf);
    EntriesMap::iterator it = backend->entries.find(get_entry_hash_key(realKey));
    if (it == backend->entries.end())
    {
      DOTRACE1("downloaded unknown file '%s': not found in index", realKey);
      callCb(NULL, error);
    }
    else if (error == streamio::ERR_OK && memcmp(downloadedHash, it->second.hash, sizeof(downloadedHash)) != 0)
    {
      char buf2[SHA_DIGEST_LENGTH * 2 + 1];
      DOTRACE1("downloaded broken file '%s': file hash (%s) != index hash (%s)", realKey, hashstr(downloadedHash, buf1),
        hashstr(it->second.hash, buf2));
      callCb(NULL, error);
      entry->del();
    }
    else
    {
      DOTRACE1("downloaded file '%s' %d %s err=%d", realKey, (int)it->second.lastModified, hashstr(downloadedHash, buf1), error);
      if (error == streamio::ERR_OK)
        entry->setLastModified(it->second.lastModified);
      else if (error != streamio::ERR_NOT_MODIFIED)
        entry->del(); // error occured, don't write empty file
      if (error != streamio::ERR_OK && error != streamio::ERR_NOT_MODIFIED)
        backend->filecache->del(entry->getKey()); // remove file from cache on error for make possible to redownload it
      else
        entry->free(); // close file
      entry = NULL;
      callCb(realKey, error);
    }
    delete this;
  }
};

struct NonIndexedFileDownloadRequest : public DownloadRequest
{
  SimpleString realKey; // i.e. the one with hash
  IGenSave *writeStream = nullptr;

  static const int max_opened_write_streams = 5;
  static volatile int num_opened_write_streams;

  NonIndexedFileDownloadRequest(WebBackend *back, const char *key_, const char *url) : DownloadRequest(back, url), realKey(key_)
  {
    entry = back->filecache->set(key_);

    if (max_opened_write_streams > 0 && interlocked_acquire_load(num_opened_write_streams) >= max_opened_write_streams)
    {
      return;
    }
    writeStream = entry->getWriteStream();
    if (writeStream)
      interlocked_increment(num_opened_write_streams);
  }

  virtual streamio::ProcessResult onStreamData(dag::ConstSpan<char> data)
  {
    if (!writeStream)
    {
      return streamio::ProcessResult::Discarded;
    }
    else if (writeStream->tryWrite(data.data(), data.size()) != data.size())
    {
      error = ERR_IO;
      return streamio::ProcessResult::IoError;
    }
    return streamio::ProcessResult::Consumed;
  }

  virtual void doJob()
  {
    WinAutoLockOpt lock(backend->csJob);
    if (stream)
    {
      if (!writeStream)
      {
        writeStream = entry->getWriteStream();
        if (writeStream)
          interlocked_increment(num_opened_write_streams);
      }
      IGenSave *cwr = writeStream;
      if (!cwr && dd_file_exist(entry->getPath())) // entry is already opened for write
        logwarn("web-backend: skip redownloading existing %s (for %s )", entry->getPath(), key);
      else if (!copy_stream(stream, cwr)) // write failed?
      {
        error = ERR_IO;
      }
    }
    if (error != streamio::ERR_OK && error != streamio::ERR_NOT_MODIFIED)
    {
      entry->del();
    }
    else if (error != streamio::ERR_NOT_MODIFIED && lastModified >= 0)
      entry->setLastModified(lastModified);
    entry->closeStream();
    if (writeStream)
    {
      interlocked_decrement(num_opened_write_streams);
      writeStream = NULL;
    }
  }
  virtual void releaseJob()
  {
    WinAutoLockOpt lock(backend->csMgr);
    if (error != streamio::ERR_OK && error != streamio::ERR_NOT_MODIFIED) // remove file from cache on error for make possible to
                                                                          // redownload it
      backend->filecache->del(entry->getKey());
    else
      entry->free();
    entry = NULL;
    callCb(realKey, error);
    delete this;
  }
};

volatile int NonIndexedFileDownloadRequest::num_opened_write_streams;

struct AsyncHashCalcJob : public AsyncJob
{
  uint8_t entryHash[SHA_DIGEST_LENGTH];
  AsyncHashCalcJob(WebBackend *back, Entry *ent, const char *key) : AsyncJob(back, ent, key)
  {
    G_ASSERT(entry);
    memset(entryHash, 0, sizeof(entryHash));
  }
  virtual void doJob()
  {
    dag::ConstSpan<uint8_t> edata = entry->getData();
    if (edata.size())
      SHA1(edata.data(), edata.size(), entryHash);
    entry->closeStream();
  }
  virtual void releaseJob()
  {
    WinAutoLockOpt lock(backend->csMgr);
    entry->free();
    entry = NULL;
    if (!backend->shutdowning)
    {
      char realKey[DAGOR_MAX_PATH];
      EntriesMap::iterator it = backend->entries.find(get_entry_hash_key(get_real_key(key, realKey)));
      if (it != backend->entries.end() && memcmp(entryHash, it->second.hash, SHA_DIGEST_LENGTH) == 0)
        callCb(realKey, 0);
      else
      {
        char urlBuf[URL_BUF_SIZE];
        removeThisFromInFlightJobs(); // to avoid adding cb again
        AsyncJob *job = backend->downloadFile(backend->getUrl(key, urlBuf, sizeof(urlBuf)), key,
          userCb.size() ? userCb[0] : datacache::WebBackend::UserCb{}, -1, WebBackend::DOWNLOAD_FILE);
        for (int i = 1; i < userCb.size(); ++i) // add the rest of callbacks to download request as well (with first one being added on
                                                // prev step)
          job->userCb.push_back(userCb[i]);
      }
    }
    else
      callCb(NULL, ERR_UNKNOWN);
    delete this;
  }
};

streamio::ProcessResult WebBackend::onHttpData(DownloadRequest *req, dag::ConstSpan<char> data, intptr_t /*req_id*/)
{
  WinAutoLockOpt lock(csMgr);
  G_ASSERT(req);
  return req->onStreamData(data);
}

static eastl::string to_string(eastl::string_view const &sv) { return eastl::string(sv.begin(), sv.end()); }

struct PassRespHeadersToCallback final : public cpujobs::IJob
{
  DownloadRequest *req;
  eastl::vector<eastl::pair<eastl::string, eastl::string>> respHeadersList;
  explicit PassRespHeadersToCallback(DownloadRequest *_req, streamio::StringMap const &resp_headers) : req(_req)
  {
    respHeadersList.reserve(resp_headers.size());
    for (auto const &kv : resp_headers)
      respHeadersList.push_back(eastl::make_pair(to_string(kv.first), to_string(kv.second)));
  }

  virtual void doJob() override{};
  void releaseJob() override
  {
    WinAutoLockOpt lock(req->backend->csMgr);
    req->callRespHeadersCb(streamio::StringMap(respHeadersList.begin(), respHeadersList.end()));
    delete this;
  }
};

void WebBackend::onHttpRespHeaders(streamio::StringMap const &resp_headers, DownloadRequest *req)
{
  WinAutoLockOpt lock(csMgr);
  auto job = new PassRespHeadersToCallback(req, resp_headers);
  if (req->syncJob)
  {
    job->doJob();
    // this job must be deleted inside this function
    job->releaseJob();
    return; //-V773
  }
  G_VERIFY(cpujobs::add_job(jobMgr, job));
}

void WebBackend::onHttpReqComplete(DownloadRequest *req, const char *url, int error, IGenLoad *stream, int64_t last_modified,
  intptr_t req_id)
{
  WinAutoLockOpt lock(csMgr);
  DOTRACE1("%s %s %d %p %d", __FUNCTION__, url, error, stream, (int)last_modified);
  bool returnCb =
    error == streamio::ERR_OK || error == streamio::ERR_NOT_MODIFIED || error == streamio::ERR_ABORTED || req->syncJob || shutdowning;
  if (error != streamio::ERR_ABORTED)
    activeRequests.erase_first(req_id);
  char urlBuf[URL_BUF_SIZE];
  if (!returnCb)
  {
    const eastl::string *foundBaseUrl = NULL;
    for (auto &wu : allBaseUrls)
      if (strncmp(url, wu.first.c_str(), wu.first.length()) == 0)
      {
        foundBaseUrl = &wu.first;
        break;
      }
    if (foundBaseUrl)
    {
      DOTRACE1("request to '%s' failed with error %d, try select new base url and try again", url, error);
      if (foundBaseUrl->c_str() == baseUrl.data()) // if failed url uses cur base -> choose new one (yes, cmp pointers, not data)
        returnCb = !chooseNextAvailableBaseUrl();
      if (++req->numRequests > allBaseUrls.size())
        returnCb = true;
      if (!returnCb)
      {
        const char *key = url + foundBaseUrl->length() + 1; // +1 for trailing '/'
        url = getUrl(key, urlBuf, sizeof(urlBuf));
      }
    }
    else // url does not uses base part of url
      returnCb = true;
  }
  if (returnCb)
  {
    req->stream = stream;
    req->error = error;
    req->lastModified = last_modified;
    if (req->syncJob)
    {
      req->doJob();
      req->releaseJob();
      return;
    }
    G_VERIFY(cpujobs::add_job(jobMgr, req));
  }
  else
  {
    DOTRACE1("re-request '%s' from new base %s", url, baseUrl.data());
    delete stream;
    getOrCreateStreamCtx().createStream(url, onHttpReqCompleteCb, onHttpDataCb, onHttpRespHeadersCb, nullptr, req, req->lastModified);
  }
}

/* static */
void WebBackend::onHttpReqCompleteCb(const char *url, int error, IGenLoad *stream, void *arg, int64_t last_modified, intptr_t req_id)
{
  auto req = reinterpret_cast<DownloadRequest *>(arg);
  if (req->backend->UBMagic == INIT_UB_MAGIC)
    req->backend->onHttpReqComplete(req, url, error, stream, last_modified, req_id);
  else
    DAG_FATAL("attempt to access to deleted instance!");
}

streamio::ProcessResult WebBackend::onHttpDataCb(dag::ConstSpan<char> data, void *arg, intptr_t req_id)
{
  auto req = static_cast<DownloadRequest *>(arg);
  if (req->backend->UBMagic == INIT_UB_MAGIC)
    return req->backend->onHttpData(req, data, req_id);
  else
    DAG_FATAL("attempt to access to deleted instance!");
  return streamio::ProcessResult::Discarded;
}

void WebBackend::onHttpRespHeadersCb(streamio::StringMap const &resp_headers, void *arg)
{
  auto req = reinterpret_cast<DownloadRequest *>(arg);
  if (req->backend->UBMagic == INIT_UB_MAGIC)
    req->backend->onHttpRespHeaders(resp_headers, req);
  else
    DAG_FATAL("attempt to access to deleted instance!");
}

void WebBackend::setIndexState(IndexState is)
{
  DOTRACE2("%s %d -> %d", __FUNCTION__, (int)indexState, (int)is);
  indexState = is;
}

int WebBackend::getOrCreateJobMgr(const char *jobmgr_name)
{
  if (jobMgr < 0)
  {
    char tmp[64];
    if (!jobmgr_name)
    {
      SNPRINTF(tmp, sizeof(tmp), "WebCache%#x", unsigned(uintptr_t(this) >> 4));
      jobmgr_name = tmp; //-V507
    }
    using namespace cpujobs;
    jobMgr = create_virtual_job_manager(512 << 10, WORKER_THREADS_AFFINITY_MASK, jobmgr_name, DEFAULT_THREAD_PRIORITY,
      DEFAULT_IDLE_TIME_SEC / 2);
  }
  return jobMgr;
}

streamio::Context &WebBackend::getOrCreateStreamCtx()
{
  if (!streamCtx)
  {
    streamio::CreationParams ctxParams;
    ctxParams.jobMgr = getOrCreateJobMgr();
    ctxParams.timeouts.reqTimeoutSec = timeoutSec;
    ctxParams.timeouts.connectTimeoutSec = connectTimeoutSec;
    ctxParams.timeouts.lowSpeedTimeSec = lowSpeedTimeSec;
    ctxParams.timeouts.lowSpeedLimitBps = lowSpeedLimitBps;
    ctxParams.csMgr = csMgr;
    ctxParams.csJob = csJob;
    auto ctx = streamio::create(&ctxParams);
    interlocked_release_store_ptr(streamCtx, ctx); // with write barrier due simultaneous access from poll()
    return *ctx;
  }
  return *streamCtx;
}

AsyncJob *WebBackend::getAsyncJobByKey(const char *key)
{
  for (AsyncJob *job = inFlightJobs; job; job = job->nextInFlightJob)
    if (strcmp(job->key.str(), key) == 0)
      return job;
  return NULL;
}

bool WebBackend::tryAddExistingJobCb(const char *key, UserCb const &user_cb)
{
  if (AsyncJob *job = getAsyncJobByKey(key))
  {
    DOTRACE1("async job for key '%s' already exists, add cb to it", key);
    if (user_cb.cb)
      job->userCb.push_back(user_cb);
    return true;
  }
  return false;
}

AsyncJob *WebBackend::addAsyncJob(const char *key, AsyncJob *job, UserCb const &user_cb)
{
  G_ASSERT(job);
  G_ASSERTF(!getAsyncJobByKey(key), "Attempt to add async job for already existing key '%s'", key); // this case shall be already
                                                                                                    // handled on previous step (see
                                                                                                    // calls to 'tryAddExistingJobCb')
  G_UNUSED(key);
  if (user_cb.cb)
    job->userCb.push_back(user_cb);
  job->nextInFlightJob = inFlightJobs;
  inFlightJobs = job;
  return job;
}

void WebBackend::abortActiveRequests()
{
  WinAutoLockOpt lock(csMgr);
  if (streamCtx)
  {
    for (intptr_t reqId : activeRequests)
      streamCtx->abort_request(reqId);
  }
  activeRequests.clear();
}

DownloadRequest *WebBackend::downloadFile(const char *url, const char *key, UserCb const &user_cb, int64_t modified_since,
  RequestType reqt, bool do_sync)
{
  DOTRACE1("add download request for '%s', key '%s', modsince=%lld", url, key, (long long)modified_since);
  WinAutoLockOpt lock(csMgr);

  DownloadRequest *req = NULL;
  switch (reqt)
  {
    case DOWNLOAD_FILE: req = new FileDownloadRequest(this, key); break;
    case DOWNLOAD_INDEX: req = new IndexDownloadRequest(this, key); break;
    case DOWNLOAD_NON_INDEXED_URL: req = new NonIndexedFileDownloadRequest(this, key, url); break;
    case DOWNLOAD_NON_INDEXED_FILE: req = new NonIndexedFileDownloadRequest(this, key, key); break;
    default: G_ASSERT(0); return nullptr;
  };
  if (do_sync)
  {
    streamio::Context &ctx = getOrCreateStreamCtx();
    lock.unlockFinal();
    req->syncJob = true;
    auto hdrCb = user_cb.resp_headers_cb ? onHttpRespHeadersCb : nullptr;
    ctx.createStream(url, onHttpReqCompleteCb, onHttpDataCb, hdrCb, nullptr, req, modified_since, true);
    return NULL;
  }

  addAsyncJob(key, req, user_cb);

  req->lastModified = modified_since; // save for possible re-request
  auto hdrCb = user_cb.resp_headers_cb ? onHttpRespHeadersCb : nullptr;
  intptr_t reqId = getOrCreateStreamCtx().createStream(url, onHttpReqCompleteCb, onHttpDataCb, hdrCb, nullptr, req, req->lastModified);
  if (reqId != 0)
    activeRequests.push_back(reqId);
  return req;
}

IndexDownloadRequest *WebBackend::downloadIndex(const char *key, UserCb const &user_cb)
{
  if (noIndex)
    return NULL;
  Entry *localIndex = filecache->get(INDEX_FILE_NAME);
  if (returnStaleData && localIndex) // use old index until newer version is received
  {
    IGenLoad *zstream = gzip_stream_create(localIndex->getReadStream());
    if (zstream)
    {
      parse_index(entries, zstream);
      delete zstream;
    }
  }
  if (indexState != INDEX_LOADED)
    setIndexState((returnStaleData && localIndex) ? INDEX_LOADED_LOCAL : INDEX_LOAD_IN_PROGRESS);
  int64_t lastModified = localIndex ? localIndex->getLastModified() : -1;
  if (localIndex)
    localIndex->free();
  char urlBuf[URL_BUF_SIZE];
  return (IndexDownloadRequest *)downloadFile(getUrl(INDEX_FILE_NAME, urlBuf, sizeof(urlBuf)), returnStaleData ? "" : key,
    returnStaleData ? UserCb{} : user_cb, lastModified, DOWNLOAD_INDEX);
}

#define RETURN_ENTRY(ent, err) \
  do                           \
  {                            \
    if (error)                 \
      *error = err;            \
    return ent;                \
  } while (0)


struct CbWithFileFallbackArg
{
  CbWithFileFallbackArg(WebBackend &backend_, const char *real_key, datacache::WebBackend::UserCb const &user_cb) :
    backend(backend_), realKey(real_key), nestedCb(user_cb.cb), nestedArg(user_cb.arg)
  {}

  WebBackend &backend;
  SimpleString realKey;
  completion_cb_t nestedCb;
  void *nestedArg;
};


static void cb_with_file_fallback(const char *key, ErrorCode error, datacache::Entry *entry, void *arg_)
{
  G_ASSERT(arg_ != NULL);
  eastl::unique_ptr<CbWithFileFallbackArg> arg((CbWithFileFallbackArg *)arg_);
  if (error != ERR_OK)
  {
    ErrorCode fileErr = ERR_OK;
    if (Entry *local = arg->backend.filecache->get(arg->realKey, &fileErr))
    {
      G_ASSERT(!entry);
      arg->nestedCb(key, ERR_OK, local, arg->nestedArg);
      return;
    }
  }
  arg->nestedCb(key, error, entry, arg->nestedArg);
}


Entry *WebBackend::getEntryNoIndex(const char *key, const char *url, ErrorCode *error, UserCb const &user_cb)
{
  if (!filecache->hasFreeSpace())
    RETURN_ENTRY(NULL, ERR_MEMORY_LIMIT);

  char realKey[DAGOR_MAX_PATH], urlHashStr[SHA_DIGEST_LENGTH * 2 + 1];
  uint8_t urlHash[SHA_DIGEST_LENGTH];

  if (key)
    get_real_key(key, realKey);
  else
  {
    SHA1((const uint8_t *)url, strlen(url), urlHash);
    hashstr(urlHash, urlHashStr);
    SNPRINTF(realKey, sizeof(realKey), "%s/%.2s/%s", RAW_HTTP_CACHE, urlHashStr, urlHashStr + 2);
  }

  WinAutoLockOpt lock(csMgr);
  if (tryAddExistingJobCb(key ? realKey : url, user_cb))
    RETURN_ENTRY(NULL, ERR_PENDING);

  if (user_cb.cb && noIndex && !returnStaleData)
  {
    lock.unlockFinal();
    UserCb downloadCb{cb_with_file_fallback, NULL, new CbWithFileFallbackArg(*this, realKey, user_cb)};
    downloadFile(url, realKey, downloadCb, -1, DOWNLOAD_NON_INDEXED_FILE);
    RETURN_ENTRY(NULL, ERR_PENDING);
  }

  Entry *local = filecache->get(realKey, error);
  if (local || !user_cb.cb) // cache hit or test existence
    return local;

  if (csMgr && !is_main_thread())
  {
    lock.unlock();
    downloadFile(url, realKey, UserCb{}, -1, key ? DOWNLOAD_NON_INDEXED_FILE : DOWNLOAD_NON_INDEXED_URL, true);
    lock.lock();
    ErrorCode fileErr = ERR_UNKNOWN;
    local = filecache->get(realKey, &fileErr);
    RETURN_ENTRY(local, local ? ERR_OK : fileErr);
  }
  downloadFile(url, realKey, user_cb, -1, key ? DOWNLOAD_NON_INDEXED_FILE : DOWNLOAD_NON_INDEXED_URL);
  RETURN_ENTRY(NULL, ERR_PENDING);
}

Entry *WebBackend::get(const char *key, ErrorCode *error, completion_cb_t cb, void *cb_arg)
{
  return getWithRespHeaders(key, error, cb, NULL, cb_arg);
}

Entry *WebBackend::getWithRespHeaders(const char *key, ErrorCode *error, completion_cb_t cb, resp_headers_cb_t resp_headers_cb,
  void *cb_arg)
{
  DOTRACE3("%s '%s'", __FUNCTION__, key);
  if (shutdowning)
    RETURN_ENTRY(NULL, ERR_UNKNOWN);

  UserCb userCb{cb, resp_headers_cb, cb_arg};
  char urlBuf[URL_BUF_SIZE], keyBuf[DAGOR_MAX_PATH];
  if (is_url(key))
    return getEntryNoIndex(NULL, key, error, userCb);
  else if (baseUrl.empty())
  {
    G_ASSERT(noIndex);
    RETURN_ENTRY(NULL, ERR_UNKNOWN);
  }

  // skip key (if set)
  int skl = skipKey.length();
  if (skl && strncmp(key, skipKey.str(), skl) == 0)
    key = key + skl;

  if (noIndex)
    return getEntryNoIndex(key, getUrl(key, urlBuf, sizeof(urlBuf)), error, userCb);

  G_ASSERT(is_main_thread());
  WinAutoLockOpt lock(csMgr);
  const char *realKey = get_real_key(key, keyBuf);
  if (tryAddExistingJobCb(realKey, userCb))
    RETURN_ENTRY(NULL, ERR_PENDING);

  Entry *local = filecache->get(realKey, error);
  if (!cb) // special semantic - if callback wasn't passed just route request to filecache
    return local;
  int64_t localLastModified = local ? local->getLastModified() : -1;
  if (indexState == INDEX_NOT_LOADED) // download index on first request
  {
    downloadIndex(key, userCb);
    if (!returnStaleData)
      goto pending;
  }
  // Note: if returnStaleData is true than assume that's we ok with little out sync and don't add requests for
  // unknown (b/c index is not downloaded yet!) files that we have at least some version of (to save some CDN traffic)
  if (indexState == INDEX_LOAD_IN_PROGRESS && returnStaleData && local)
    RETURN_ENTRY(local, ERR_OK);
  else if (indexState == INDEX_LOADED || indexState == INDEX_LOADED_LOCAL) // we know exactly what we have
  {
    EntriesMap::iterator it = entries.find(get_entry_hash_key(realKey));
    if (it == entries.end()) // unknown key
    {
      DOTRACE2("%s(%s) unknown key (not found in index)", __FUNCTION__, realKey);
      if (returnStaleData)
      {
        if (local || indexState != INDEX_LOADED_LOCAL)
          RETURN_ENTRY(local, local ? ERR_OK : ERR_UNKNOWN);
        // otherwise assume that some unknown files was added -> download them
      }
      else
      {
        if (local)
          local->free();
        RETURN_ENTRY(NULL, ERR_UNKNOWN);
      }
    }
    else if (local && local->getDataSize() == it->second.size)
    {
      if (localLastModified == it->second.lastModified)
      {
        DOTRACE2("%s(%s) cache hit (in local file cache)", __FUNCTION__, realKey);
        RETURN_ENTRY(local, ERR_OK);
      }
      else // fast check failed -> check hash (somewhat slow)
      {
        if (maxSyncCheckSize == 0 || local->getDataSize() <= maxSyncCheckSize)
        {
          uint8_t localHash[SHA_DIGEST_LENGTH];
          dag::ConstSpan<uint8_t> fdata = local->getData();
          bool eq = fdata.size() && memcmp(SHA1(fdata.data(), fdata.size(), localHash), it->second.hash, SHA_DIGEST_LENGTH) == 0;
          DOTRACE2("%s(%s) cache hit, but file time/size is mismatched, hashes eq = %d", __FUNCTION__, realKey, (int)eq);
          if (eq)
            local->setLastModified(it->second.lastModified);
          local->free();
          local = (eq || returnStaleData) ? filecache->get(realKey, error) : NULL; // re-get entry (in order to avoid assert on double
                                                                                   // open)
          if (eq)
            return local;
          else
            localLastModified = -1; // hash check failed -> it doesn't matter what time we have in cache
        }
        else // too big to calc hash synchronously -> calc asynchronously
        {
          DOTRACE2("add async calc hash job for '%s'", key);
          AsyncJob *job = addAsyncJob(key, new AsyncHashCalcJob(this, local, key), userCb);
          G_VERIFY(cpujobs::add_job(getOrCreateJobMgr(), job));
          RETURN_ENTRY(NULL, ERR_PENDING);
        }
      }
    }
    else // size check failed -> do not perfom since-modified check (i.e. download new one)
      localLastModified = -1;
  }
  downloadFile(getUrl(key, urlBuf, sizeof(urlBuf)), key, userCb, localLastModified, DOWNLOAD_FILE);
pending:
  if (!returnStaleData && local)
  {
    local->free();
    local = NULL;
  }
  RETURN_ENTRY(local, ERR_PENDING);
}

Entry *WebBackend::set(const char *key, int64_t modtime)
{
  WinAutoLockOpt lock(csMgr);
  return noIndex ? filecache->set(key, modtime) : NULL;
}

void WebBackend::control(int opcode, void *p0, void *p1)
{
  WinAutoLockOpt lock(csMgr);
  switch (opcode)
  {
    case _MAKE4C('DLIX'): // download index
    {
      if (indexState != INDEX_LOAD_IN_PROGRESS && indexState != INDEX_LOADED_LOCAL) // i.e. no index download in progress already
      {
        G_STATIC_ASSERT(sizeof(completion_cb_t) == sizeof(p0));
        IndexDownloadRequest *ireq = downloadIndex();
        if (ireq)
          ireq->onIndexDownloaded = UserCb{(completion_cb_t)p0, NULL, p1};
        else
          ((completion_cb_t)p0)("", ERR_UNKNOWN, NULL, p1);
      }
    }
    break;

    case _MAKE4C('SYSZ'):
    {
      int oldMaxSyncCheckSize = maxSyncCheckSize;
      maxSyncCheckSize = *(int *)p0;
      *(int *)p0 = oldMaxSyncCheckSize;
    }
    break;

    default: filecache->control(opcode, p0, p1);
  }
}

void WebBackend::poll()
{
  if (auto ctx = interlocked_acquire_load_ptr(streamCtx))
    ctx->poll();
}

bool WebBackend::del(const char *key) { return noIndex ? filecache->del(key) : false; }

void WebBackend::delAll()
{
  if (noIndex)
    filecache->delAll();
}

int WebBackend::getEntriesCount() { return noIndex ? filecache->getEntriesCount() : (int)entries.size(); }

Entry *WebBackend::nextEntry(void **iter) { return noIndex ? filecache->nextEntry(iter) : NULL; }

void WebBackend::endEnumeration(void **iter)
{
  if (noIndex)
    filecache->endEnumeration(iter);
}

} // namespace datacache
