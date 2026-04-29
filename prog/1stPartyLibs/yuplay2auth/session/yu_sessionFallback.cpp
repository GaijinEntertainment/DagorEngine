#include "stdafx.h"
#include "yu_sessionFallback.h"
#include "yu_fallback.h"


//==================================================================================================
YuSession::FallbackAction::FallbackAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb,
                                          const YuString& fail_ip, bool use_failback) :
    YuSession::AsyncHttpAction(s, id, msg, cb)
{
  if (use_failback)
    fallbackIp = &fail_ip;
}


//==================================================================================================
YuSession::FallbackAction::FallbackAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb,
                                          const YuStringTab& fail_hosts, const YuString& fail_ip,
                                          bool use_failback) :
    YuSession::AsyncHttpAction(s, id, msg, cb)
{
  if (use_failback)
  {
    fallbackHosts = &fail_hosts;
    fallbackIp = &fail_ip;
  }
}


//==================================================================================================
YuFallbackRequest* YuSession::FallbackAction::createFallback() const
{
  if (fallbackIp)
  {
    if (fallbackHosts)
      return new YuFallbackRequest(*fallbackHosts, *fallbackIp, true);

    return new YuFallbackRequest(*fallbackIp, true);
  }

  return new YuFallbackRequest(true);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doPost(const YuString& url, const YuStrMap& params)
{
  fallback = createFallback();
  return fallback->post(url, params, this);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doPost(const YuString& url, const YuStrMap& params,
                                               const YuStrMap& headers)
{
  fallback = createFallback();
  return fallback->post(url, params, headers, this);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doPost(const YuString& url, const YuFormData& params)
{
  fallback = createFallback();
  return fallback->post(url, params, this);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doPost(const YuString& url, const YuCharTab& body)
{
  fallback = createFallback();
  return fallback->post(url, body, this);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doPost(const YuString& url, const YuStrMap& params,
                                               unsigned conn_ms, unsigned req_ms)
{
  fallback = createFallback();
  return fallback->postTimeout(url, params, conn_ms, req_ms, this);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doGet(const YuString& url, const YuStrMap& params)
{
  fallback = createFallback();
  return fallback->get(url, params, this);
}


//==================================================================================================
YuHttpHandle YuSession::FallbackAction::doGet(const YuString& url, const YuStrMap& params,
                                              unsigned conn_ms, unsigned req_ms)
{
  fallback = createFallback();
  return fallback->getTimeout(url, params, conn_ms, req_ms, this);
}


//==================================================================================================
void YuSession::FallbackAction::stop()
{
  if (fallback)
    fallback->stop();
}
