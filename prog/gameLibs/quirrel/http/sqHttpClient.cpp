#include <quirrel/http/sqHttpClient.h>
#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector_map.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <sqModules/sqModules.h>
#include <sqrat.h>
#include <squirrel.h>
#include <sqstdblob.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqStackChecker.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>


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

/**@module dagor.http
@function requestHttp

@kwarged

@param method s : ("POST" (default), "GET", "HEAD"), optional
@param url s
@param userAgent s : (optional)
@param headers t : {string:string} (optional)
@param data s|t|x|y : (optional)
@param callback . : ({status, http_code, response, headers}), optional
@param context t : user data specific for request response, optional
@param respEventId s : (optional) : event id to send into eventbus
@param timeout_ms i : (optional), DEF_REQUEST_TIMEOUT_MS by default (10 seconds)
@param waitable b : (optional, false by default) : if true then this request will be waited for on app shutdown

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

  Sqrat::Var<const char *> varUrl = params.RawGetSlot("url").GetVar<const char *>();
  if (!varUrl.value || !*varUrl.value)
    return sq_throwerror(vm, "No url provided");
  if (SQ_FAILED(is_url_allowed(vm, varUrl.value)))
    return SQ_ERROR;

  reqParams.url = varUrl.value;

  Sqrat::Object methodObj = params.RawGetSlot("method");
  const char *method = sq_objtostring(&methodObj.GetObject());
  if (!method || strcmp(method, "POST") == 0)
    reqParams.reqType = HTTPReq::POST;
  else if (strcmp(method, "GET") == 0)
    reqParams.reqType = HTTPReq::GET;
  else if (strcmp(method, "HEAD") == 0)
    reqParams.reqType = HTTPReq::HEAD;
  else
    return sq_throwerror(vm, "Only GET, POST and HEAD methods are supported");

  Sqrat::Object userAgent = params.RawGetSlot("userAgent");
  if (userAgent.GetType() == OT_STRING)
    reqParams.userAgent = sq_objtostring(&userAgent.GetObject());

  Sqrat::Object timeout = params.RawGetSlot("timeout_ms");
  if (timeout.GetType() == OT_INTEGER)
    reqParams.reqTimeoutMs = eastl::clamp((int)sq_objtointeger(&timeout.GetObject()), 0, MAX_REQUEST_TIMEOUT_MS);
  else
    reqParams.reqTimeoutMs = DEF_REQUEST_TIMEOUT_MS;

  Sqrat::Object dataObj = params.RawGetSlot("data");
  Sqrat::Object jsonObj = params.RawGetSlot("json");
  Sqrat::Table tblHeaders = params.GetSlot("headers");
  bool isBlob = false;

  if (!dataObj.IsNull() && !jsonObj.IsNull())
    return sq_throwerror(vm, "Only one of 'data' and 'json' arguments can be provided");

  eastl::vector<eastl::string> stringStorage;
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
            return sq_throwerror(vm, "Failed to convert data object key to string");
          if (SQ_FAILED(sq_tostring(vm, -2)))
            return sq_throwerror(vm, "Failed to convert data object value to string");

          const SQChar *strKey = nullptr, *strValue = nullptr;
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
      return sqstd_throwerrorf(vm, "Unsupported 'data' type %s (%#X), only string, table, class and instance are supported",
        sq_objtypestr(dataObjType), dataObjType);
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
      return sqstd_throwerrorf(vm, "Unsupported 'json' type %s (%#X), only string, table, class and instance are supported",
        sq_objtypestr(jsonObjType), jsonObjType);
    }
  }

  bool contentTypeProvided = false;
  if (tblHeaders.GetType() == OT_TABLE)
  {
    Sqrat::Object::iterator it;
    for (; tblHeaders.Next(it);)
    {
      if (sq_type(it.getKey()) != OT_STRING)
        return sq_throwerror(vm, "Header name is not a string");

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
    make_http_callback([req](RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &headers) {
      bool haveCb = !req->callback.IsNull();
      if (haveCb || !req->respEventId.empty())
      {
        HSQUIRRELVM vm = haveCb ? req->callback.GetVM() : sqeventbus::get_vm();
        if (vm && (status != RequestStatus::SHUTDOWN))
        {
          Sqrat::Table resp(vm);
          resp.SetValue("status", (SQInteger)status);
          resp.SetValue("http_code", (SQInteger)http_code);
          resp.SetValue("context", req->context);

          if (!headers.empty())
          {
            Sqrat::Table sqHeaders(vm);
            for (auto kv : headers)
              sqHeaders.SetValue(kv.first.data(), kv.second.data());
            resp.SetValue("headers", sqHeaders);
          }
          if (response.size() > 0)
          {
            SQUserPointer ptr = sqstd_createblob(vm, response.size());
            G_ASSERT(ptr); // can happen if blob library was not registered
            if (ptr)
            {
              memcpy(ptr, response.data(), response.size());
              HSQOBJECT hBlob;
              sq_getstackobj(vm, -1, &hBlob);
              resp.SetValue("body", Sqrat::Object(hBlob, vm));
              sq_pop(vm, 1);
            }
          }

          resp.FreezeSelf();
          if (haveCb)
          {
            if (!req->callback(resp))
              logerr_ctx("Failed to call HTTP request callback");
          }
          if (!req->respEventId.empty())
            sqeventbus::send_event(req->respEventId.c_str(), resp);
        }
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
  ///@skipline
  nsTbl.SquirrelFunc("request", request, 2, ".t")
    .SquirrelFunc("httpRequest", request, 2, ".t")
    ///@skipline
    .SquirrelFunc("abort", abort_request, 2, ".i")
    .SquirrelFunc("httpAbort", abort_request, 2, ".i")
    ///@param request_id i
    .SetValue("HTTP_SUCCESS", (SQInteger)httprequests::RequestStatus::SUCCESS)
    .SetValue("HTTP_FAILED", (SQInteger)httprequests::RequestStatus::FAILED)
    .SetValue("HTTP_ABORTED", (SQInteger)httprequests::RequestStatus::ABORTED);
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
  whitelists.erase(vm);
}


void http_set_domains_whitelist(HSQUIRRELVM vm, const eastl::vector<eastl::string> &domains)
{
  eastl::hash_set<eastl::string> &wl = whitelists[vm];
  wl.clear();
  wl.insert(domains.begin(), domains.end());
}


void http_set_blocking_wait_mode(bool on) { blocking_wait_mode = on; }

} // namespace bindquirrel
