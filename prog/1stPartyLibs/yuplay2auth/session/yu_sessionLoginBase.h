#ifndef __YUPLAY2_SESSION_LOGIN_BASE__
#define __YUPLAY2_SESSION_LOGIN_BASE__
#pragma once


#include "yu_sessionFallback.h"


class YuJson;


class YuSession::CommonLoginAction : public YuSession::FallbackAction
{
public:
  CommonLoginAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb,
                    bool use_fallback = true, bool psn = false) :
    FallbackAction(s, id, msg, cb, s.getFailbackIp(), use_fallback && s.doAuthFailback()),
    psnRestricted(psn) {}

protected:
  bool psnRestricted;

  virtual Yuplay2Status parseServerAnswer(const YuCharTab& data);

  virtual Yuplay2Status onErrorStatus(Yuplay2Status status);
  virtual void onOkStatus(const YuJson& json) {}

  void onHttpResponse(const YuString& url, const YuCharTab& data) override;
};


#endif //__YUPLAY2_SESSION_LOGIN_BASE__
