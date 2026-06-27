#ifndef __YUPLAY2_SESSION_LOGIN__
#define __YUPLAY2_SESSION_LOGIN__
#pragma once


#include "yu_sessionLoginBase.h"


class YuSession::LoginAction : public YuSession::CommonLoginAction
{
public:
  LoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_CHECK_LOGIN, cb, true, true) {}

  Yuplay2Status parseServerAnswer(const YuCharTab& data) override;
  bool run(const YuString& login, const YuString& pass, bool auth_client, const YuString& game_id);

private:
  virtual void onOkStatus(const YuJson& json);
  virtual Yuplay2Status onErrorStatus(Yuplay2Status status);
};


class YuSession::TwoStepLoginAction : public YuSession::CommonLoginAction
{
public:
  TwoStepLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_2STEP_LOGIN, cb, true, true) {}

  bool run(const YuString& login, const YuString& pass, const YuString& code, bool auth_client);
};


class YuSession::TokenLoginAction : public YuSession::CommonLoginAction
{
public:
  TokenLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_CHECK_TOKEN, cb) {}

  bool run(const YuString& token);

private:
  virtual Yuplay2Status onErrorStatus(Yuplay2Status status);
};


class YuSession::StokenLoginAction : public YuSession::CommonLoginAction
{
public:
  StokenLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_GET_STOKEN, cb, false) {}

  bool run(const YuString& token);
};


class YuSession::SsoStokenLoginAction : public YuSession::CommonLoginAction
{
public:
  SsoStokenLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_SSO_STOKEN_LOGIN, cb, false) {}

  bool run(const YuString& token);
};


class YuSession::NSwitchLoginAction: public YuSession::CommonLoginAction
{
public:
  NSwitchLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_NSWITCH_LOGIN, cb) {}

  bool run(const YuString& nintendo_jwt, const YuString& game_id,
           const YuString& nintendo_nick,const YuString& lang);
private:
  YuStrMap headers;

  virtual void onOkStatus(const YuJson& json);

  virtual const YuStrMap* getRequestHeaders() const { return &headers; }
};


class YuSession::PsnLoginAction : public YuSession::CommonLoginAction
{
public:
  PsnLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_PSN_LOGIN, cb) {}

  bool run(const YuString& code, const YuString& game_id, int issuer, const YuString& lang,
           unsigned conn_timeout, unsigned request_timeout, const YuString& title_id);
private:
  virtual void onOkStatus(const YuJson& json);
};


class YuSession::SteamLoginAction : public YuSession::CommonLoginAction
{
public:
  SteamLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_STEAM_LOGIN, cb) {}

  bool run(const YuString& token, const YuMemRange<char>& code, int app_id,
           const YuString& steam_nick, const YuString& lang, bool only_known);
};


class YuSession::TencentLoginAction : public YuSession::CommonLoginAction
{
public:
  TencentLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_TENCENT_LOGIN, cb, false) {}

  bool run();
};


class YuSession::DmmLoginAction : public YuSession::CommonLoginAction
{
public:
  DmmLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_DMM_LOGIN, cb) {}

  bool run(const YuString& user_id, const YuString& dmm_token,
           const YuString& app_id, const YuString& game_id);
};


class YuSession::XboxLoginAction: public YuSession::CommonLoginAction
{
public:
  XboxLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_XBOX_LOGIN, cb, false) {}

  bool run(const YuString& key, const YuString& sig);

private:
  virtual void onOkStatus(const YuJson& json);
};


class YuSession::EpicLoginAction : public YuSession::CommonLoginAction
{
public:
  EpicLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_EPIC_LOGIN, cb) {}

  bool run(const YuString& epic_token, const YuString& game_id);
};


class YuSession::AppleLoginAction : public YuSession::CommonLoginAction
{
public:
  AppleLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_APPLE_LOGIN, cb) {}

  bool run(const YuString& id_token);
};


class YuSession::GooglePlayLoginAction : public YuSession::CommonLoginAction
{
public:
  GooglePlayLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_GOOGLE_PLAY_LOGIN, cb) {}

  bool run(const YuString& code, const YuString& firebase_id);
};


class YuSession::FacebookLoginAction : public YuSession::CommonLoginAction
{
public:
  FacebookLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_FACEBOOK_LOGIN, cb) {}

  bool run(const YuString& access_token, bool limited);
};


class YuSession::FirebaseLoginAction : public YuSession::CommonLoginAction
{
public:
  FirebaseLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_FIREBASE_LOGIN, cb) {}

  bool run(const YuString& firebase_id, const YuString& secret, bool only_known);
};


class YuSession::GuestLoginAction : public YuSession::CommonLoginAction
{
public:
  GuestLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_GUEST_LOGIN, cb) {}

  bool run(const YuString& system_id, const YuString& secret, const YuString& nick,bool only_known);
};


class YuSession::SamsungLoginAction : public YuSession::CommonLoginAction
{
public:
  SamsungLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_SAMSUNG_LOGIN, cb) {}

  bool run(const YuString& samsung_token);
};


class YuSession::ExternalLoginAction : public YuSession::CommonLoginAction
{
public:
  ExternalLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_EXTERNAL_LOGIN, cb) {}

  bool run(const YuString& external_jwt);

private:
  YuStrMap headers;

  virtual const YuStrMap* getRequestHeaders() const { return &headers; }
};


class YuSession::HuaweiLoginAction : public YuSession::CommonLoginAction
{
public:
  HuaweiLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_HUAWEI_LOGIN, cb) {}

  bool run(const YuString& huawei_jwt);
};


class YuSession::NvidiaLoginAction : public YuSession::CommonLoginAction
{
public:
  NvidiaLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_NVIDIA_LOGIN, cb) {}

  bool run(const YuString& nvidia_jwt);

private:
#if YU2_WINDOWS
  bool getStatusJsonPath(YuPath& path) const;
  void saveStatusJson(const YuString& status, const YuString& user_id, const YuString& nick,
                      const YuString& error) const;

  Yuplay2Status onErrorStatus(Yuplay2Status status) override;
  void onOkStatus(const YuJson& json) override;
#endif //YU2_WINDOWS
};


class YuSession::VkLoginAction : public YuSession::CommonLoginAction
{
public:
  VkLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_VK_LOGIN, cb) {}

  bool run(const YuString& vk_token, const YuString& client_id);
};


class YuSession::WebLoginAction : public YuSession::CommonLoginAction
{
public:
  WebLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_WEB_LOGIN, cb, false) {}

  bool run(const YuString& session_id);

private:
  YuStrMap post;
  bool repeatingRequest = false;
  YuEvent stopEvt;
  int64_t startTime = 0;

  void repeatRequest(const YuString& url);

  virtual void stop(); //To stop delay wait in onHttpResponse() with "DELAY" status

  virtual void onHttpResponse(const YuString& url, const YuCharTab& data);
  virtual void onHttpError(const YuString& url, int err_no);
  virtual void onHttpEnd(const YuString& url);
};


class YuSession::KongZhongLoginAction : public YuSession::CommonLoginAction
{
public:
  KongZhongLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_KONGZHONG_LOGIN, cb, false) {}

  bool run(const YuString& kz_token);

private:
  virtual void onOkStatus(const YuJson& json);
  virtual Yuplay2Status onErrorStatus(Yuplay2Status status);
};


class YuSession::WegameLoginAction : public YuSession::CommonLoginAction
{
public:
  WegameLoginAction(YuSession& s, ActionId id, IYuplay2Cb* cb) :
    CommonLoginAction(s, id, YUPLAY2_WEGAME_LOGIN, cb, false) {}

  bool run(uint64_t wegame_id, const YuString& ticket, const YuString& nick);

private:
  uint64_t wegameId = 0;

  virtual void onOkStatus(const YuJson& json);
  virtual Yuplay2Status onErrorStatus(Yuplay2Status status);
};


#endif //__YUPLAY2_SESSION_LOGIN__
