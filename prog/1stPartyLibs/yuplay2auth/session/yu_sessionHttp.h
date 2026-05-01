#ifndef __YUPLAY2_SESSION_PROC_HTTP__
#define __YUPLAY2_SESSION_PROC_HTTP__
#pragma once


#include "yu_sessionAction.h"
#include "yu_http.h"


class YuSession::AsyncHttpAction :
    public YuSession::AsyncAction,
    public IHttpCallback
{
public:
  AsyncHttpAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb) :
    AsyncAction(s, id, msg, cb), httpAct(NULL) {}

protected:
  YuHttpHandle httpAct;
  YuCriticalSection httpActGuard;

  void setCommonLoginParams(YuStrMap& form, const char* game_id = NULL);
  void setCommonLoginParams(YuFormData& form, const char* game_id = NULL);

  bool sendPost(const YuString& url, const YuStrMap& params);
  bool sendPost(const YuString& url, const YuStrMap& params, const YuStrMap& headers);
  bool sendPost(const YuString& url, const YuFormData& params);
  bool sendPost(const YuString& url, const YuCharTab& body);
  bool sendPost(const YuString& url, const YuStrMap& params, unsigned conn_ms, unsigned req_ms);
  bool sendGet(const YuString& url, const YuStrMap& params);
  bool sendGet(const YuString& url, const YuStrMap& params, unsigned conn_ms, unsigned req_ms);

  virtual YuHttpHandle doPost(const YuString& url, const YuStrMap& params);
  virtual YuHttpHandle doPost(const YuString& url, const YuStrMap& params, const YuStrMap& headers);
  virtual YuHttpHandle doPost(const YuString& url, const YuFormData& params);
  virtual YuHttpHandle doPost(const YuString& url, const YuCharTab& body);
  virtual YuHttpHandle doPost(const YuString& url, const YuStrMap& params, unsigned conn_ms,
                              unsigned req_ms);
  virtual YuHttpHandle doGet(const YuString& url, const YuStrMap& params);
  virtual YuHttpHandle doGet(const YuString& url, const YuStrMap& params, unsigned conn_ms,
                             unsigned req_ms);

  void setHttpAct(YuHttpHandle act);

  bool onHttpStart(const YuString& url) override { return true; }
  void onHttpResponse(const YuString& url, const YuCharTab& data) override;
  void onHttpError(const YuString& url, int err_no) override;
  void onHttpProgress(const YuString& url, int progress) override {}
  void onHttpEnd(const YuString& url) override { setHttpAct(NULL); doneEvt.set(); }

  void stop() override;

  Yuplay2Status parseServerJson(const YuCharTab& answer_bytes);

  static Yuplay2Status checkServerAnswer(const YuCharTab& answer, Yuplay2Status def_error = YU2_FAIL);
  static Yuplay2Status checkJsonAnswer(const YuString& answer, Yuplay2Status def_error = YU2_FAIL);
  static Yuplay2Status curlError(int error);

private:
  template<class POST>
  void setCommonLoginParamsTpl(POST& form, const char* game_id = NULL);
};



#endif //__YUPLAY2_SESSION_PROC_HTTP__
