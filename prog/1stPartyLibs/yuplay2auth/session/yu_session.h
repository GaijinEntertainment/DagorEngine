#ifndef __YUPLAY2_SESSION_IMPL__
#define __YUPLAY2_SESSION_IMPL__
#pragma once


#include <yuplay2_session.h>

#include "yu_threadPool.h"
#include "yu_containers.h"
#include "yu_url.h"

#include <EASTL/bitset.h>


class YuPath;


class YuSession :
    public IYuplay2Session,
    public IYuplay2UserProc,
    public IYuplay2ItemProc
{
public:
  YuSession(const char* client_name, const char* game, unsigned app_id, const char* distr);

  IYuplay2Session* createFromMe() const;
  IYuplay2Session* createFromMe(unsigned new_app_id) const;

private:
  class AsyncAction;
  class AsyncHttpAction;
  class LoginAction;
  class TokenLoginAction;
  class PsnLoginAction;
  class NSwitchLoginAction;
  class TwoStepLoginAction;
  class BasicUserinfoAction;
  class PurchCountAction;
  class CheckVersionAction;
  class GetYupAction;
  class GetYupVersionAction;
  class PurchaseAction;
  class RenewTokenAction;
  class RenewJwtAction;
  class ItemInfoAction;
  class OpenItemPageAction;
  class MultiPurchCountAction;
  class MultiItemInfoAction;
  class ActivateAction;
  class LoggedUrlAction;
  class SsoLoggedUrlAction;
  class SsoGetShortTokenAction;
  class GetStokenAction;
  class StokenLoginAction;
  class TagUserAction;
  class TencentLoginAction;
  class GetVersionAction;
  class CommonVersionAction;
  class SteamLoginAction;
  class FallbackAction;
  class CommonLoginAction;
  class TencentBuyGoldAction;
  class TencentPaymentConfAction;
  class SteamLinkTokenAction;
  class DmmLoginAction;
  class CheckNewerYupAction;
  class YupmasterAction;
  class GetTwoStepCodeAction;
  class SsoStokenLoginAction;
  class XboxLoginAction;
  class XboxLinkAction;
  class EpicLoginAction;
  class WebLoginSessionIdAction;
  class WebLoginAction;
  class KongZhongLoginAction;
  class AppleLoginAction;
  class KongZhongLoggedUrlAction;
  class GooglePlayLoginAction;
  class WegameLoginAction;
  class FirebaseLoginAction;
  class GuestLoginAction;
  class GooglePurchaseAction;
  class FacebookLoginAction;
  class ApplePurchaseAction;
  class SamsungLoginAction;
  class GetOpenIdJwtAction;
  class ExternalLoginAction;
  class HuaweiLoginAction;
  class HuaweiPurchaseAction;
  class NvidiaLoginAction;

  enum ActionId
  {
    ACTION_LOGIN,
    ACTION_USER_INFO,
    ACTION_PURCH_CNT,
    ACTION_CHECK_VERSION,
    ACTION_GET_YUP,
    ACTION_GET_YUP_VERSION,
    ACTION_BUY_ITEM,
    ACTION_RENEW_TOKEN,
    ACTION_ITEM_INFO,
    ACTION_MULTI_ITEM_INFO,
    ACTION_MULTI_PURCH_CNT,
    ACTION_BASIC_USER_INFO,
    ACTION_OPEN_ITEM_PAGE,
    ACTION_TOKEN_LOGIN,
    ACTION_ACTIVATE,
    ACTION_LOGGED_URL,
    ACTION_PSN_LOGIN,
    ACTION_GET_STOKEN,
    ACTION_STOKEN_LOGIN,
    ACTION_2STEP_LOGIN,
    ACTION_TAG_USER,
    ACTION_TENCENT_LOGIN,
    ACTION_GET_REMOTE_VERSION,
    ACTION_STEAM_LOGIN,
    ACTION_TENCENT_BUY_GOLD,
    ACTION_TENCENT_PAYMENT_CONF,
    ACTION_STEAM_LINK_TOKEN,
    ACTION_DMM_LOGIN,
    ACTION_CHECK_NEWER_YUP,
    ACTION_GET_TWO_STEP_CODE,
    ACTION_SSO_STOKEN_LOGIN,
    ACTION_XBOX_LOGIN,
    ACTION_XBOX_LINK,
    ACTION_SSO_LOGGED_URL,
    ACTION_SSO_MAKE_LOGGED_URL,
    ACTION_NSWITCH_LOGIN,
    ACTION_SSO_GET_SHORT_TOKEN,
    ACTION_EPIC_LOGIN,
    ACTION_WEB_LOGIN_SESSION_ID,
    ACTION_WEB_LOGIN,
    ACTION_WEB_LOGIN_BROWSER,
    ACTION_KONGZHONG_LOGIN,
    ACTION_RENEW_JWT,
    ACTION_APPLE_LOGIN,
    ACTION_KONGZHONG_LOGGED_URL,
    ACTION_GOOGLE_PLAY_LOGIN,
    ACTION_WEGAME_LOGIN,
    ACTION_FIREBASE_LOGIN,
    ACTION_GUEST_LOGIN,
    ACTION_GOOGLE_PURCHASE,
    ACTION_FACEBOOK_LOGIN,
    ACTION_APPLE_PURCHASE,
    ACTION_SAMSUNG_LOGIN,
    ACTION_GET_OPENID_JWT,
    ACTION_EXTERNAL_LOGIN,
    ACTION_HUAWEI_LOGIN,
    ACTION_HUAWEI_PURCHASE,
    ACTION_NVIDIA_LOGIN,

    __ACTION_LAST
  };

  YuCriticalSection sessionLock;
  AsyncAction* currentActions[__ACTION_LAST];

  struct ActionCall
  {
    bool async;
    ActionId id;

    int* status;
    IYuplay2Cb* cb;
    Yuplay2Handle* handle;
  };

  typedef eastl::bitset<__ACTION_LAST>  ApiHosts;

  YuString cliName;
  YuString distrId;
  YuString gameId;
  unsigned appId = 0;

  YuString yuLogin;
  YuString yuNick;
  YuString yuNickorig;
  YuString yuNicksfx;
  YuString userId;
  YuString yuToken;
  YuString yuTags;
  YuString yuJwt;
  YuString yuLang;
  YuString yuCountryCode;
  int64_t tokenExpTime;
  int64_t tokenLifeTime;
  YuString firebaseSecret;
  YuString guestSecret;
  YuStrMap meta;

  YuUrl apiUrls[__ACTION_LAST];
  ApiHosts authUrls;
  ApiHosts ssoUrls;
  ApiHosts gjApiUrls;
  ApiHosts yupmasterUrls;

  YuString tencentAppData;
  YuString failbackIp;
  YuString yupmasterFailbackIp;
  YuString gaijinNetApiIp;

  YuStringTab yupmasterFallbackHosts;

  YuString twoStepRequestId;
  YuString twoStepUserId;
  bool hasGaijinPassTwoStepType;
  bool hasEmailTwoStepType;
  bool hasWTAssistantTwoStepType;

  unsigned stateBits;
  unsigned tencentZoneId;

  float tencentGoldRate;

  unsigned psnConnectTimeout;
  unsigned psnRequestTimeout;
  unsigned versionConnTimeout;
  unsigned versionReqTimeout;
  unsigned webLoginStatusTimeout;

  bool useAuthFailbackIp;
  bool useYupmasterFailbackIp;
  bool useGaijinNetFailbackIp;
  bool getMeta = false;

  int nintendoErrCode = 0;
  YuString nintendoErrText;

  bool psnPlusOn = false;
  bool gamePassOn = false;

  int kzFcm = -1;
  YuString kzMsg;
  YuString kzChannelCode;
  uint64_t wegameId;

  const YuString& getLoginUrl() const { return apiUrls[ACTION_LOGIN].str(); }
  const YuString& getTokenUrl() const { return apiUrls[ACTION_TOKEN_LOGIN].str(); }
  const YuString& getJwtUrl() const { return apiUrls[ACTION_RENEW_JWT].str(); }
  const YuString& getPsnLoginUrl() const { return apiUrls[ACTION_PSN_LOGIN].str(); }
  const YuString& getVersionUrl() const { return apiUrls[ACTION_GET_REMOTE_VERSION].str(); }
  const YuString& getYupUrl() const { return apiUrls[ACTION_GET_YUP].str(); }
  const YuString& getUserStokenUrl() const { return apiUrls[ACTION_GET_STOKEN].str(); }
  const YuString& getSsoUserStokenUrl() const { return apiUrls[ACTION_SSO_LOGGED_URL].str(); }
  const YuString& getSsoLoggedUrl() const { return apiUrls[ACTION_SSO_MAKE_LOGGED_URL].str(); }
  const YuString& getStokenLoginUrl() const { return apiUrls[ACTION_STOKEN_LOGIN].str(); }
  const YuString& getTagUserUrl() const { return apiUrls[ACTION_TAG_USER].str(); }
  const YuString& getSteamLoginUrl() const { return apiUrls[ACTION_STEAM_LOGIN].str(); }
  const YuString& getSteamLinkUrl() const { return apiUrls[ACTION_STEAM_LINK_TOKEN].str(); }
  const YuString& getTencentGoldUrl() const { return apiUrls[ACTION_TENCENT_BUY_GOLD].str(); }
  const YuString& getTencentPayConfUrl() const { return apiUrls[ACTION_TENCENT_PAYMENT_CONF].str(); }
  const YuString& getDmmLoginUrl() const { return apiUrls[ACTION_DMM_LOGIN].str(); }
  const YuString& getNewerYupUrl() const { return apiUrls[ACTION_CHECK_NEWER_YUP].str(); }
  const YuString& getTwoStepUrl() const { return apiUrls[ACTION_GET_TWO_STEP_CODE].str(); }
  const YuString& getSsoStokenLoginUrl() const { return apiUrls[ACTION_SSO_STOKEN_LOGIN].str(); }
  const YuString& getXboxLoginUrl() const { return apiUrls[ACTION_XBOX_LOGIN].str(); }
  const YuString& getXboxLinkUrl() const { return apiUrls[ACTION_XBOX_LINK].str(); }
  const YuString& getNswitchLoginUrl() const { return apiUrls[ACTION_NSWITCH_LOGIN].str(); }
  const YuString& getEpicLoginUrl() const { return apiUrls[ACTION_EPIC_LOGIN].str(); }
  const YuString& getAppleLoginUrl() const { return apiUrls[ACTION_APPLE_LOGIN].str(); }
  const YuString& getGooglePlayLoginUrl() const { return apiUrls[ACTION_GOOGLE_PLAY_LOGIN].str(); }
  const YuString& getWebLoginSessionIdUrl() const { return apiUrls[ACTION_WEB_LOGIN_SESSION_ID].str(); }
  const YuString& getWebLoginUrl() const { return apiUrls[ACTION_WEB_LOGIN].str(); }
  const YuString& getWegameLoginUrl() const { return apiUrls[ACTION_WEGAME_LOGIN].str(); }
  const YuString& getFacebookLoginUrl() const { return apiUrls[ACTION_FACEBOOK_LOGIN].str(); }
  const YuString& getFirebaseLoginUrl() const { return apiUrls[ACTION_FIREBASE_LOGIN].str(); }
  const YuString& getGuestLoginUrl() const { return apiUrls[ACTION_GUEST_LOGIN].str(); }
  const YuString& getGooglePurchaseUrl() const { return apiUrls[ACTION_GOOGLE_PURCHASE].str(); }
  const YuString& getHuaweiPurchaseUrl() const { return apiUrls[ACTION_HUAWEI_PURCHASE].str(); }
  const YuString& getApplePurchaseUrl() const { return apiUrls[ACTION_APPLE_PURCHASE].str(); }
  const YuString& getSamsungLoginUrl() const { return apiUrls[ACTION_SAMSUNG_LOGIN].str(); }
  const YuString& getOpenIdJwtUrl() const { return apiUrls[ACTION_GET_OPENID_JWT].str(); }
  const YuString& getExternalLoginUrl() const { return apiUrls[ACTION_EXTERNAL_LOGIN].str(); }
  const YuString& getHuaweiLoginUrl() const { return apiUrls[ACTION_HUAWEI_LOGIN].str(); }
  const YuString& getNvidiaLoginUrl() const { return apiUrls[ACTION_NVIDIA_LOGIN].str(); }
  const YuString& getWebLoginBrowserUrl() const { return apiUrls[ACTION_WEB_LOGIN_BROWSER].str(); }
  const YuString& getItemInfoUrl() const { return apiUrls[ACTION_ITEM_INFO].str(); }
  const YuString& getBuyItemUrl() const { return apiUrls[ACTION_BUY_ITEM].str(); }
  const YuString& getActivateItemUrl() const { return apiUrls[ACTION_ACTIVATE].str(); }

  const YuString& getFailbackIp() const { return failbackIp; }
  const YuString& getYupmasterFailbackIp() const { return yupmasterFailbackIp; }
  const YuString& getGaijinNetFailbackIp() const { return gaijinNetApiIp; }

  const YuStringTab& getYupmasterFallbackHosts() const { return yupmasterFallbackHosts; }

  const YuString& getTwoStepRequestId() const { return twoStepRequestId; }
  const YuString& getTwoStepUserId() const { return twoStepUserId; }

  unsigned getAppId() const { return appId; }
  const YuString& getDistr() const { return distrId; }
  const YuString& getGameId() const { return gameId; }
  bool doGetMeta() const { return getMeta; }

  void updateApiHost(const ApiHosts& hosts, const char* host);

  const YuString& getKzChannelCode() const { return kzChannelCode; }

  virtual bool YU2VCALL isHasTwoStepRequest() const
    { return twoStepRequestId.length() && twoStepUserId.length(); }
  virtual bool YU2VCALL isHasGaijinPassTwoStepTypeSync() const { return hasGaijinPassTwoStepType; }
  virtual bool YU2VCALL isHasEmailTwoStepTypeSync() const { return hasEmailTwoStepType; }
  virtual bool YU2VCALL isHasWTAssistantTwoStepTypeSync() const {return hasWTAssistantTwoStepType;}

  bool doAuthFailback() const { return useAuthFailbackIp; }
  bool doYupmasterFailback() const { return useYupmasterFailbackIp; }
  bool doGaijinNetFailback() const { return useGaijinNetFailbackIp; }

  void gcActions();

  void onLogin(const YuString& login, const YuString& nick, const YuString& user_id,
               const YuString& token, const YuString& jwt, const YuString& tags,
               const YuString& lang, const YuString& country, int64_t exp_time,
               const YuString& nickorig, const YuString& nicksfx, const YuStrMap& meta);
  void onNewToken(const YuString& token, int64_t exp_time);
  void onNewJwt(const YuString& jwt);
  void onNewTags(const YuString& tags);
  void onTencentZoneId(unsigned zone_id);
  void onTencentAppData(const YuString &data);
  void onTencentPaymentConf(float gold_rate);
  void onTwoStepResponse(const YuString& req_id, const YuString& user_id,
                         bool gp_totp, bool email_totp, bool wt_totp);
  void onNswitchLogin(int eshop_code, const YuString& eshop_msg);
  void onPsnLogin(bool psn_plus);
  void onXboxLogin(bool game_pass);
  void onKongZhongLogin(int fcm);
  void onKongZhongLoginFail(const YuString& msg);
  void onWegameLogin(uint64_t wegame_id);

  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID>
  Yuplay2Status createAction(Yuplay2AsyncCall* call, ACTION*& action);

  template<class ACTION, ActionId ACT_ID>
  Yuplay2Status doSyncAction(ACTION* action, const void** result = NULL);

  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID>
  Yuplay2Status doAction(Yuplay2AsyncCall* call, const void** result);

  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2, class V3>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2, const V3& v3,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2, class V3, class V4>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2, class V3, class V4, class V5>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2, const V3& v3, const V4& v4, const V5& v5,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2, class V3, class V4, class V5, class V6>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2, const V3& v3, const V4& v4, const V5& v5, const V6& v6,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2, class V3, class V4, class V5, class V6, class V7>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2, const V3& v3, const V4& v4, const V5& v5, const V6& v6, const V7& v7,
                         const void** result);
  template<bool NEED_LOGIN, class ACTION, ActionId ACT_ID, class V1, class V2, class V3, class V4, class V5, class V6, class V7, class V8>
  Yuplay2Status doAction(Yuplay2AsyncCall* call,
                         const V1& v1, const V2& v2, const V3& v3, const V4& v4, const V5& v5, const V6& v6, const V7& v7, const V8& v8,
                         const void** result);

  void stopAction(ActionId id);

  bool getDescFilePath(const YuString& folder, const YuString& guid, YuPath& path) const;


  //IYuplay2Session
  virtual void YU2VCALL free();

  virtual IYuplay2UserProc* YU2VCALL user() { return (IYuplay2UserProc*)this; }
  virtual IYuplay2ItemProc* YU2VCALL item() { return (IYuplay2ItemProc*)this; }
  virtual IYuplay2Answer* YU2VCALL answer(Yuplay2Handle req);

  virtual unsigned YU2VCALL getStateSync();
  virtual unsigned YU2VCALL getTencentZoneIdSync();
  virtual IYuplay2String* YU2VCALL getTencentAppDataSync();
#if YU2EASTL_STRING
  virtual void YU2VCALL getTencentAppDataSync(eastl::string& data);
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL getSteamLinkTokenSync(const void* auth_code,
                                                       unsigned auth_code_len,
                                                       int steam_app_id, IYuplay2String** token);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSteamLinkTokenSync(const void* auth_code,
                                                       unsigned auth_code_len,
                                                       int steam_app_id,
                                                       eastl::string& token);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSteamLinkTokenAsync(const void* auth_code,
                                                        unsigned auth_code_len,
                                                        int steam_app_id, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL linkXboxSync(const char* email, const char* key, const char* sig);
  virtual Yuplay2Status YU2VCALL linkXboxAsync(const char* email, const char* key,
                                                const char* sig, Yuplay2AsyncCall* call);
  virtual IYuplay2String* YU2VCALL getXboxLinkUrlSync() const;
#if YU2EASTL_STRING
  virtual void YU2VCALL getXboxLinkUrlSync(eastl::string& url) const;
#endif //YU2EASTL_STRING

  virtual IYuplay2String* YU2VCALL getUserTokenSync();
#if YU2EASTL_STRING
  virtual void YU2VCALL getUserTokenSync(eastl::string& token);
#endif //YU2EASTL_STRING

  virtual IYuplay2String* YU2VCALL getUserJwtSync();
#if YU2EASTL_STRING
  virtual void YU2VCALL getUserJwtSync(eastl::string& jwt);
#endif //YU2EASTL_STRING

  virtual IYuplay2String* YU2VCALL getCountryCodeSync();
#if YU2EASTL_STRING
  virtual void YU2VCALL getCountryCodeSync(eastl::string& code);
#endif //YU2EASTL_STRING

  virtual int64_t YU2VCALL getUserTokenExpirationTime();
  virtual int64_t YU2VCALL getUserTokenLifeTime();

  virtual Yuplay2Status YU2VCALL renewUserTokenSync();
  virtual Yuplay2Status YU2VCALL renewUserTokenAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL renewUserJwtSync();
  virtual Yuplay2Status YU2VCALL renewUserJwtAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getLoggedUrlSync(const char* url, IYuplay2String** logged_url,
                                                  const char* host = NULL);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getLoggedUrlSync(const char* url, eastl::string& logged_url,
                                                  const char* base_host = NULL);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getLoggedUrlAsync(const char* url, Yuplay2AsyncCall* call,
                                                   const char* host = NULL);

  virtual Yuplay2Status YU2VCALL getSsoLoggedUrlSync(const char* url, IYuplay2String** logged_url,
                                                     const char* host = NULL, const char* service = NULL);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoLoggedUrlSync(const char* url, eastl::string& logged_url,
                                                     const char* base_host = NULL,
                                                     const char* service = NULL);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoLoggedUrlAsync(const char* url, Yuplay2AsyncCall* call,
                                                      const char* host = NULL, const char* service = NULL);

  virtual Yuplay2Status YU2VCALL getKongZhongLoggedUrlSync(const char* url,
                                                           IYuplay2String** logged_url);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getKongZhongLoggedUrlSync(const char* url,
                                                           eastl::string& logged_url);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getKongZhongLoggedUrlAsync(const char* url,
                                                            Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getSsoShortTokenSync(IYuplay2String** ret_short_token);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoShortTokenSync(eastl::string& short_token);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoShortTokenAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getUserStokenSync(IYuplay2String** stoken);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getUserStokenSync(eastl::string& stoken);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getUserStokenAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getOpenIdJwtSync(IYuplay2String** openid_jwt);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getOpenIdJwtSync(eastl::string& openid_jwt);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getOpenIdJwtAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL setOptionIntSync(Yuplay2SessionOptions id, int val);
  virtual Yuplay2Status YU2VCALL setOptionStringSync(Yuplay2SessionOptions id, const char* val);
  virtual Yuplay2Status YU2VCALL setOptionStringsSync(Yuplay2SessionOptions id, const char** vals,
                                                      unsigned count);

  virtual Yuplay2Status YU2VCALL loginSync(const char* login, const char* password,
                                           bool auth_client, const char* game_id);
  virtual Yuplay2Status YU2VCALL loginAsync(const char* login, const char* password,
                                            bool auth_client, Yuplay2AsyncCall* call,
                                            const char* game_id);

  virtual Yuplay2Status YU2VCALL getTwoStepCodeAsync(Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL getTwoStepCodeSync(IYuplay2String** code);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getTwoStepCodeSync(eastl::string& code);
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL twoStepLoginSync(const char* login, const char* password,
                                                  const char* code, bool auth_client);
  virtual Yuplay2Status YU2VCALL twoStepLoginAsync(const char* login, const char* password,
                                                   const char* code, bool auth_client,
                                                   Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL psnLoginSync(const char* auth_code, const char* game_id,
                                              int issuer, const char* lang,
                                              const char* title_id = NULL);
  virtual Yuplay2Status YU2VCALL psnLoginAsync(const char* auth_code, const char* game_id,
                                               int issuer, const char* lang,
                                               Yuplay2AsyncCall* call,
                                               const char* title_id = NULL);

  virtual Yuplay2Status YU2VCALL steamLoginSync(bool link_with_auth_token, const void* auth_code,
                                                unsigned auth_code_len, int steam_app_id,
                                                const char* steam_nick, const char* lang,
                                                bool only_known);
  virtual Yuplay2Status YU2VCALL steamLoginAsync(bool link_with_auth_token, const void* auth_code,
                                                 unsigned auth_code_len, int steam_app_id,
                                                 const char* steam_nick, const char* lang,
                                                 bool only_known, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL dmmLoginSync(bool link_with_auth_token, const char* dmm_user_id,
                                              const char* dmm_token, const char* dmm_app_id,
                                              const char* game_id);
  virtual Yuplay2Status YU2VCALL dmmLoginAsync(bool link_with_auth_token, const char* dmm_user_id,
                                              const char* dmm_token, const char* dmm_app_id,
                                              const char* game_id, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL xboxLoginSync(const char* key, const char* sig);
  virtual Yuplay2Status YU2VCALL xboxLoginAsync(const char* key, const char* sig,
                                                Yuplay2AsyncCall* call);
  virtual IYuplay2String* YU2VCALL getXboxLoginUrlSync() const;
#if YU2EASTL_STRING
  virtual void YU2VCALL getXboxLoginUrlSync(eastl::string& url) const;
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL nswitchLoginSync(const char* nintendo_jwt,
                                                  const char* nintendo_nick,
                                                  const char* game_id);
  virtual Yuplay2Status YU2VCALL nswitchLoginAsync(const char* nintendo_jwt,
                                                   const char* nintendo_nick,
                                                   const char* game_id,
                                                   Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL epicLoginSync(const char* epic_token, const char* game_id);
  virtual Yuplay2Status YU2VCALL epicLoginAsync(const char* epic_token, const char* game_id,
                                                Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL appleLoginSync(const char* id_token);
  virtual Yuplay2Status YU2VCALL appleLoginAsync(const char* id_token, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL googlePlayLoginSync(const char* code, const char* firebase_id);
  virtual Yuplay2Status YU2VCALL googlePlayLoginAsync(const char* code, Yuplay2AsyncCall* call,
                                                      const char* firebase_id);

  virtual Yuplay2Status YU2VCALL facebookLoginSync(const char* access_token, bool limited);
  virtual Yuplay2Status YU2VCALL facebookLoginAsync(const char* access_token, bool limited,
                                                    Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL firebaseLoginSync(const char* firebase_id, bool only_known);
  virtual Yuplay2Status YU2VCALL firebaseLoginAsync(const char* firebase_id,
                                                    Yuplay2AsyncCall* call, bool only_known);

  virtual Yuplay2Status YU2VCALL guestLoginSync(const char* system_id, const char* nick,
                                                bool only_known);
  virtual Yuplay2Status YU2VCALL guestLoginAsync(const char* system_id, Yuplay2AsyncCall* call,
                                                 const char* nick, bool only_known);

  virtual Yuplay2Status YU2VCALL samsungLoginSync(const char* samsung_token);
  virtual Yuplay2Status YU2VCALL samsungLoginAsync(const char* samsung_token,
                                                   Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL externalLoginSync(const char* jwt);
  virtual Yuplay2Status YU2VCALL externalLoginAsync(const char* jwt, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL huaweiLoginSync(const char* jwt);
  virtual Yuplay2Status YU2VCALL huaweiLoginAsync(const char* jwt, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL nvidiaLoginSync(const char* nvidia_jwt);
  virtual Yuplay2Status YU2VCALL nvidiaLoginAsync(const char* nvidia_jwt,
                                                  Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL tokenLoginSync(const char* token);
  virtual Yuplay2Status YU2VCALL tokenLoginAsync(const char* token,
                                                 Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL stokenLoginSync(const char* token);
  virtual Yuplay2Status YU2VCALL stokenLoginAsync(const char* token,
                                                  Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL ssoStokenLoginSync(const char* token);
  virtual Yuplay2Status YU2VCALL ssoStokenLoginAsync(const char* token,
                                                     Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getWebLoginSessionIdSync(IYuplay2String** session_id);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getWebLoginSessionIdSync(eastl::string& session_id);
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getWebLoginSessionIdAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL webLoginSync(const char* session_id);
  virtual Yuplay2Status YU2VCALL webLoginAsync(const char* session_id, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL kongZhongLoginSync(const char* kz_token);
  virtual Yuplay2Status YU2VCALL kongZhongLoginAsync(const char* kz_token,
                                                     Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL tencentLoginSync();
  virtual Yuplay2Status YU2VCALL tencentLoginAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL wegameLoginSync(uint64_t wegame_id, const char* ticket,
                                                 const char* nick);
  virtual Yuplay2Status YU2VCALL wegameLoginAsync(uint64_t wegame_id, const char* ticket,
                                                  Yuplay2AsyncCall* call, const char* nick);

  virtual Yuplay2Status YU2VCALL getUserInfoSync(IYuplay2UserInfo** user_info);
  virtual Yuplay2Status YU2VCALL getUserInfoAsync(Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getBasicUserInfoSync(IYuplay2UserInfo** user_info);
  virtual Yuplay2Status YU2VCALL getBasicUserInfoAsync(Yuplay2AsyncCall* call);

  virtual bool YU2VCALL checkActivationCode(const char* code);

  virtual Yuplay2Status YU2VCALL getPurchasesCountSync(const char* guid,
                                                       int* count);
  virtual Yuplay2Status YU2VCALL getPurchasesCountAsync(const char* guid,
                                                        Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getMultiPurchasesCountSync(const char** guids,
                                                            unsigned guids_count,
                                                            IYuplay2ItemPurchases** purchases);
  virtual Yuplay2Status YU2VCALL getMultiPurchasesCountAsync(const char** guids,
                                                             unsigned guids_count,
                                                             Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL buyItemSync(const char* guid, Yuplay2PaymentMethods payment);
  virtual Yuplay2Status YU2VCALL buyItemAsync(const char* guid, Yuplay2PaymentMethods payment,
                                              Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL activateItemSync(const char* code);
  virtual Yuplay2Status YU2VCALL activateItemAsync(const char* code, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getItemInfoSync(const char* guid, IYuplay2ItemInfo** info);
  virtual Yuplay2Status YU2VCALL getItemInfoAsync(const char* guid, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL openItemPageSync(const char* guid);
  virtual Yuplay2Status YU2VCALL openItemPageAsync(const char* guid, Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL getMultiItemsInfoSync(const char** guids, unsigned guids_count,
                                                       IYuplay2ItemsInfo** info);
  virtual Yuplay2Status YU2VCALL getMultiItemsInfoAsync(const char** guids, unsigned guids_count,
                                                        Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL checkVersionAsync(const char* folder, const char* guid,
                                                   Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL checkVersionSync(const char* folder, const char* guid,
                                                  bool* new_version);

  virtual Yuplay2Status YU2VCALL checkTagVersionAsync(const char* folder,
                                                      const char* guid,
                                                      const char* tag,
                                                      Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL checkTagVersionSync(const char* folder,
                                                     const char* guid,
                                                     const char* tag,
                                                     bool* new_version);

  virtual Yuplay2Status YU2VCALL getLocalVersionAsync(const char* folder,
                                                      const char* guid,
                                                      Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL getLocalVersionSync(const char* folder,
                                                     const char* guid,
                                                     IYuplay2Binary** version);

  virtual Yuplay2Status YU2VCALL getCurrentYupAsync(const char* guid,
                                                    Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL getCurrentYupSync(const char* guid,
                                                   IYuplay2Binary** yup_file);

  virtual Yuplay2Status YU2VCALL getTagCurrentYupAsync(const char* guid,
                                                       const char* tag,
                                                       Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL getTagCurrentYupSync(const char* guid,
                                                      const char* tag,
                                                      IYuplay2Binary** yup_file);

  virtual Yuplay2Status YU2VCALL checkNewerYupAsync(const char* folder,
                                                    const char* guid,
                                                    const char* tag,
                                                    Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL checkNewerYupYupSync(const char* folder,
                                                      const char* guid,
                                                      const char* tag,
                                                      IYuplay2Binary** yup_file);

  virtual Yuplay2Status YU2VCALL getRemoteVersionAsync(const char* guid,
                                                       const char* tag,
                                                       Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL getRemoteVersionSync(const char* guid,
                                                      const char* tag,
                                                      IYuplay2String** version);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getRemoteVersionSync(const char* guid,
                                                      const char* tag,
                                                      eastl::string& version);
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL buyTencentGoldAsync(unsigned gold, Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL buyTencentGoldSync(unsigned gold);

  virtual Yuplay2Status YU2VCALL getTencentGoldRateAsync(Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL getTencentGoldRateSync(float* rate);

  virtual Yuplay2Status YU2VCALL notifyGooglePlayPurchaseAsync(const char* purchase_json,
                                                               bool do_acknowledge,
                                                               Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL notifyGooglePlayPurchaseSync(const char* purchase_json,
                                                              bool do_acknowledge,
                                                              IYuplay2GooglePurchaseInfo** info);

  virtual Yuplay2Status YU2VCALL notifyHuaweiPurchaseAsync(const char* purchase_json,
                                                           bool do_acknowledge,
                                                           Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL notifyHuaweiPurchaseSync(const char* purchase_json,
                                                          bool do_acknowledge,
                                                          IYuplay2GooglePurchaseInfo** info);

  virtual Yuplay2Status YU2VCALL notifyApplePurchaseAsync(const char* receipt_json,
                                                          Yuplay2AsyncCall* call);
  virtual Yuplay2Status YU2VCALL notifyApplePurchaseSync(const char* receipt_json,
                                                         IYuplay2String** apple_trans_id);
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL notifyApplePurchaseSync(const char* receipt_json,
                                                         eastl::string& apple_trans_id);
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL addUserTagSync(const char* tag);
  virtual Yuplay2Status YU2VCALL addUserTagAsync(const char* tag,
                                                 Yuplay2AsyncCall* call);

  virtual Yuplay2Status YU2VCALL addUserTagsSync(const char** tags,
                                                 unsigned count);
  virtual Yuplay2Status YU2VCALL addUserTagsAsync(const char** tags,
                                                  unsigned count,
                                                  Yuplay2AsyncCall* call);

  virtual int YU2VCALL getNswitchEshopErrorCodeSync() const;
  virtual IYuplay2String* YU2VCALL getNswitchEshopErrorMessageSync() const;
#if YU2EASTL_STRING
  virtual void YU2VCALL getNswitchEshopErrorMessageSync(eastl::string& err_text) const;
#endif //YU2EASTL_STRING

  virtual bool YU2VCALL getPsnPlusStatusSync() const;
  virtual bool YU2VCALL getGamePassStatusSync() const;

  virtual int YU2VCALL getKongZhongFcmSync() const;
#if YU2EASTL_STRING
  virtual void YU2VCALL getKongZhongMsgSync(eastl::string& msg_text) const;
  virtual void YU2VCALL getKongZhongAccountIdSync(eastl::string& accountid) const;

  virtual void YU2VCALL getDistrSync(eastl::string& distr) const;
#endif //YU2EASTL_STRING
};


#endif //__YUPLAY2_SESSION_IMPL__
