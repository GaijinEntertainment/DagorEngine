#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <datacache/datacache.h>
#include <folders/folders.h>
#include <json/json.h>
#include <sqEventBus/sqEventBus.h>
#include <sqModules/sqModules.h>
#include <sqDataCache/datacache.h>

namespace bindquirrel
{

static eastl::unordered_map<eastl::string, eastl::unique_ptr<datacache::Backend>> datacaches;

static void send_evbus_resp_headers(const char *entry_key, streamio::StringMap const &resp_headers)
{
  eastl::string eventName(eastl::string::CtorSprintf(), "datacache.headers.%s", entry_key); // TODO: rename to .resp_headers
  Json::Value msg;
  for (auto &header : resp_headers)
  {
    const eastl::string key(header.first.begin(), header.first.end());
    const eastl::string data(header.second.begin(), header.second.end());
    msg[key.c_str()] = data.c_str();
  }
  sqeventbus::send_event(eventName.c_str(), msg);
}

static void send_evbus_error(const char *entry_key, const char *err, datacache::ErrorCode error_code)
{
  eastl::string eventName(eastl::string::CtorSprintf(), "datacache.%s", entry_key);
  Json::Value msg;
  msg["error"] = err;
  msg["error_code"] = error_code;
  sqeventbus::send_event(eventName.c_str(), msg);
  send_evbus_resp_headers(entry_key, streamio::StringMap{}); // send empty header for unsubscribe
}

static void send_evbus_entry_info(const char *entry_key, datacache::Entry *entry)
{
  eastl::string eventName(eastl::string::CtorSprintf(), "datacache.%s", entry_key);
  Json::Value msg;
  msg["key"] = entry->getKey();
  msg["path"] = entry->getPath();
  msg["size"] = entry->getDataSize();
  sqeventbus::send_event(eventName.c_str(), msg);
  send_evbus_resp_headers(entry_key, streamio::StringMap{}); // send empty header for unsubscribe
}

void init_cache(const char *cache_name, Sqrat::Table params)
{
  if (auto it = datacaches.find(cache_name); it != datacaches.end())
    return;

  char tmps[128];

  datacache::WebBackendConfig config;
  eastl::string mountPath = params.GetSlotValue<eastl::string>("mountPath", config.mountPath);
  const char *folderPath = folders::get_path(mountPath.c_str());
  G_ASSERTF_RETURN(folderPath, , "Unknown mount folder '%s'", mountPath);
  config.mountPath = folderPath;
  config.noIndex = true;
  config.timeoutSec = params.GetSlotValue("timeoutSec", config.timeoutSec);
  config.maxSize = params.GetSlotValue("maxSize", config.maxSize);
  config.manualEviction = params.GetSlotValue("manualEviction", config.manualEviction);
  SNPRINTF(tmps, sizeof(tmps), "sq-webcache-%s", cache_name);
  config.jobMgrName = tmps;

  eastl::unique_ptr<datacache::Backend> cache(datacache::create(config));
  if (!cache)
    logerr("Failed to create datacache '%s'", cache_name);
  else
    datacaches.insert({cache_name, std::move(cache)});
}

void abort_requests(const char *cache_name)
{
  if (auto it = datacaches.find(cache_name); it != datacaches.end())
  {
    eastl::unique_ptr<datacache::Backend> &datacache = it->second;
    datacache->abortActiveRequests();
  }
}

static void on_entry_resp_headers(const char *key, streamio::StringMap const &resp_headers, void *)
{
  send_evbus_resp_headers(key, resp_headers);
}

static void on_entry_completion(const char *key, datacache::ErrorCode error, datacache::Entry *entry, void *)
{
  if (error == datacache::ERR_OK)
  {
    send_evbus_entry_info(key, entry);
    entry->free();
  }
  else
  {
    eastl::string errorMsg(eastl::string::CtorSprintf(), "failed to get file from datacache: %d", (int)error);
    send_evbus_error(key, errorMsg.c_str(), error);
  }
}

bool del_entry(const char *cache_name, const char *entry_key)
{
  auto it = datacaches.find(cache_name);
  if (it == datacaches.end())
    return false;
  return it->second->del(entry_key);
}

SQInteger get_all_entries(HSQUIRRELVM vm)
{
  const SQChar *cacheName = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &cacheName)));
  auto it = datacaches.find(cacheName);
  if (it == datacaches.end())
    return 1;

  eastl::unique_ptr<datacache::Backend> &cache = it->second;
  void *iter = NULL;
  Sqrat::Array entries(vm, cache->getEntriesCount());
  int i = 0;
  for (datacache::Entry *entry = cache->nextEntry(&iter); entry; entry = cache->nextEntry(&iter), ++i)
  {
    Sqrat::Table sqEntry(vm);
    sqEntry.SetValue("path", entry->getPath());
    sqEntry.SetValue("key", entry->getKey());
    entries.SetValue(i, sqEntry);
    entry->free();
  }
  sq_pushobject(vm, entries);
  cache->endEnumeration(&iter);
  return 1;
}

void request_entry(const char *cache_name, const char *entry_key)
{
  auto it = datacaches.find(cache_name);
  if (it == datacaches.end())
  {
    send_evbus_error(entry_key, "Unknown datacache", datacache::ERR_UNKNOWN);
    return;
  }
  eastl::unique_ptr<datacache::Backend> &datacache = it->second;
  datacache::ErrorCode error;
  datacache::EntryHolder entry(datacache->getWithRespHeaders(entry_key, &error, on_entry_completion, on_entry_resp_headers));
  if (error == datacache::ERR_OK)
  {
    send_evbus_entry_info(entry_key, entry.get());
  }
  else if (error != datacache::ERR_PENDING)
  {
    eastl::string errorMsg(eastl::string::CtorSprintf(), "failed to get file from datacache: %d", (int)error);
    send_evbus_error(entry_key, errorMsg.c_str(), error);
  }
}

void bind_datacache(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());
  ///@module datacache
  nsTbl.Func("init_cache", init_cache)
    .Func("request_entry", request_entry)
    .Func("abort_requests", abort_requests)
    .Func("del_entry", del_entry)
    .SquirrelFunc("get_all_entries", get_all_entries, 2, ".s")
    .SetValue("ERR_UNKNOWN", (SQInteger)datacache::ERR_UNKNOWN)
    .SetValue("ERR_OK", (SQInteger)datacache::ERR_OK)
    .SetValue("ERR_PENDING", (SQInteger)datacache::ERR_PENDING)
    .SetValue("ERR_IO", (SQInteger)datacache::ERR_IO)
    .SetValue("ERR_ABORTED", (SQInteger)datacache::ERR_ABORTED)
    .SetValue("ERR_MEMORY_LIMIT", (SQInteger)datacache::ERR_MEMORY_LIMIT);
  module_mgr->addNativeModule("datacache", nsTbl);
}

void shutdown_datacache() { datacaches.clear(); }

} // namespace bindquirrel
