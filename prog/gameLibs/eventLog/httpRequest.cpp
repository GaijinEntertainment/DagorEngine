#include "httpRequest.h"
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <osApiWrappers/dag_events.h>
#include <debug/dag_debug.h>


namespace event_log
{
namespace http
{

template <typename F>
static httprequests::RequestId make_request(const char *url, const void *data, uint32_t size, const char *user_agent, uint32_t timeout,
  const F &callback)
{
  httprequests::AsyncRequestParams reqParams;
  reqParams.url = url;
  reqParams.reqType = httprequests::HTTPReq::POST; //-V1048
  reqParams.userAgent = user_agent;
  reqParams.postData = dag::ConstSpan<char>((const char *)data, size);
  reqParams.reqTimeoutMs = timeout * 1000;     // S -> MS
  reqParams.connectTimeoutMs = timeout * 1000; // S -> MS
  reqParams.needResponseHeaders = false;
  reqParams.callback = httprequests::make_http_callback(callback);
  return httprequests::async_request(reqParams);
}


void post(const char *url, const void *data, uint32_t size, const char *user_agent, uint32_t timeout)
{
  os_event_t event;
  os_event_create(&event);
  make_request(url, data, size, user_agent, timeout, [&event](httprequests::RequestStatus status, int http_code, auto, auto) {
    debug("%s finished with %d/%d", __FUNCTION__, int(status), http_code);
    os_event_set(&event);
  });

  do
  {
    httprequests::poll();
    int waitRes = os_event_wait(&event, 1);
    if (waitRes != OS_WAIT_TIMEOUTED)
      break;
  } while (1);

  os_event_destroy(&event);
}


void post_async(const char *url, const void *data, uint32_t size, const char *user_agent, uint32_t timeout)
{
  make_request(url, data, size, user_agent, timeout, [](httprequests::RequestStatus status, int http_code, auto, auto) {
    debug("%s finished with %d/%d", __FUNCTION__, int(status), http_code);
  });
}

} // namespace http
} // namespace event_log