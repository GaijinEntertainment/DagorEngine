#pragma once
#include <datacache/datacache.h>
#include <EASTL/vector.h>
#include <EASTL/utility.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_critSec.h>
#include <streamIO/streamIO.h>
#include "webindex.h"

class DataBlock;
class SimpleString;

namespace datacache
{

struct AsyncJob;
struct DownloadRequest;
struct IndexDownloadRequest;

typedef eastl::vector<eastl::pair<eastl::string, float>> WeightedUrlsType;

class WebBackend final : public Backend
{
public:
  WebBackend();
  ~WebBackend();
  static Backend *create(WebBackendConfig &config);

public:
  struct UserCb
  {
    completion_cb_t cb;
    resp_headers_cb_t resp_headers_cb;
    void *arg;
  };

  enum IndexState
  {
    INDEX_NOT_LOADED,
    INDEX_LOAD_IN_PROGRESS,
    INDEX_LOADED_LOCAL,
    INDEX_LOADED
  };

  Entry *getEntryNoIndex(const char *key, const char *url, ErrorCode *error, UserCb const &user_cb);
  Entry *get(const char *key, ErrorCode *error, completion_cb_t cb, void *cb_arg) override;
  Entry *getWithRespHeaders(const char *key, ErrorCode *error, completion_cb_t cb = NULL, resp_headers_cb_t resp_headers_cb = NULL,
    void *cb_arg = NULL) override;
  Entry *set(const char *key, int64_t modTime) override;
  bool del(const char *key) override;
  void delAll() override;
  int getEntriesCount() override;
  Entry *nextEntry(void **iter) override;
  void endEnumeration(void **iter) override;
  void control(int opc, void *p0, void *p1) override;
  void poll() override;

  void onHttpReqComplete(DownloadRequest *req, const char *url, int error, IGenLoad *stream, int64_t last_modified, intptr_t req_id);
  streamio::ProcessResult onHttpData(DownloadRequest *req, dag::ConstSpan<char> data, intptr_t req_id);
  void onHttpRespHeaders(streamio::StringMap const &resp_headers, DownloadRequest *req);

  static void onHttpReqCompleteCb(const char *, int error, IGenLoad *stream, void *arg, int64_t last_modified, intptr_t req_id);
  static streamio::ProcessResult onHttpDataCb(dag::ConstSpan<char> data, void *arg, intptr_t req_id);
  static void onHttpRespHeadersCb(streamio::StringMap const &headers, void *arg);

  enum RequestType
  {
    DOWNLOAD_FILE,
    DOWNLOAD_INDEX,
    DOWNLOAD_NON_INDEXED_URL,
    DOWNLOAD_NON_INDEXED_FILE
  };
  DownloadRequest *downloadFile(const char *url, const char *key, UserCb const &user_cb, int64_t modified_since, RequestType reqt,
    bool do_sync = false);
  IndexDownloadRequest *downloadIndex(const char *key = "", UserCb const &user_cb = UserCb{});

  void abortActiveRequests() override;

  const char *getUrl(const char *url, char *buf, int buf_size);
  void collectGarbage();
  void setIndexState(IndexState is);
  int getOrCreateJobMgr(const char *jobmgr_name = nullptr);
  streamio::Context &getOrCreateStreamCtx();

  AsyncJob *getAsyncJobByKey(const char *key);
  bool tryAddExistingJobCb(const char *key, UserCb const &user_cb);
  AsyncJob *addAsyncJob(const char *key, AsyncJob *job, UserCb const &user_cb);

  bool chooseNextAvailableBaseUrl(); // return false if looped (or no base urls at all)

public:
  volatile uintptr_t UBMagic;
  Backend *filecache;
  WeightedUrlsType allBaseUrls;
  eastl::vector<const eastl::pair<eastl::string, float> *> availBaseUrls; // points to 'allBaseUrls', can't contain NULLs
  eastl::string_view baseUrl;                                             // points to element from 'allBaseUrls'
  String skipKey;
  streamio::Context *streamCtx;
  IndexState indexState;
  EntriesMap entries;
  bool returnStaleData;
  bool noIndex;
  bool gcScheduled;
  bool shutdowning;
  int jobMgr;
  int timeoutSec;
  int connectTimeoutSec;
  int maxSyncCheckSize;
  int lowSpeedTimeSec;
  int lowSpeedLimitBps;
  AsyncJob *inFlightJobs;
  WinCritSec *csMgr = NULL, *csJob = NULL;
  eastl::vector<intptr_t> activeRequests;
};

}; // namespace datacache
