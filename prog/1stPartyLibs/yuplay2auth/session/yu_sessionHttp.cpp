#include "stdafx.h"
#include "yu_sessionHttp.h"
#include "yu_string.h"

#ifdef YUPLAY2_USE_XCURL
#include <XCurl.h>
#else
#include <curl/curl.h>
#endif //YUPLAY2_USE_XCURL


struct YuCodeStr
{
  const char* str;
  Yuplay2Status status;
};


//==================================================================================================
Yuplay2Status YuSession::AsyncHttpAction::checkJsonAnswer(const YuString& answer,
                                                          Yuplay2Status def_error)
{
  static YuCodeStr err[] =
  {
    { "OK", YU2_OK },
    { "ERROR", YU2_FAIL },
    { "LOGINERROR", YU2_WRONG_LOGIN },
    { "EXPIRED", YU2_EXPIRED },
    { "NOMONEY", YU2_NO_MONEY },
    { "NOITEM", YU2_EMPTY },
    { "FROZEN", YU2_FROZEN },
    { "NOTOWNER", YU2_NOT_OWNER },
    { "ERASED", YU2_PROFILE_DELETED },
    { "PSNUNKNOWN", YU2_PSN_UNKNOWN },
    { "PSN_RESTRICTED", YU2_PSN_RESTRICTED },
    { "NOEULA", YU2_NO_EULA_ACCEPTED },
    { "2STEP", YU2_2STEP_AUTH },
    { "BADPAYMENT", YU2_WRONG_PAYMENT },
    { "2STEPERROR", YU2_WRONG_2STEP_CODE },
    { "ALREADY", YU2_ALREADY },
    { "PAYLIMIT", YU2_PAYMENT_LIMIT },
    { "NOTFOUND", YU2_NOT_FOUND },
    { "BADDOMAIN", YU2_BAD_DOMAIN },
    { "WRONGEMAIL", YU2_WRONG_EMAIL },
    { "LOGININUSE", YU2_ALREADY },
    { "DOIINCOMPLETE", YU2_DOI_INCOMPLETE },
    { "NEED_2STEP", YU2_FORBIDDEN_NEED_2STEP },
    { "RETRY", YU2_RETRY },
    { "UPDATE", YU2_UPDATE },
    { "FORBIDDEN", YU2_FORBIDDEN },
  };

  for (int i = 0; i < sizeof(err) / sizeof(err[0]); ++i)
    if (answer == err[i].str)
      return err[i].status;

  return def_error;
}


//==================================================================================================
Yuplay2Status YuSession::AsyncHttpAction::checkServerAnswer(const YuCharTab& answer,
                                                            Yuplay2Status def_error)
{
  if (answer.empty())
    return def_error;

  static YuCodeStr err[] =
  {
    { "DONE", YU2_OK },
    { "ERROR", YU2_FAIL },
    { "LOGINERROR", YU2_WRONG_LOGIN },
    { "WRONG", YU2_WRONG_PARAMETER },
    { "EMPTY", YU2_EMPTY },
    { "ALREADY", YU2_ALREADY },
    { "FORBIDDEN", YU2_FORBIDDEN },
    { "FROZEN", YU2_FROZEN },
    { "NOITEM", YU2_EMPTY }
  };

  for (int i = 0; i < sizeof(err) / sizeof(err[0]); ++i)
    if (YuStr::begins(answer, err[i].str))
      return err[i].status;

  return def_error;
}


//==================================================================================================
Yuplay2Status YuSession::AsyncHttpAction::curlError(int error)
{
#ifndef YU2_XBOX

  struct ErrCode
  {
    int curlError;
    Yuplay2Status yuError;
  };

  static ErrCode errors[] =
  {
    { CURLE_SSL_CACERT, YU2_SSL_CACERT },
    { CURLE_OPERATION_TIMEDOUT, YU2_TIMEOUT },
    { CURLE_SSL_CONNECT_ERROR, YU2_SSL_ERROR },
    { CURLE_PEER_FAILED_VERIFICATION, YU2_SSL_ERROR},
    { CURLE_SSL_CACERT_BADFILE, YU2_SSL_CACERT_FILE },
    { CURLE_COULDNT_RESOLVE_HOST, YU2_HOST_RESOLVE },
    { CURLE_REMOTE_FILE_NOT_FOUND, YU2_EMPTY }
  };

  for (int i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
    if (errors[i].curlError == error)
      return errors[i].yuError;

#endif //YU2_XBOX

  return YU2_FAIL;
}


//==================================================================================================
void YuSession::AsyncHttpAction::stop()
{
  YuWarden w(httpActGuard);

  //Stop HTTP action if it has been started (delete self will be called in onHttpEnd() callback)
  //Or delete self if HTTP action hasn't been started
  if (httpAct)
    YuHttp::self().cancel(httpAct);
}


//==================================================================================================
void YuSession::AsyncHttpAction::setHttpAct(YuHttpHandle act)
{
  YuWarden w(httpActGuard);
  httpAct = act;
}


//==================================================================================================
template<class POST>
void YuSession::AsyncHttpAction::setCommonLoginParamsTpl(POST& form, const char* game_id)
{
  unsigned appId = session.getAppId();

  form["game"] = (game_id && *game_id) ? game_id : session.getGameId();

  if (appId && appId != -1)
    form["gapp_id"] = YuString(YuString::CtorSprintf(), "%u", session.getAppId());

  if (session.getDistr().length())
    form["distr"] = session.getDistr();

  if (session.doGetMeta())
    form["meta"] = "1";
}


//==================================================================================================
void YuSession::AsyncHttpAction::setCommonLoginParams(YuStrMap& form, const char* game_id)
{
  setCommonLoginParamsTpl(form, game_id);
}


//==================================================================================================
void YuSession::AsyncHttpAction::setCommonLoginParams(YuFormData& form, const char* game_id)
{
  setCommonLoginParamsTpl(form, game_id);
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendPost(const YuString& url, const YuStrMap& req)
{
  YuHttpHandle http = doPost(url, req);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendPost(const YuString& url, const YuFormData& req)
{
  YuHttpHandle http = doPost(url, req);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendPost(const YuString& url, const YuStrMap& req,
                                          const YuStrMap& headers)
{
  YuHttpHandle http = doPost(url, req, headers);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendPost(const YuString& url, const YuCharTab& body)
{
  YuHttpHandle http = doPost(url, body);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendPost(const YuString& url, const YuStrMap& params,
                                          unsigned conn_ms, unsigned req_ms)
{
  YuHttpHandle http = doPost(url, params, conn_ms, req_ms);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendGet(const YuString& url, const YuStrMap& params)
{
  YuHttpHandle http = doGet(url, params);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
bool YuSession::AsyncHttpAction::sendGet(const YuString& url, const YuStrMap& params,
                                         unsigned conn_ms, unsigned req_ms)
{
  YuHttpHandle http = doGet(url, params, conn_ms, req_ms);
  setHttpAct(http);

  if (!http)
    doneEvt.set();

  return http;
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doPost(const YuString& url, const YuStrMap& params)
{
  return YuHttp::self().post(url, params, this);
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doPost(const YuString& url, const YuStrMap& params,
                                                const YuStrMap& headers)
{
  return YuHttp::self().post(url, params, headers, this);
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doPost(const YuString& url, const YuFormData& params)
{
  return YuHttp::self().post(url, params, this);
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doPost(const YuString& url, const YuCharTab& body)
{
  return YuHttp::self().post(url, body, this);
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doPost(const YuString& url, const YuStrMap& params,
                                                unsigned conn_ms, unsigned req_ms)
{
  return YuHttp::self().postTimeout(url, params, conn_ms, req_ms, this);
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doGet(const YuString& url, const YuStrMap& params)
{
  return YuHttp::self().get(url, params, this);
}


//==================================================================================================
YuHttpHandle YuSession::AsyncHttpAction::doGet(const YuString& url, const YuStrMap& params,
                                               unsigned conn_ms, unsigned req_ms)
{
  return YuHttp::self().getTimeout(url, params, conn_ms, req_ms, this);
}


//==================================================================================================
void YuSession::AsyncHttpAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  done(parseServerJson(data));
}


//==================================================================================================
void YuSession::AsyncHttpAction::onHttpError(const YuString& url, int err_no)
{
  done(curlError(err_no));
}


//==================================================================================================
Yuplay2Status YuSession::AsyncHttpAction::parseServerJson(const YuCharTab& answer_bytes)
{
  answer.clear();

  if (answer.decode(answer_bytes))
    return checkJsonAnswer(answer.at("status").s());

  return YU2_FAIL;
}
