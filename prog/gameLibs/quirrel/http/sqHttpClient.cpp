// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/http/sqHttpClient.h>
#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector_map.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <quirrel/sqasync_helpers.h>
#include <sqasync.h>
#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <squirrel.h>
#include <sqstdblob.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqStackChecker.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_delayedAction.h>


namespace bindquirrel
{

struct Request
{
  httprequests::RequestId rid = 0;
  Sqrat::Function callback;
  Sqrat::Object context;
  eastl::string respEventId;
  eastl::vector<eastl::string> stringStorage;
  bool waitable = false;
};

static eastl::vector<eastl::unique_ptr<Request>> active_requests;

// Heap context for a single httpFetch invocation. FutureHolder owns the
// strong ref on the awaiting Future.
//
// Lifecycle:
//   1. [VM thread]      http_fetch creates the ctx, kicks off async_request.
//   2. [worker thread]  HTTP completion fills POD result fields and posts
//                       settle_fetch_on_vm to sqasync's inbox.
//   3. [VM thread]      settle_fetch_on_vm builds the response object,
//                       settles the holder, erases from active_fetches.
struct FetchCtx
{
  sqasync_helpers::FutureHolder holder;
  // POD result fields filled on the worker thread before post_from_any_thread:
  httprequests::RequestStatus status = httprequests::RequestStatus::SUCCESS;
  int httpCode = 0;
  eastl::vector<char> bodyOwned;
  eastl::vector<eastl::pair<eastl::string, eastl::string>> headersOwned;
  httprequests::RequestId rid = 0;
  explicit FetchCtx(HSQUIRRELVM vm) : holder(vm) {}
};

static eastl::vector<eastl::unique_ptr<FetchCtx>> active_fetches;

static eastl::vector_map<HSQUIRRELVM, eastl::hash_set<eastl::string>> whitelists;

static constexpr int DEF_REQUEST_TIMEOUT_MS = 10 * 1000;
static constexpr int MAX_REQUEST_TIMEOUT_MS = 20 * 1000;

static bool blocking_wait_mode = false;


static SQRESULT is_url_allowed(HSQUIRRELVM vm, const char *url)
{
  auto it = whitelists.find(vm);
  if (it == whitelists.end())
    return SQ_OK;

  const char *afterProto = nullptr;
  if (strnicmp(url, "http:", 5) == 0)
    afterProto = url + 5;
  else if (strnicmp(url, "https:", 6) == 0)
    afterProto = url + 6;
  else
    return sq_throwerror(vm, "Unknown scheme (only http and https are supported)");

  if (strncmp(afterProto, "//", 2) != 0)
    return sq_throwerror(vm, "Host name / address required");

  const char *host = afterProto + 2;
  const char *afterHost = host;
  for (; *afterHost != 0 && *afterHost != ':' && *afterHost != '/'; ++afterHost)
    ;

  eastl::string hostStr(host, afterHost - host);
  if (it->second.find(hostStr) == it->second.end())
    return sqstd_throwerrorf(vm, "Request to host '%s' is forbidden", hostStr.c_str());

  return SQ_OK;
}


// Parses the HTTP params shared by httpRequest and httpFetch: url + whitelist,
// method (POST default), userAgent, timeout_ms, needResponseHeaders, headers,
// data (string / blob / table-as-form-urlencoded), json, and Content-Type
// defaulting. stringStorage receives owned copies of header values, body data,
// and JSON; reqParams.postData spans / reqParams.headers pairs point into it.
// Storage is reserved up front so no reallocation invalidates those pointers.
//
// Returns true on success. On failure, sq_throwerror has already been called
// and the caller should return SQ_ERROR.
static bool parse_http_params(HSQUIRRELVM vm, Sqrat::Table &params, httprequests::AsyncRequestParams &reqParams,
  eastl::vector<eastl::string> &stringStorage)
{
  using namespace httprequests;

  Sqrat::Var<const char *> varUrl = params.RawGetSlot("url").GetVar<const char *>();
  if (!varUrl.value || !*varUrl.value)
  {
    sq_throwerror(vm, "No url provided");
    return false;
  }
  if (SQ_FAILED(is_url_allowed(vm, varUrl.value)))
    return false;
  reqParams.url = varUrl.value;

  Sqrat::Object methodObj = params.RawGetSlot("method");
  const char *method = sq_objtostring(&methodObj.GetObject());
  if (!method || strcmp(method, "POST") == 0)
    reqParams.reqType = HTTPReq::POST; //-V1048
  else if (strcmp(method, "GET") == 0)
    reqParams.reqType = HTTPReq::GET;
  else if (strcmp(method, "HEAD") == 0)
    reqParams.reqType = HTTPReq::HEAD;
  else
  {
    sq_throwerror(vm, "Only GET, POST and HEAD methods are supported");
    return false;
  }

  Sqrat::Object userAgent = params.RawGetSlot("userAgent");
  if (userAgent.GetType() == OT_STRING)
    reqParams.userAgent = sq_objtostring(&userAgent.GetObject());

  Sqrat::Object timeout = params.RawGetSlot("timeout_ms");
  if (timeout.GetType() == OT_INTEGER)
    reqParams.reqTimeoutMs = eastl::clamp((int)sq_objtointeger(&timeout.GetObject()), 0, MAX_REQUEST_TIMEOUT_MS);
  else
    reqParams.reqTimeoutMs = DEF_REQUEST_TIMEOUT_MS;

  reqParams.needResponseHeaders = true; //-V1048 To consider: flip this default to false
  if (Sqrat::Object needResponseHeaders = params.RawGetSlot("needResponseHeaders"); !needResponseHeaders.IsNull())
    reqParams.needResponseHeaders = needResponseHeaders.Cast<bool>();

  Sqrat::Object dataObj = params.RawGetSlot("data");
  Sqrat::Object jsonObj = params.RawGetSlot("json");
  Sqrat::Table tblHeaders = params.GetSlot("headers");
  bool isBlob = false;

  if (!dataObj.IsNull() && !jsonObj.IsNull())
  {
    sq_throwerror(vm, "Only one of 'data' and 'json' arguments can be provided");
    return false;
  }

  stringStorage.reserve((tblHeaders.GetType() == OT_TABLE ? tblHeaders.Length() : 0) + (dataObj.GetType() != OT_NULL ? 1 : 0) +
                        (jsonObj.GetType() != OT_NULL ? 1 : 0));

  {
    SQObjectType dataObjType = dataObj.GetType();
    if (dataObjType == OT_TABLE || dataObjType == OT_CLASS || dataObjType == OT_INSTANCE)
    {
      if (dataObjType == OT_INSTANCE)
      {
        SqStackChecker chk(vm);
        SQUserPointer blobDataPtr = nullptr;
        sq_pushobject(vm, dataObj.GetObject());
        if (SQ_SUCCEEDED(sqstd_getblob(vm, -1, &blobDataPtr)))
        {
          SQInteger blobDataSize = sqstd_getblobsize(vm, -1);
          isBlob = true;
          eastl::string outBuf;
          outBuf.resize(blobDataSize + 1, 0);
          memcpy(&outBuf[0], blobDataPtr, blobDataSize);
          stringStorage.emplace_back(outBuf);
          reqParams.postData = make_span(stringStorage.back().c_str(), blobDataSize);
        }
        sq_pop(vm, 1);
      }

      if (!isBlob)
      {
        SqStackChecker chk(vm);
        // if content is provided via 'data' field it will be form-urlencoded
        Sqrat::Object::iterator it;
        eastl::string outBuf;
        for (; dataObj.Next(it);)
        {
          sq_pushobject(vm, it.getKey());
          sq_pushobject(vm, it.getValue());

          if (SQ_FAILED(sq_tostring(vm, -2)))
          {
            sq_throwerror(vm, "Failed to convert data object key to string");
            return false;
          }
          if (SQ_FAILED(sq_tostring(vm, -2)))
          {
            sq_throwerror(vm, "Failed to convert data object value to string");
            return false;
          }

          const char *strKey = nullptr, *strValue = nullptr;
          G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -2, &strKey)));
          G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &strValue)));

          if (!outBuf.empty())
            outBuf += "&";
          outBuf.append(strKey);
          outBuf.append("=");
          outBuf.append(strValue);

          sq_pop(vm, 4);
        }
        stringStorage.emplace_back(outBuf);
        reqParams.postData = make_span(stringStorage.back().c_str(), stringStorage.back().size());
      }
    }
    else if (dataObjType == OT_STRING)
    {
      stringStorage.emplace_back(dataObj.Cast<eastl::string>());
      reqParams.postData = make_span(stringStorage.back().c_str(), stringStorage.back().size());
    }
    else if (dataObjType != OT_NULL)
    {
      sqstd_throwerrorf(vm, "Unsupported 'data' type %s (%#X), only string, table, class and instance are supported",
        sq_objtypestr(dataObjType), dataObjType);
      return false;
    }
  }

  {
    SQObjectType jsonObjType = jsonObj.GetType();
    if (jsonObjType == OT_TABLE || jsonObjType == OT_CLASS || jsonObjType == OT_INSTANCE)
    {
      // if content is provided via 'json' field it will be converted to json string
      stringStorage.emplace_back(quirrel_to_jsonstr(jsonObj));
      reqParams.postData = make_span(stringStorage.back().c_str(), stringStorage.back().size());
    }
    else if (jsonObjType == OT_STRING)
    {
      stringStorage.emplace_back(jsonObj.Cast<eastl::string>());
      reqParams.postData = make_span(stringStorage.back().c_str(), stringStorage.back().size());
    }
    else if (jsonObjType != OT_NULL)
    {
      sqstd_throwerrorf(vm, "Unsupported 'json' type %s (%#X), only string, table, class and instance are supported",
        sq_objtypestr(jsonObjType), jsonObjType);
      return false;
    }
  }

  bool contentTypeProvided = false;
  if (tblHeaders.GetType() == OT_TABLE)
  {
    Sqrat::Object::iterator it;
    for (; tblHeaders.Next(it);)
    {
      if (sq_type(it.getKey()) != OT_STRING)
      {
        sq_throwerror(vm, "Header name is not a string");
        return false;
      }

      HSQOBJECT hValue = it.getValue();
      Sqrat::Object valueObj(hValue, vm);
      stringStorage.emplace_back(valueObj.Cast<eastl::string>());
      reqParams.headers.push_back({it.getName(), stringStorage.back().c_str()});

      if (stricmp("Content-Type", it.getName()) == 0)
        contentTypeProvided = true;
    }
  }

  if (!contentTypeProvided)
  {
    if (!jsonObj.IsNull())
      reqParams.headers.push_back({"Content-Type", "application/json"});
    else if (isBlob)
      reqParams.headers.push_back({"Content-Type", "application/octet-stream"});
  }

  return true;
}


/* qdox @module dagor.http
@function requestHttp

@kwarged

@param method s : ("POST" (default), "GET", "HEAD"), optional
@param url s
@param userAgent s : (optional)
@param headers t : {string:string} (optional)
@param data s|t|x|y : (optional) : request data which is treated as from-urlencoded data. If specified, json field is omitted
@param json s|t|x|y : (optional) : request data which is treated as json data. If specified, data field is omitted
@param callback . : ({status, http_code, response, headers}), optional
@param context t : user data specific for request response, optional
@param respEventId s : (optional) : event id to send into eventbus
@param timeout_ms i : (optional), DEF_REQUEST_TIMEOUT_MS by default (10 seconds)
@param waitable b : (optional, false by default) : if true then this request will be waited for on app shutdown
@param needResponseHeaders (optional, true by default) - specify false if you not need http response headers (optimization)

@return i : request id

response in eventbus data or in callback will be table
@code response
  {
    "status" : integer (SUCCESS, FAILED, ABORTED)
    "http_code" : integer
    "context" : null | table | string
    "headers" : table, optional
    "reponce" : blob, optional
  }
code@
*/
///@resetscope

static SQInteger request(HSQUIRRELVM vm)
{
  using namespace httprequests;

  Sqrat::Var<Sqrat::Table> varParams(vm, 2);
  Sqrat::Table &params = varParams.value;
  if (params.GetType() != OT_TABLE)
    return sq_throwerror(vm, "First (and only) argument shall be table");

  AsyncRequestParams reqParams;
  reqParams.sendResponseInMainThreadOnly = true;
  eastl::vector<eastl::string> stringStorage;

  if (!parse_http_params(vm, params, reqParams, stringStorage))
    return SQ_ERROR;

  Sqrat::Object cbFunc = params.RawGetSlot("callback");
  if (cbFunc.GetType() != OT_NULL && cbFunc.GetType() != OT_CLOSURE && cbFunc.GetType() != OT_NATIVECLOSURE &&
      cbFunc.GetType() != OT_CLASS && cbFunc.GetType() != OT_INSTANCE)
  {
    return sq_throwerror(vm, "Provided callback is not of callable type");
  }

  Sqrat::Object objRespEventId = params.RawGetSlot("respEventId");
  if (objRespEventId.GetType() != OT_NULL && objRespEventId.GetType() != OT_STRING)
    return sq_throwerror(vm, "'respEventId' must be a string");

  Request *req = new Request();
  if (cbFunc.GetType() != OT_NULL)
    req->callback = Sqrat::Function(vm, Sqrat::Object(vm), cbFunc);
  eastl::swap(stringStorage, req->stringStorage);

  if (objRespEventId.GetType() == OT_STRING)
    req->respEventId = sq_objtostring(&objRespEventId.GetObject());

  Sqrat::Object waitable = params.RawGetSlot("waitable");
  if (waitable.GetType() == OT_BOOL)
    req->waitable = sq_objtobool(&waitable.GetObject());

  req->context = params.GetSlot("context");

  reqParams.callback =
    make_http_callback([req](RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &resp_headers) {
      auto cb = [&](sqeventbus::Value &resp) {
        auto vm = resp.getVM();
        resp["status"] = (SQInteger)status;
        resp["http_code"] = (SQInteger)http_code;
        resp["context"] = req->context;

        if (!resp_headers.empty())
        {
          sqeventbus::Value sqRespHeaders(vm);
          for (auto &kv : resp_headers)
            sqRespHeaders[kv.first.data()] = kv.second.data();
          resp["headers"] = sqRespHeaders; // TODO: rename to `resp_headers`
        }
        if (!response.empty())
        {
          SQUserPointer ptr = sqstd_createblob(vm, response.size());
          G_ASSERT(ptr); // can happen if blob library was not registered
          if (ptr)
          {
            memcpy(ptr, response.data(), response.size());
            resp["body"] = Sqrat::Var<Sqrat::Object>(vm, -1).value;
            sq_pop(vm, 1);
          }
        }
        resp.obj.FreezeSelf();
      };
      if (status != RequestStatus::SHUTDOWN)
      {
        if (!req->callback.IsNull())
        {
          sqeventbus::Value resp(req->callback.GetVM());
          cb(resp);
          if (!req->callback(resp.obj))
            LOGERR_CTX("Failed to call HTTP request callback");
        }
        if (!req->respEventId.empty())
          sqeventbus::write_event_main_thread(req->respEventId.c_str(), cb);
      }
      auto it = eastl::find_if(active_requests.begin(), active_requests.end(),
        [req](eastl::unique_ptr<Request> &p) { return p.get() == req; });
      G_ASSERT(it != active_requests.end());
      active_requests.erase(it);
    });

  req->rid = httprequests::async_request(reqParams);
  active_requests.emplace_back(req);
  sq_pushinteger(vm, req->rid);

  if (blocking_wait_mode)
    http_client_wait_active_requests(0);

  return 1;
}


// Settle a fetch promise. Runs on the VM thread via sqasync::pump's inbox
// drain (posted by the worker-side completion lambda via post_from_any_thread).
// Always resolves with a frozen response table {status, http_code, headers?,
// body?}: status is the RequestStatus enum as integer (HTTP_SUCCESS /
// HTTP_FAILED / HTTP_ABORTED / HTTP_SHUTDOWN, same as httpRequest's status),
// http_code is 0 when no server response was received, and headers / body are
// only present when non-empty -- mirroring httpRequest's callback shape.
// Documented HTTP failure (4xx/5xx) is a resolved value, not a fault.
// resolve is a no-op if the holder has been disowned by onVmShutdown(), so
// this is safe to call after http_client_on_vm_shutdown.
static void settle_fetch_on_vm(void *user)
{
  auto *ctx = static_cast<FetchCtx *>(user);
  HSQUIRRELVM vm = ctx->holder.vm();

  Sqrat::Table resp(vm);
  resp.SetValue("status", (SQInteger)ctx->status);
  resp.SetValue("http_code", (SQInteger)ctx->httpCode);

  if (!ctx->headersOwned.empty())
  {
    Sqrat::Table hdrs(vm);
    for (const auto &kv : ctx->headersOwned)
      hdrs.SetValue(kv.first.c_str(), kv.second.c_str());
    resp.SetValue("headers", hdrs);
  }

  // Body is delivered as a Blob to match httpRequest's response shape (script
  // gets it via body.as_string() for text, or .readn / read pointer for binary).
  // Empty body leaves the slot unset (resp.body == null), again matching httpRequest.
  if (!ctx->bodyOwned.empty())
  {
    SQUserPointer ptr = sqstd_createblob(vm, ctx->bodyOwned.size());
    G_ASSERT(ptr); // null only if the blob library was not registered for this VM
    if (ptr)
    {
      memcpy(ptr, ctx->bodyOwned.data(), ctx->bodyOwned.size());
      resp.SetValue("body", Sqrat::Var<Sqrat::Object>(vm, -1).value);
      sq_pop(vm, 1);
    }
  }

  resp.FreezeSelf();
  ctx->holder.resolve(resp.GetObject());

  auto it = eastl::find_if(active_fetches.begin(), active_fetches.end(),
    [ctx](const eastl::unique_ptr<FetchCtx> &p) { return p.get() == ctx; });
  G_ASSERT(it != active_fetches.end());
  active_fetches.erase(it);
}


/* qdox @module dagor.http
@function httpFetch

Future-based fetch. Always resolves with a frozen response table; never
faults. The script checks `response.status` against `HTTP_SUCCESS` /
`HTTP_FAILED` / `HTTP_ABORTED` / `HTTP_SHUTDOWN` -- the same status values
`httpRequest`'s callback receives. Documented HTTP failure (4xx/5xx) is a
resolved `HTTP_SUCCESS` response with a non-2xx `http_code`.

Response shape matches `httpRequest`'s callback:
  - `status` : int (always present)
  - `http_code` : int (0 when no server response was received)
  - `headers` : table (optional; present only when non-empty)
  - `body` : Blob (optional; present only when non-empty). Use
            `body.as_string()` for text; `.readn` / read pointer for binary.

@param params t : same shape as httpRequest, minus callback / respEventId / context / waitable
       (the Future is the awaitable).
@return p : Future
*/
static SQInteger http_fetch(HSQUIRRELVM vm)
{
  using namespace httprequests;

  Sqrat::Var<Sqrat::Table> varParams(vm, 2);
  Sqrat::Table &params = varParams.value;
  if (params.GetType() != OT_TABLE)
    return sq_throwerror(vm, "httpFetch: first argument must be a table");

  AsyncRequestParams reqParams;
  // sqasync inbox handles the VM-thread hop, so no sendResponseInMainThreadOnly here.
  eastl::vector<eastl::string> stringStorage;

  if (!parse_http_params(vm, params, reqParams, stringStorage))
    return SQ_ERROR;

  auto ctxOwner = eastl::make_unique<FetchCtx>(vm);
  if (sq_isnull(ctxOwner->holder.instance()))
    return SQ_ERROR;                              // future_create raised
  sq_pushobject(vm, ctxOwner->holder.instance()); // for `return 1`
  FetchCtx *ctx = ctxOwner.get();
  active_fetches.emplace_back(eastl::move(ctxOwner));

  reqParams.callback = make_http_callback([ctx, storage = eastl::move(stringStorage)](RequestStatus status, int http_code,
                                            dag::ConstSpan<char> response, StringMap const &resp_headers) {
    // RUNS ON ANY THREAD: POD-only writes to ctx, no Sqrat / HSQOBJECT off the
    // VM thread. `storage` keeps request strings alive until the lambda dies.
    (void)storage;
    ctx->status = status;
    ctx->httpCode = http_code;
    ctx->bodyOwned.assign(response.begin(), response.end());
    ctx->headersOwned.reserve(resp_headers.size());
    for (const auto &h : resp_headers)
      ctx->headersOwned.emplace_back(eastl::string(h.first.data(), h.first.size()), eastl::string(h.second.data(), h.second.size()));

    // http_client_on_vm_shutdown waits for in-flight workers before
    // sqasync::shutdown, so the inbox is guaranteed open here.
    G_VERIFY(SQ_SUCCEEDED(sqasync::post_from_any_thread(ctx->holder.vm(), &settle_fetch_on_vm, ctx)));
  });

  ctx->rid = httprequests::async_request(reqParams);

  return 1; // promise instance left on stack
}


static SQInteger download(HSQUIRRELVM vm)
{
  // TODO: recover from disconnection or pause - like Steam does

  // checking param
  Sqrat::Var<Sqrat::Table> varParams(vm, 2);
  Sqrat::Table &params = varParams.value;
  if (params.GetType() != OT_TABLE)
    return sq_throwerror(vm, "First (and only) argument must be a table");

  // http request params init
  httprequests::AsyncRequestParams reqParams;
  reqParams.sendResponseInMainThreadOnly = true;
  reqParams.reqType = httprequests::HTTPReq::GET;
  reqParams.headers.push_back({"Content-Type", "application/octet-stream"});
  // TODO: timeout between chunks (seems impossible)
  // TODO: should user agent be set?

  // url extraction
  const char *url;
  {
    Sqrat::Object objUrl = params.RawGetSlot("url");
    if (objUrl.GetType() != OT_STRING)
      return sq_throwerror(vm, "Url is required and must be a string");
    url = sq_objtostring(&objUrl.GetObject());
    if (is_url_allowed(vm, url) != SQ_OK)
      return SQ_ERROR;
  }
  reqParams.url = url;

  // filepath extraction
  file_ptr_t dstFileHandle;
  const char *filepath;
  {
    Sqrat::Object objFilepath = params.RawGetSlot("filepath");
    if (objFilepath.GetType() != OT_STRING)
      return sq_throwerror(vm, "Filepath is required and must be a string");
    filepath = sq_objtostring(&objFilepath.GetObject());

    char dirBuff[DAGOR_MAX_PATH];
    dd_get_fname_location(dirBuff, filepath);
    if (!dd_dir_exist(dirBuff))
      return sq_throwerror(vm, ("Directory " + eastl::string(dirBuff) + " does not exist").c_str());
    dstFileHandle = df_open(filepath, DF_WRITE);
    if (!dstFileHandle)
      return sq_throwerror(vm, ("Failed to open file " + eastl::string(filepath)).c_str());
  }

  // events extraction
  Sqrat::Object objDoneEventId = params.RawGetSlot("doneEventId");
  if (objDoneEventId.GetType() != OT_STRING)
    return sq_throwerror(vm, "doneEventId is required and must be a string");
  Sqrat::Object objProgressEventId = params.RawGetSlot("progressEventId");
  if (objProgressEventId.GetType() != OT_STRING && objProgressEventId.GetType() != OT_NULL)
    return sq_throwerror(vm, "progressEventId must be a string");

  // other data extraction
  Sqrat::Object objId = params.RawGetSlot("id");
  if (objId.GetType() != OT_STRING)
    return sq_throwerror(vm, "id is required must be a string");

  Sqrat::Object objUpdatePeriodMs = params.RawGetSlot("updatePeriodMs");
  if (objUpdatePeriodMs.GetType() != OT_INTEGER && objUpdatePeriodMs.GetType() != OT_NULL)
    return sq_throwerror(vm, "updatePeriodMs must be an integer");
  Sqrat::Object objPreallocateFile = params.RawGetSlot("preallocateFile");
  if (objPreallocateFile.GetType() != OT_BOOL && objPreallocateFile.GetType() != OT_NULL)
    return sq_throwerror(vm, "preallocateFile must be a bool");

  // creation of data for the callback
  struct DownloadRequestData
  {
    httprequests::RequestId rid = 0;
    eastl::string id; // turn back into context?
    eastl::string doneEventId;
    eastl::string progressEventId; // optional
    eastl::string filepath;        // needed only for deletion in case of a failed download
    bool preallocateFile = true;   // optional
    file_ptr_t file = nullptr;
    int contentLength = -1;
    int totalDownloaded = 0;
    int updatePeriodMs = 500;
    int lastUpdate = -1;
  };

  auto req = eastl::make_unique<DownloadRequestData>();
  req->filepath = filepath;
  req->id = sq_objtostring(&objId.GetObject());
  req->doneEventId = sq_objtostring(&objDoneEventId.GetObject());
  if (objProgressEventId.GetType() == OT_STRING)
    req->progressEventId = sq_objtostring(&objProgressEventId.GetObject());
  req->file = dstFileHandle;
  if (objUpdatePeriodMs.GetType() == OT_INTEGER)
    req->updatePeriodMs = sq_objtointeger(&objUpdatePeriodMs.GetObject());
  if (objPreallocateFile.GetType() == OT_BOOL)
    req->preallocateFile = sq_objtobool(&objPreallocateFile.GetObject());

  class DownloadCallback final : public httprequests::IAsyncHTTPCallback
  {
  public:
    DownloadCallback(eastl::unique_ptr<DownloadRequestData> &&req) : req(std::move(req)) {}

    void onRequestDone(httprequests::RequestStatus status, int http_code, dag::ConstSpan<char> /*response*/,
      httprequests::StringMap const & /*resp_headers*/) override
    {
      // note: `onRequestDone` will be called even if the request failed or was aborted
      // so the file would be closed in any case
      df_close(req->file);

      if (status != httprequests::RequestStatus::SUCCESS)
        dd_erase(req->filepath.c_str());

      // Note: no need to `delayed_call` since `sendResponseInMainThreadOnly=true`
      sqeventbus::write_event_main_thread(req->doneEventId.c_str(), [&](auto &resp) {
        resp["status"] = (SQInteger)status;
        resp["http_code"] = (SQInteger)http_code;
        resp["id"] = req->id;
        resp["downloaded"] = (SQInteger)req->totalDownloaded;
      });
    }

    bool onResponseData(dag::ConstSpan<char> data) override
    {
      int bytesWritten = df_write(req->file, data.data(), data.size());
      if (bytesWritten < data.size())
      {
        // TODO response more grecefully
        logerr("Writing to the file failed, aborting download");
        httprequests::abort_request(req->rid);
        return true;
      }
      req->totalDownloaded += data.size();

      if (req->progressEventId.empty())
        return true;
      int curTime = get_time_msec();
      if (curTime - req->lastUpdate >= req->updatePeriodMs)
      {
        req->lastUpdate = curTime;

        delayed_call([event = req->progressEventId, id = req->id, downloaded = req->totalDownloaded]() {
          sqeventbus::write_event_main_thread(event.c_str(), [&](auto &resp) {
            resp["downloaded"] = (SQInteger)downloaded;
            resp["id"] = id;
          });
        });
      }
      return true;
    }

    void onHttpHeadersResponse(httprequests::StringMap const &resp_headers) override
    {
      // if headers are received after the first data chunk (they shouldn't), then don't allocate the whole file
      if (req->totalDownloaded > 0)
        return;

      auto contentLengthIt = eastl::find_if(resp_headers.begin(), resp_headers.end(),
        [](const eastl::pair<eastl::string_view, eastl::string_view> &p) { return p.first == "Content-Length"; });
      if (contentLengthIt != resp_headers.end())
        req->contentLength = atoi(contentLengthIt->second.data());
      else
        return;

      if (req->preallocateFile)
      {
        // allocate the whole file before writing
        df_seek_to(req->file, req->contentLength - 1);
        df_write(req->file, "\0", 1);
        df_seek_to(req->file, 0);
      }
    }

    void release() override { delete this; }

  private:
    eastl::unique_ptr<DownloadRequestData> req;
  };
  DownloadRequestData *reqPtr = req.get();
  reqParams.callback = new DownloadCallback(std::move(req));

  // actually sending the request
  httprequests::RequestId rid = httprequests::async_request(reqParams);
  reqPtr->rid = rid;
  sq_pushinteger(vm, rid);
  return 1;
}


SQInteger abort_request(HSQUIRRELVM vm)
{
  SQInteger rid = 0;
  sq_getinteger(vm, 2, &rid);
  httprequests::abort_request((httprequests::RequestId)rid);
  return 0;
}

void bind_http_client(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());
  ///@module dagor.http
  nsTbl //
    .SquirrelFuncDeclString(request, "httpRequest(params: table): int")
    .SquirrelFuncDeclString(http_fetch, "httpFetch(params: table): instance")
    .SquirrelFuncDeclString(abort_request, "httpAbort(request_id: int): null")
    .SquirrelFuncDeclString(download, "httpDownload(params: table): int")
    .Func("poll", httprequests::poll)
    ///@param request_id i
    .SetValue("HTTP_SUCCESS", (SQInteger)httprequests::RequestStatus::SUCCESS)
    .SetValue("HTTP_FAILED", (SQInteger)httprequests::RequestStatus::FAILED)
    .SetValue("HTTP_ABORTED", (SQInteger)httprequests::RequestStatus::ABORTED)
    .SetValue("HTTP_SHUTDOWN", (SQInteger)httprequests::RequestStatus::SHUTDOWN)
    /**/;
  module_mgr->addNativeModule("dagor.http", nsTbl);
}

void http_client_wait_active_requests(int timeout_ms)
{
  if (active_requests.empty())
    return;
  int deadLineTime = timeout_ms ? get_time_msec() + timeout_ms : 0;
  do
  {
    auto wit = eastl::find_if(active_requests.begin(), active_requests.end(), [](const auto &req) { return req->waitable; });
    if (wit != active_requests.end() && (!timeout_ms || get_time_msec() <= deadLineTime))
    {
      httprequests::poll();
      sleep_msec(10);
    }
    else
      break;
  } while (1);
}


void http_client_on_vm_shutdown(HSQUIRRELVM vm)
{
  for (auto &req : active_requests)
  {
    if (req->callback.GetVM() == vm || req->context.GetVM() == vm)
    {
      req->callback.Release();
      req->context.Release();
    }
  }
  // onVmShutdown() disowns the holder so a late worker settle becomes a no-op.
  // The drain below must complete BEFORE sqasync::shutdown (~SQSharedState),
  // or a worker post would race with inbox teardown.
  bool anyForVm = false;
  for (auto &ctx : active_fetches)
  {
    if (ctx->holder.vm() == vm)
    {
      ctx->holder.onVmShutdown();
      httprequests::abort_request(ctx->rid);
      anyForVm = true;
    }
  }
  if (anyForVm)
  {
    const int deadlineMs = get_time_msec() + 5000;
    while (true)
    {
      httprequests::poll();
      sqasync::pump(vm);
      bool stillAny = false;
      for (auto &ctx : active_fetches)
        if (ctx->holder.vm() == vm)
        {
          stillAny = true;
          break;
        }
      if (!stillAny)
        break;
      if (get_time_msec() > deadlineMs)
      {
        logerr("[httpFetch] vm shutdown timed out waiting for in-flight fetches; ctxs leak until process exit");
        break;
      }
      sleep_msec(1);
    }
  }
  whitelists.erase(vm);
}


bool http_has_pending_async_fetch() { return !active_fetches.empty(); }


void http_set_domains_whitelist(HSQUIRRELVM vm, const eastl::vector<eastl::string> &domains)
{
  eastl::hash_set<eastl::string> &wl = whitelists[vm];
  wl.clear();
  wl.insert(domains.begin(), domains.end());
}


void http_set_blocking_wait_mode(bool on) { blocking_wait_mode = on; }

} // namespace bindquirrel
