#ifndef __YUPLAY2_SESSION_PROC_FALLBACK__
#define __YUPLAY2_SESSION_PROC_FALLBACK__
#pragma once


#include "yu_sessionHttp.h"


class YuFallbackRequest;


class YuSession::FallbackAction : public YuSession::AsyncHttpAction
{
public:
  FallbackAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb, const YuString& fail_ip,
                 bool use_failback);
  FallbackAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb, const YuStringTab& h,
                 const YuString& fail_ip, bool use_failback);

protected:
  YuFallbackRequest* fallback = NULL;

  const YuStringTab* fallbackHosts = NULL;
  const YuString* fallbackIp = NULL;

  void stop() override;

  virtual const YuStrMap* getRequestHeaders() const { return NULL; }

private:
  YuHttpHandle doPost(const YuString& url, const YuStrMap& params) override;
  YuHttpHandle doPost(const YuString& url, const YuStrMap& params, const YuStrMap& headers) override;
  YuHttpHandle doPost(const YuString& url, const YuFormData& params) override;
  YuHttpHandle doPost(const YuString& url, const YuCharTab& body) override;
  YuHttpHandle doPost(const YuString& url, const YuStrMap& params, unsigned conn_ms,
                              unsigned req_ms) override;
  YuHttpHandle doGet(const YuString& url, const YuStrMap& params) override;
  YuHttpHandle doGet(const YuString& url, const YuStrMap& params, unsigned conn_ms,
                             unsigned req_ms) override;

  YuFallbackRequest* createFallback() const;
};


#endif //__YUPLAY2_SESSION_PROC_FALLBACK__
