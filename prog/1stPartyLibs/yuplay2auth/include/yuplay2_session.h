#ifndef __YUPLAY2_SESSION__
#define __YUPLAY2_SESSION__
#pragma once


#include <yuplay2_def.h>
#include <yuplay2_callback.h>
#include <yuplay2_user.h>
#include <yuplay2_data.h>
#include <yuplay2_friends.h>
#include <yuplay2_item.h>

#if YU2EASTL_STRING
#include <EASTL/string.h>
#else
#include <stdint.h>
#endif //YU2EASTL_STRING


struct IYuplay2Answer;


enum Yuplay2SessionFlags
{
  YU2_SESSION_LOGGED_IN         = 1 << 0
};


enum Yuplay2SessionOptions
{
  YU2_INTERNET_CONNECT_TIMEOUT_SEC = 1,   // Connect timeout in seconds. Int
  YU2_INTERNET_TIMEOUT_SEC,               // Total http request timeout in seconds. Int
  YU2_PSN_AUTH_CONNECT_TIMEOUT_SEC,       // Same as YU2_INTERNET_* but for psnLogin only.
  YU2_PSN_AUTH_TIMEOUT_SEC,               // Set these values after YU2_INTERNET_*
  YU2_DEBUG_CURL,                         // 1 for detailed CURL debug. Default is 0. Int
  YU2_LOGIN_URL,                          // Auth URL. Empty string for default. String
  YU2_TOKEN_URL,                          // Auth by token URL. Empty string for default. String
  YU2_PSN_LOGIN_URL,                      // PSN auth URL. Empty string for default. String
  YU2_VERSION_URL,                        // Game version URL. Empty string for default. String
  YU2_YUP_URL,                            // .yup file URL. Empty string for default. String
  YU2_USER_STOKEN_URL,                    // User short token URL. Empty string for default. String
  YU2_STOKEN_LOGIN_URL,                   // Short token login URL. Empty string for default. String
  YU2_TAG_USER_URL,                       // Tag user URL. Empty string for default. String
  YU2_CURL_CA,                            // Path to allowed CAs. Strings. 0 - host, 1 - path
  YU2_DNS_SERVERS,                        // DNS servers override. Strings
  YU2_USE_AUTH_FAILBACK_IP,               // Use failback IP for auth. Default is 1 (use). Int
  YU2_AUTH_FAILBACK_IP,                   // Custom failback IP. Empty string for default. String
  YU2_USE_YUPMASTER_FAILBACK_IP,          // Use yupmaster failback IP. Default is 1 (use). Int
  YU2_YUPMASTER_FAILBACK_IP,              // Custom yupmaster failback IP. String
  YU2_USE_GAIJIN_NET_FAILBACK_IP,         // Use failback IP for store API. Default is 1 (use). Int
  YU2_GAIJIN_NET_FAILBACK_IP,             // Custom failback IP. Empty string for default. String
  YU2_STEAM_LOGIN_URL,                    // Steam auth URL. Empty string for default. String
  YU2_STEAM_LINK_URL,                     // Steam link URL. Empty string for default. String
  YU2_TENCENT_GOLD_URL,                   // Tencent purchase gold URL. Empty string for default. String
  YU2_TENCENT_PAYMENT_CONF_URL,           // Tencent payment config URL. Empty string for default. String
  YU2_DMM_LOGIN_URL,                      // DMM auth URL. Empty string for default. String
  YU2_CHECK_NEWER_YUP_URL,                // .yup file version check URL. Empty string for default. String
  YU2_GET_TWO_STEP_CODE_URL,              // get two step code by request id
  YU2_SSO_STOKEN_LOGIN_URL,               // SSO short token auth URL. Empty string for default. String
  YU2_CDN_TIMEOUT_SEC,                    // CDN download timeout. It may be larger than YU2_INTERNET_TIMEOUT_SEC
  YU2_VERSION_CONNECT_TIMEOUT_SEC,        // Same as YU2_INTERNET_* but for getRemoteVersion only.
  YU2_VERSION_TIMEOUT_SEC,                // Set these values after YU2_INTERNET_*
  YU2_WEGAME_LOGIN_URL,                   // WeGame auth URL. Empty string for default. String
  YU2_FIREBASE_SECRET,                    // Secret for Firebase auth. Empty string for default. String
  YU2_GUEST_LOGIN_SECRET,                 // Secret for guest auth. Empty string for default. String
  YU2_SYSTEM_ID_FILE_PATH,                // Path to file where random System ID will be stored. String
  YU2_GET_LOGIN_META,                     // 1 to get user meta during login. Default is 0. Int
  YU2_KONGZHONG_CHANNEL_CODE,             // KongZhong chanellCode override. String
  YU2_EXTERNAL_LOGIN_URL,                 // External login URL. Empty string for default. String
  YU2_AUTH_HOST,                          // Replace of AUTH_SERVER_HOST in all URLs. Empty string for default. String
  YU2_SSO_HOST,                           // Replace of SSO_SERVER_HOST in all URLs. Empty string for default. String
  YU2_STORE_API_HOST,                     // Replace of GAIJIN_NET_API_HOST in all URLs. Empty string for default. String
  YU2_YUPMASTER_API_HOST,                 // Replace of YUPMASTER_HOST in all URLs. Empty string for default. String
  YU2_HUAWEI_LOGIN_URL,                   // Huawei auth URL. Empty string for default. String
  YU2_YUPMASTER_FALLBACK_HOSTS,           // Yupmaster fallback hosts override. Strings
  YU2_WEB_LOGIN_TIMEOUT,                  // How long (in seconds) session will check for web login session status. Int
};


enum Yuplay2PaymentMethods
{
  YU2_PAY_NONE              = 0,
  YU2_PAY_QIWI              = 1 << 0,
  YU2_PAY_YANDEX            = 1 << 1,
  YU2_PAY_PAYPAL            = 1 << 2,
  YU2_PAY_WEBMONEY          = 1 << 3,
  YU2_PAY_AMAZON            = 1 << 4,
  YU2_PAY_GJN               = 1 << 5,
  YU2_PAY_BRNTREE           = 1 << 6,
};


struct IYuplay2UserProc
{
  virtual Yuplay2Status YU2VCALL getUserInfoSync(IYuplay2UserInfo** user_info) = 0;
  virtual Yuplay2Status YU2VCALL getUserInfoAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getBasicUserInfoSync(IYuplay2UserInfo** user_info) = 0;
  virtual Yuplay2Status YU2VCALL getBasicUserInfoAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL addUserTagSync(const char* tag) = 0;
  virtual Yuplay2Status YU2VCALL addUserTagAsync(const char* tag,
                                                 Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL addUserTagsSync(const char** tags,
                                                 unsigned count) = 0;
  virtual Yuplay2Status YU2VCALL addUserTagsAsync(const char** tags,
                                                  unsigned count,
                                                  Yuplay2AsyncCall* call) = 0;
};


struct IYuplay2ItemProc
{
  virtual bool YU2VCALL checkActivationCode(const char* code) = 0;

  virtual Yuplay2Status YU2VCALL getPurchasesCountSync(const char* guid,
                                                       int* count) = 0;
  virtual Yuplay2Status YU2VCALL getPurchasesCountAsync(const char* guid,
                                                        Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getMultiPurchasesCountSync(const char** guids,
                                                            unsigned guids_count,
                                                            IYuplay2ItemPurchases** purchases) = 0;
  virtual Yuplay2Status YU2VCALL getMultiPurchasesCountAsync(const char** guids,
                                                             unsigned guids_count,
                                                             Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL buyItemSync(const char* guid,
                                             Yuplay2PaymentMethods payment) = 0;
  virtual Yuplay2Status YU2VCALL buyItemAsync(const char* guid,
                                              Yuplay2PaymentMethods payment,
                                              Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL activateItemSync(const char* code) = 0;
  virtual Yuplay2Status YU2VCALL activateItemAsync(const char* code, Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getItemInfoSync(const char* guid, IYuplay2ItemInfo** info) = 0;
  virtual Yuplay2Status YU2VCALL getItemInfoAsync(const char* guid, Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL openItemPageSync(const char* guid) = 0;
  virtual Yuplay2Status YU2VCALL openItemPageAsync(const char* guid, Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getMultiItemsInfoSync(const char** guids, unsigned guids_count,
                                                       IYuplay2ItemsInfo** info) = 0;
  virtual Yuplay2Status YU2VCALL getMultiItemsInfoAsync(const char** guids, unsigned guids_count,
                                                        Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL checkVersionAsync(const char* folder,
                                                   const char* guid,
                                                   Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL checkVersionSync(const char* folder,
                                                  const char* guid,
                                                  bool* new_version) = 0;

  virtual Yuplay2Status YU2VCALL checkTagVersionAsync(const char* folder,
                                                      const char* guid,
                                                      const char* tag,
                                                      Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL checkTagVersionSync(const char* folder,
                                                     const char* guid,
                                                     const char* tag,
                                                     bool* new_version) = 0;

  virtual Yuplay2Status YU2VCALL getLocalVersionAsync(const char* folder,
                                                      const char* guid,
                                                      Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL getLocalVersionSync(const char* folder,
                                                     const char* guid,
                                                     IYuplay2Binary** version) = 0;

  virtual Yuplay2Status YU2VCALL getCurrentYupAsync(const char* guid,
                                                    Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL getCurrentYupSync(const char* guid,
                                                   IYuplay2Binary** yup_file) = 0;

  virtual Yuplay2Status YU2VCALL getTagCurrentYupAsync(const char* guid,
                                                       const char* tag,
                                                       Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL getTagCurrentYupSync(const char* guid,
                                                      const char* tag,
                                                      IYuplay2Binary** yup_file) = 0;

  virtual Yuplay2Status YU2VCALL checkNewerYupAsync(const char* folder,
                                                    const char* guid,
                                                    const char* tag,
                                                    Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL checkNewerYupYupSync(const char* folder,
                                                      const char* guid,
                                                      const char* tag,
                                                      IYuplay2Binary** yup_file) = 0;

  virtual Yuplay2Status YU2VCALL getRemoteVersionAsync(const char* guid,
                                                       const char* tag,
                                                       Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL getRemoteVersionSync(const char* guid,
                                                      const char* tag,
                                                      IYuplay2String** version) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getRemoteVersionSync(const char* guid,
                                                      const char* tag,
                                                      eastl::string& version) = 0;
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL buyTencentGoldAsync(unsigned gold, Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL buyTencentGoldSync(unsigned gold) = 0;

  virtual Yuplay2Status YU2VCALL getTencentGoldRateAsync(Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL getTencentGoldRateSync(float* rate) = 0;

  virtual Yuplay2Status YU2VCALL notifyGooglePlayPurchaseAsync(const char* purchase_json,
                                                               bool do_acknowledge,
                                                               Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL notifyGooglePlayPurchaseSync(const char* purchase_json,
                                                              bool do_acknowledge,
                                                              IYuplay2GooglePurchaseInfo** info) = 0;

  virtual Yuplay2Status YU2VCALL notifyHuaweiPurchaseAsync(const char* purchase_json,
                                                           bool do_acknowledge,
                                                           Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL notifyHuaweiPurchaseSync(const char* purchase_json,
                                                          bool do_acknowledge,
                                                          IYuplay2GooglePurchaseInfo** info) = 0;

  virtual Yuplay2Status YU2VCALL notifyApplePurchaseAsync(const char* receipt_json,
                                                          Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL notifyApplePurchaseSync(const char* receipt_json,
                                                         IYuplay2String** apple_trans_id) = 0;

#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL notifyApplePurchaseSync(const char* receipt_json,
                                                         eastl::string& apple_trans_id) = 0;
#endif //YU2EASTL_STRING
};


struct IYuplay2Session
{
  virtual void YU2VCALL free() = 0;

  virtual IYuplay2UserProc* YU2VCALL user() = 0;
  virtual IYuplay2ItemProc* YU2VCALL item() = 0;
  virtual IYuplay2Answer* YU2VCALL answer(Yuplay2Handle req) = 0;

  virtual unsigned YU2VCALL getStateSync() = 0;

  virtual Yuplay2Status YU2VCALL setOptionIntSync(Yuplay2SessionOptions id, int val) = 0;
  virtual Yuplay2Status YU2VCALL setOptionStringSync(Yuplay2SessionOptions id, const char* val) = 0;
  virtual Yuplay2Status YU2VCALL setOptionStringsSync(Yuplay2SessionOptions id,
                                                      const char** vals,
                                                      unsigned count) = 0;

  virtual Yuplay2Status YU2VCALL loginSync(const char* login,
                                           const char* password,
                                           bool auth_client,
                                           const char* game_id = NULL) = 0;

  virtual Yuplay2Status YU2VCALL loginAsync(const char* login,
                                            const char* password,
                                            bool auth_client,
                                            Yuplay2AsyncCall* call,
                                            const char* game_id = NULL) = 0;

  virtual Yuplay2Status YU2VCALL twoStepLoginSync(const char* login,
                                                  const char* password,
                                                  const char* code,
                                                  bool auth_client) = 0;

  virtual Yuplay2Status YU2VCALL twoStepLoginAsync(const char* login,
                                                   const char* password,
                                                   const char* code,
                                                   bool auth_client,
                                                   Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL psnLoginSync(const char* auth_code,
                                              const char* game_id,
                                              int issuer,
                                              const char* lang,
                                              const char* title_id = NULL) = 0;
  virtual Yuplay2Status YU2VCALL psnLoginAsync(const char* auth_code,
                                               const char* game_id,
                                               int issuer,
                                               const char* lang,
                                               Yuplay2AsyncCall* call,
                                               const char* title_id = NULL) = 0;

  virtual Yuplay2Status YU2VCALL steamLoginSync(bool link_with_auth_token,
                                                const void* auth_code,
                                                unsigned auth_code_len,
                                                int steam_app_id,
                                                const char* steam_nick,
                                                const char* lang,
                                                bool only_known) = 0;
  virtual Yuplay2Status YU2VCALL steamLoginAsync(bool link_with_auth_token,
                                                 const void* auth_code,
                                                 unsigned auth_code_len,
                                                 int steam_app_id,
                                                 const char* steam_nick,
                                                 const char* lang,
                                                 bool only_known,
                                                 Yuplay2AsyncCall* call) = 0;

  //FIXME: link_with_auth_token obsolete, should be removed from here and game code
  virtual Yuplay2Status YU2VCALL dmmLoginSync(bool link_with_auth_token,
                                              const char* dmm_user_id,
                                              const char* dmm_token,
                                              const char* dmm_app_id,
                                              const char* game_id) = 0;
  virtual Yuplay2Status YU2VCALL dmmLoginAsync(bool link_with_auth_token,
                                               const char* dmm_user_id,
                                               const char* dmm_token,
                                               const char* dmm_app_id,
                                               const char* game_id,
                                               Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL xboxLoginSync(const char* key, const char* sig) = 0;
  virtual Yuplay2Status YU2VCALL xboxLoginAsync(const char* key, const char* sig,
                                                Yuplay2AsyncCall* call) = 0;
  virtual IYuplay2String* YU2VCALL getXboxLoginUrlSync() const = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getXboxLoginUrlSync(eastl::string& url) const = 0;
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL nswitchLoginSync(const char* nintendo_jwt,
                                                  const char* nintendo_nick,
                                                  const char* game_id) = 0;
  virtual Yuplay2Status YU2VCALL nswitchLoginAsync(const char* nintendo_jwt,
                                                   const char* nintendo_nick,
                                                   const char* game_id,
                                                   Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL epicLoginSync(const char* epic_token, const char* game_id) = 0;
  virtual Yuplay2Status YU2VCALL epicLoginAsync(const char* epic_token, const char* game_id,
                                                Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL appleLoginSync(const char* id_token) = 0;
  virtual Yuplay2Status YU2VCALL appleLoginAsync(const char* id_token,
                                                 Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL googlePlayLoginSync(const char* code,
                                                     const char* firebase_id = NULL) = 0;
  virtual Yuplay2Status YU2VCALL googlePlayLoginAsync(const char* code,
                                                      Yuplay2AsyncCall* call,
                                                      const char* firebase_id = NULL) = 0;

  virtual Yuplay2Status YU2VCALL facebookLoginSync(const char* access_token, bool limited) = 0;
  virtual Yuplay2Status YU2VCALL facebookLoginAsync(const char* access_token, bool limited,
                                                    Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL firebaseLoginSync(const char* firebase_id,
                                                   bool only_known = false) = 0;
  virtual Yuplay2Status YU2VCALL firebaseLoginAsync(const char* firebase_id,
                                                    Yuplay2AsyncCall* call,
                                                    bool only_known = false) = 0;

  virtual Yuplay2Status YU2VCALL guestLoginSync(const char* system_id, const char* nick = NULL,
                                                bool only_known = false) = 0;
  virtual Yuplay2Status YU2VCALL guestLoginAsync(const char* system_id, Yuplay2AsyncCall* call,
                                                 const char* nick = NULL,
                                                 bool only_known = false) = 0;

  virtual Yuplay2Status YU2VCALL samsungLoginSync(const char* samsung_token) = 0;
  virtual Yuplay2Status YU2VCALL samsungLoginAsync(const char* samsung_token,
                                                   Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL externalLoginSync(const char* jwt) = 0;
  virtual Yuplay2Status YU2VCALL externalLoginAsync(const char* jwt,
                                                    Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL huaweiLoginSync(const char* jwt) = 0;
  virtual Yuplay2Status YU2VCALL huaweiLoginAsync(const char* jwt,
                                                  Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL nvidiaLoginSync(const char* nvidia_jwt) = 0;
  virtual Yuplay2Status YU2VCALL nvidiaLoginAsync(const char* nvidia_jwt,
                                                  Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL tokenLoginSync(const char* token) = 0;
  virtual Yuplay2Status YU2VCALL tokenLoginAsync(const char* token,
                                                 Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL stokenLoginSync(const char* token) = 0;
  virtual Yuplay2Status YU2VCALL stokenLoginAsync(const char* token,
                                                  Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL ssoStokenLoginSync(const char* token) = 0;
  virtual Yuplay2Status YU2VCALL ssoStokenLoginAsync(const char* token,
                                                     Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getWebLoginSessionIdSync(IYuplay2String** session_id) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getWebLoginSessionIdSync(eastl::string& session_id) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getWebLoginSessionIdAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL webLoginSync(const char* session_id) = 0;
  virtual Yuplay2Status YU2VCALL webLoginAsync(const char* session_id, Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL kongZhongLoginSync(const char* kz_token) = 0;
  virtual Yuplay2Status YU2VCALL kongZhongLoginAsync(const char* kz_token,
                                                     Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL tencentLoginSync() = 0;
  virtual Yuplay2Status YU2VCALL tencentLoginAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL wegameLoginSync(uint64_t wegame_id, const char* ticket,
                                                 const char* nick = NULL) = 0;
  virtual Yuplay2Status YU2VCALL wegameLoginAsync(uint64_t wegame_id, const char* ticket,
                                                  Yuplay2AsyncCall* call,
                                                  const char* nick = NULL) = 0;

  virtual unsigned YU2VCALL getTencentZoneIdSync() = 0;
  virtual IYuplay2String* YU2VCALL getTencentAppDataSync() = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getTencentAppDataSync(eastl::string& data) = 0;
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL getSteamLinkTokenSync(const void* auth_code,
                                                       unsigned auth_code_len,
                                                       int steam_app_id,
                                                       IYuplay2String** token) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSteamLinkTokenSync(const void* auth_code,
                                                       unsigned auth_code_len,
                                                       int steam_app_id,
                                                       eastl::string& token) = 0;
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL getSteamLinkTokenAsync(const void* auth_code,
                                                        unsigned auth_code_len,
                                                        int steam_app_id,
                                                        Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL linkXboxSync(const char* email, const char* key,
                                              const char* sig) = 0;
  virtual Yuplay2Status YU2VCALL linkXboxAsync(const char* email, const char* key,
                                              const char* sig, Yuplay2AsyncCall* call) = 0;
  virtual IYuplay2String* YU2VCALL getXboxLinkUrlSync() const = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getXboxLinkUrlSync(eastl::string& url) const = 0;
#endif //YU2EASTL_STRING

  virtual IYuplay2String* YU2VCALL getUserTokenSync() = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getUserTokenSync(eastl::string& token) = 0;
#endif //YU2EASTL_STRING

  virtual IYuplay2String* YU2VCALL getUserJwtSync() = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getUserJwtSync(eastl::string& jwt) = 0;
#endif //YU2EASTL_STRING

  virtual IYuplay2String* YU2VCALL getCountryCodeSync() = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getCountryCodeSync(eastl::string& code) = 0;
#endif //YU2EASTL_STRING

  virtual int64_t YU2VCALL getUserTokenExpirationTime() = 0;
  virtual int64_t YU2VCALL getUserTokenLifeTime() = 0;

  virtual Yuplay2Status YU2VCALL renewUserTokenSync() = 0;
  virtual Yuplay2Status YU2VCALL renewUserTokenAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL renewUserJwtSync() = 0;
  virtual Yuplay2Status YU2VCALL renewUserJwtAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getTwoStepCodeAsync(Yuplay2AsyncCall* call) = 0;
  virtual Yuplay2Status YU2VCALL getTwoStepCodeSync(IYuplay2String** code) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getTwoStepCodeSync(eastl::string& code) = 0;
#endif //YU2EASTL_STRING

  virtual Yuplay2Status YU2VCALL getLoggedUrlSync(const char* url, IYuplay2String** logged_url,
                                                  const char* base_host = NULL) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getLoggedUrlSync(const char* url, eastl::string& logged_url,
                                                  const char* base_host = NULL) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getLoggedUrlAsync(const char* url, Yuplay2AsyncCall* call,
                                                   const char* base_host = NULL) = 0;

  virtual Yuplay2Status YU2VCALL getSsoLoggedUrlSync(const char* url, IYuplay2String** logged_url,
                                                     const char* base_host = NULL,
                                                     const char* service = NULL) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoLoggedUrlSync(const char* url, eastl::string& logged_url,
                                                     const char* base_host = NULL,
                                                     const char* service = NULL) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoLoggedUrlAsync(const char* url, Yuplay2AsyncCall* call,
                                                      const char* base_host = NULL,
                                                      const char* service = NULL) = 0;

  virtual Yuplay2Status YU2VCALL getKongZhongLoggedUrlSync(const char* url,
                                                           IYuplay2String** logged_url) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getKongZhongLoggedUrlSync(const char* url,
                                                           eastl::string& logged_url) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getKongZhongLoggedUrlAsync(const char* url,
                                                            Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getSsoShortTokenSync(IYuplay2String** short_token) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoShortTokenSync(eastl::string& short_token) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getSsoShortTokenAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getUserStokenSync(IYuplay2String** stoken) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getUserStokenSync(eastl::string& stoken) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getUserStokenAsync(Yuplay2AsyncCall* call) = 0;

  virtual Yuplay2Status YU2VCALL getOpenIdJwtSync(IYuplay2String** openid_jwt) = 0;
#if YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getOpenIdJwtSync(eastl::string& openid_jwt) = 0;
#endif //YU2EASTL_STRING
  virtual Yuplay2Status YU2VCALL getOpenIdJwtAsync(Yuplay2AsyncCall* call) = 0;

  virtual bool YU2VCALL isHasTwoStepRequest() const = 0; //true if external app 2-step available
  virtual bool YU2VCALL isHasGaijinPassTwoStepTypeSync() const = 0; //true if Gaijin Pass set
  virtual bool YU2VCALL isHasWTAssistantTwoStepTypeSync() const = 0; //true if WT Assist set
  virtual bool YU2VCALL isHasEmailTwoStepTypeSync() const = 0; //true if e-mail 2-step set

  virtual int YU2VCALL getNswitchEshopErrorCodeSync() const = 0;
  virtual IYuplay2String* YU2VCALL getNswitchEshopErrorMessageSync() const = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getNswitchEshopErrorMessageSync(eastl::string& err_text) const = 0;
  virtual void YU2VCALL getDistrSync(eastl::string& distr) const = 0;
#endif //YU2EASTL_STRING

  virtual bool YU2VCALL getPsnPlusStatusSync() const = 0;
  virtual bool YU2VCALL getGamePassStatusSync() const = 0;

  virtual int YU2VCALL getKongZhongFcmSync() const = 0;
#if YU2EASTL_STRING
  virtual void YU2VCALL getKongZhongMsgSync(eastl::string& msg_text) const = 0;
  virtual void YU2VCALL getKongZhongAccountIdSync(eastl::string& accountid) const = 0;
#endif //YU2EASTL_STRING
};


#if YU2_LIBRARY_DLL
extern "C"
{
#endif //YU2_LIBRARY_DLL

//Use app_id == -1 to get basic JWT with no rights
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session(const char* client_name = NULL);
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_app_session(const char* client_name,
                                                           unsigned app_id);
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session_distr(const char* client_name,
                                                             const char* distr);
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_app_session_distr(const char* client_name,
                                                                 unsigned app_id,
                                                                 const char* distr);
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_app_game_session_distr(const char* client_name,
                                                                      const char* game,
                                                                      unsigned app_id,
                                                                      const char* distr);

//Does not clone session settings or tokens. Only client_name, app_id, game and distr
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session_from_session(IYuplay2Session* from);
//Does not clone session settings or tokens. Only client_name, game and distr and set new app id
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session_from_session_app(IYuplay2Session* from,
                                                                        unsigned new_app_id);

void YU2CALL yuplay2_shutdown_http();

#if YU2_LIBRARY_DLL
}
#endif //YU2_LIBRARY_DLL


#endif //__YUPLAY2_SESSION__
