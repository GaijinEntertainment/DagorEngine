#ifndef __YUPLAY2_SESSION_PROC__
#define __YUPLAY2_SESSION_PROC__
#pragma once


#include "yu_sessionFallback.h"
#include "yu_path.h"
#include "yu_url.h"


class YuFriends;
class YupmasterGetFile;


class YuSession::WebLoginSessionIdAction : public YuSession::AsyncHttpAction
{
public:
  WebLoginSessionIdAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_WEB_LOGIN_SESSION_ID, cb) {}

  bool run();

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::GetTwoStepCodeAction : public YuSession::AsyncHttpAction
{
public:
  GetTwoStepCodeAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_GET_TWO_STEP_CODE, cb) {}

  bool run();

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::XboxLinkAction: public YuSession::AsyncHttpAction
{
public:
  XboxLinkAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_XBOX_LINK, cb) {}

  bool run(const YuString& email, const YuString& key, const YuString& sig);
};


class YuSession::BasicUserinfoAction : public YuSession::AsyncAction
{
public:
  BasicUserinfoAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncAction(s, id,
      id == ACTION_BASIC_USER_INFO ? YUPLAY2_BASIC_USER_INFO : YUPLAY2_USER_INFO, cb) {}

  bool run(const YuString& user_id, const YuString& login, const YuString& nick,
           const YuString& token, const YuString& tags, const YuString& nickorig,
           const YuString& nicksfx, const YuStrMap& meta);

private:
  virtual void stop() {}
};


class YuSession::PurchCountAction : public YuSession::AsyncHttpAction
{
public:
  PurchCountAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_PURCHASES_COUNT, cb) {}

  bool run(const YuString& token, const YuString& guid);

private:
  YuString guid;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& answer, int& count);
};


class YuSession::MultiPurchCountAction : public YuSession::AsyncHttpAction
{
public:
  MultiPurchCountAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_MULTI_PURCHASES_COUNT, cb) {}

  bool run(const YuString& token, const YuStringSet& guids);

private:
  YuStringSet guids;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& answer, IYuplay2ItemPurchases*& purch);
};


class YuSession::PurchaseAction : public YuSession::FallbackAction
{
public:
  PurchaseAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_PURCHASE, cb, s.getGaijinNetFailbackIp(), s.doGaijinNetFailback()) {}

  bool run(const YuString& token, const YuString& guid, Yuplay2PaymentMethods payment);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::ActivateAction : public YuSession::FallbackAction
{
public:
  ActivateAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_ACTIVATE, cb, s.getGaijinNetFailbackIp(), s.doGaijinNetFailback()) {}

  bool run(const YuString& token, const YuString& code);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::CommonVersionAction : public YuSession::FallbackAction
{
public:
  CommonVersionAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb) :
    FallbackAction(s, id, msg, cb, s.getYupmasterFallbackHosts(),
      s.getYupmasterFailbackIp(), s.doYupmasterFailback()) {}

protected:
  virtual void* onVersion(const YuString& version) = 0;

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::GetVersionAction : public YuSession::CommonVersionAction
{
public:
  GetVersionAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonVersionAction(s, id, YUPLAY2_REMOTE_VERSION, cb) {}

  bool run(const YuString& guid, const YuString& tag, unsigned conn_timeout, unsigned req_timeout);

private:
  void* onVersion(const YuString& version) override;
};


class YuSession::CheckVersionAction : public YuSession::CommonVersionAction
{
public:
  CheckVersionAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonVersionAction(s, id, YUPLAY2_VERSION, cb) {}

  bool run(const YuPath& torrent, const YuString& guid, const YuString& tag);

private:
  YuPath torrent;

  void* onVersion(const YuString& version) override;
};


class YuSession::YupmasterAction : public YuSession::AsyncHttpAction
{
public:
  YupmasterAction(YuSession& s, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, msg, cb), yupmaster(NULL) {}

protected:
  YupmasterGetFile* yupmaster = NULL;

  bool getFromYupmaster(const YuString& url, const YuStrMap& uri);

private:
  virtual void stop();
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::GetYupAction : public YuSession::YupmasterAction
{
public:
  GetYupAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    YupmasterAction(s, id, YUPLAY2_YUP_FILE, cb) {}

  bool run(const YuString& guid, const YuString& tag);
};


class YuSession::CheckNewerYupAction : public YuSession::YupmasterAction
{
public:
  CheckNewerYupAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    YupmasterAction(s, id, YUPLAY2_NEWER_YUP, cb) {}

  bool run(const YuPath& yup_path, const YuString& guid, const YuString& tag);
};


class YuSession::GetYupVersionAction : public YuSession::AsyncAction
{
public:
  GetYupVersionAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncAction(s, id, YUPLAY2_LOCAL_VERSION, cb) {}

  bool run(const YuPath& torrent);

private:
  virtual void stop() {}
};


class YuSession::RenewTokenAction : public YuSession::FallbackAction
{
public:
  RenewTokenAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_RENEW_TOKEN, cb, s.getFailbackIp(), s.doAuthFailback()) {}

  bool run(const YuString& token);

private:
  Yuplay2Status parseServerAnswer(const YuCharTab& data, YuString& token, int64_t& token_exp,
                                  YuString& tags);
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::RenewJwtAction : public YuSession::FallbackAction
{
public:
  RenewJwtAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_RENEW_JWT, cb, s.getFailbackIp(), false) {}

  bool run(const YuString& jwt);

private:
  Yuplay2Status parseServerAnswer(const YuCharTab& data, YuString& jwt);
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::ItemInfoAction : public YuSession::FallbackAction
{
public:
  ItemInfoAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_ITEM_INFO, cb, s.getGaijinNetFailbackIp(), s.doGaijinNetFailback()) {}

  bool run(const YuString& guid, const YuString& token);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2ItemInfo*& info);
};


class YuSession::MultiItemInfoAction : public YuSession::AsyncHttpAction
{
public:
  MultiItemInfoAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_MULTI_ITEM_INFO, cb) {}

  bool run(const YuStringSet& guids, const YuString& token);

private:
  YuStringSet guids;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2ItemsInfo*& info);
};


class YuSession::OpenItemPageAction : public YuSession::AsyncHttpAction
{
public:
  OpenItemPageAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_OPEN_ITEM_PAGE, cb) {}

  bool run(const YuString& guid, const YuString& token);

private:
  enum State
  {
    STATE_ITEM_INFO,
    STATE_TOKEN,
    STATE_DONE
  };

  State openPageState = STATE_ITEM_INFO;

  YuString guid;
  YuString token;
  YuString stoken;
  YuUrl itemUrl;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  virtual void onHttpError(const YuString& url, int err_no);
  virtual void onHttpEnd(const YuString& url);

  Yuplay2Status onItemInfoReceived(const YuCharTab& data);
  Yuplay2Status onUserStokenReceived(const YuCharTab& data);

  void openUrlInBrowser(const YuUrl& url);

  YuUrl getLoginUrl() const;
};


class YuSession::LoggedUrlAction : public YuSession::AsyncHttpAction
{
public:
  LoggedUrlAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_LOGGED_URL, cb) {}

  bool run(const YuString& token, const YuString& url, const YuString& base_host);

private:
  YuUrl url;
  YuString baseHost;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2String*& ret_url);
};


class YuSession::SsoLoggedUrlAction : public YuSession::AsyncHttpAction
{
public:
  SsoLoggedUrlAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_SSO_LOGGED_URL, cb) {}

  bool run(const YuString& token, const YuString& url, const YuString& base_host,
           const YuString& service_name);

private:
  YuUrl url;
  YuString baseHost;
  YuString serviceName;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2String*& ret_url);
};


class YuSession::KongZhongLoggedUrlAction : public YuSession::AsyncAction
{
public:
  KongZhongLoggedUrlAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncAction(s, id, YUPLAY2_KONGZHONG_LOGGED_URL, cb) {}

  bool run(const YuString& kz_user_id, uint64_t wegame_id, const YuString& url);

private:
  virtual void stop() {}
};


class YuSession::SsoGetShortTokenAction : public YuSession::AsyncHttpAction
{
public:
  SsoGetShortTokenAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_SSO_GET_SHORT_TOKEN, cb) {}

  bool run(const YuString& token);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2String*& ret_short_token);
};


class YuSession::GetStokenAction : public YuSession::AsyncHttpAction
{
public:
  GetStokenAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_GET_STOKEN, cb) {}

  bool run(const YuString& token, const YuString& jwt);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2String*& stoken);
};


class YuSession::GetOpenIdJwtAction : public YuSession::AsyncHttpAction
{
public:
  GetOpenIdJwtAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    AsyncHttpAction(s, id, YUPLAY2_GET_OPENID_JWT, cb) {}

  bool run(const YuString& jwt);

private:
  void onHttpResponse(const YuString& url, const YuCharTab& data) override;
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2String*& openid_jwt);
};


class YuSession::TagUserAction : public YuSession::FallbackAction
{
public:
  TagUserAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_TAG_USER, cb, s.getFailbackIp(), s.doAuthFailback()) {}

  bool run(const YuString& token, const YuStringSet& tags);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data);
};


class YuSession::TencentBuyGoldAction : public YuSession::FallbackAction
{
public:
  TencentBuyGoldAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_TENCENT_BUY_GOLD, cb, s.getFailbackIp(), s.doAuthFailback()) {}

  bool run(const YuString& token, unsigned gold);
};


class YuSession::TencentPaymentConfAction : public YuSession::FallbackAction
{
public:
  TencentPaymentConfAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_TENCENT_PAYMENT_CONF, cb, s.getFailbackIp(), s.doAuthFailback()){}

  bool run();

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::SteamLinkTokenAction : public YuSession::FallbackAction
{
public:
  SteamLinkTokenAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_STEAM_LINK_TOKEN, cb, s.getFailbackIp(), s.doAuthFailback()) {}

  bool run(const YuString& token, const YuMemRange<char>& code, int app_id);

private:
  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  Yuplay2Status parseServerAnswer(const YuCharTab& data, IYuplay2String*& token);
};


class YuSession::GooglePurchaseAction : public YuSession::FallbackAction
{
public:
  GooglePurchaseAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_GOOGLE_PURCHASE, cb, s.getGaijinNetFailbackIp(), s.doGaijinNetFailback()) {}

  bool run(const YuString& token, const YuString& purch_json, bool do_acknowledge);

private:
  YuString itemId;
  YuString purchToken;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::HuaweiPurchaseAction : public YuSession::FallbackAction
{
public:
  HuaweiPurchaseAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_HUAWEI_PURCHASE, cb, s.getGaijinNetFailbackIp(), s.doGaijinNetFailback()) {}

  bool run(const YuString& token, const YuString& purch_json, bool do_acknowledge);

private:
  YuString itemId;
  YuString purchToken;

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


class YuSession::ApplePurchaseAction : public YuSession::FallbackAction
{
public:
  ApplePurchaseAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    FallbackAction(s, id, YUPLAY2_APPLE_PURCHASE, cb, s.getGaijinNetFailbackIp(), s.doGaijinNetFailback()) {}

  bool run(const YuString& token, const YuString& purch_json);

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
};


#endif //__YUPLAY2_SESSION_PROC__
