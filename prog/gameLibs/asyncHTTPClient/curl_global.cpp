#include <asyncHTTPClient/curl_global.h>
#include <debug/dag_debug.h>

#if defined(USE_XCURL)
#include <Xcurl.h>
#else
#include <curl/curl.h>
#endif

#define TRACE(...) debug("[CURL_GLOBAL] " __VA_ARGS__)

namespace curl_global
{

static bool is_initialized = false;
static bool is_initialized_externally = false;

void init()
{
  if (!is_initialized_externally)
  {
    if (!is_initialized)
    {
      is_initialized = !::curl_global_init(CURL_GLOBAL_DEFAULT);
      TRACE("init: %d", is_initialized);
    }
    else
    {
      TRACE("is already initialized, skipping init");
    }
    return;
  }
  TRACE("was initialized externally, skipping init");
}


void shutdown()
{
  if (!is_initialized_externally)
  {
    if (is_initialized)
    {
      ::curl_global_cleanup();
      is_initialized = false;
      TRACE("shutdown");
    }
    else
    {
      TRACE("is not initialized, skipping shutdown");
    }
    return;
  }
  TRACE("was initialized externally, skipping shutdown");
}


void was_initialized_externally(bool value)
{
  G_ASSERT(!is_initialized);
  TRACE("was_initialized_externally(%d)", value);
  is_initialized_externally = value;
}


} // namespace curl_global