#include "stdafx.h"
#include "yu_session.h"
#include "yu_sessionProc.h"
#include "yu_debug.h"
#include "yu_path.h"
#include "yu_fs.h"
#include "yu_system.h"
#include "yu_binary.h"
#include "yu_urls.h"
#include "yu_yupmaster.h"

#include <openssl/md5.h>

#include <time.h>


#if YU2_LIBRARY_DLL
extern "C"
{
  const char *dagor_exe_build_date;
  const char *dagor_exe_build_time;
}

#if YU2_WINAPI
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
  return TRUE;
}
#endif //YU2_WINAPI
#endif //YU2_LIBRARY_DLL



//==================================================================================================
YuSession::YuSession(const char* client_name, const char* game, unsigned app_id, const char* distr) :
    stateBits(0), tokenExpTime(0), tokenLifeTime(0), psnConnectTimeout(0), psnRequestTimeout(0),
    tencentGoldRate(0.0), getMeta(false), tencentZoneId(0), useAuthFailbackIp(true), wegameId(0),
    failbackIp(AUTH_FAILBACK_IP_DEF), yupmasterFailbackIp(YUPMASTER_FAILBACK_IP_DEF),
    useYupmasterFailbackIp(true), hasGaijinPassTwoStepType(false), hasEmailTwoStepType(false),
    hasWTAssistantTwoStepType(false), appId(app_id), versionConnTimeout(0), versionReqTimeout(0),
    gaijinNetApiIp(GAIJIN_NET_FAILBACK_IP_DEF), useGaijinNetFailbackIp(true),
    webLoginStatusTimeout(120)
{
  struct ApiUrl
  {
    ActionId act;
    const char* url;
  };

  ApiUrl urls[] =
  {
    { ACTION_LOGIN, CHECK_LOGIN_URL_DEF },
    { ACTION_TOKEN_LOGIN, CHECK_TOKEN_URL_DEF },
    { ACTION_RENEW_JWT, RENEW_JWT_URL_DEF },
    { ACTION_PSN_LOGIN, PSN_LOGIN_URL_DEF },
    { ACTION_GET_REMOTE_VERSION, GET_VERSION_URL_DEF },
    { ACTION_GET_YUP, GET_YUP_URL_DEF },
    { ACTION_GET_STOKEN, GET_USER_STOKEN_URL_DEF },
    { ACTION_SSO_LOGGED_URL, GET_SSO_USER_STOKEN_URL_DEF },
    { ACTION_SSO_MAKE_LOGGED_URL, SSO_LOGIN_BY_STOKEN_URL_DEF },
    { ACTION_STOKEN_LOGIN, STOKEN_LOGIN_URL_DEF },
    { ACTION_TAG_USER, TAG_USER_URL_DEF },
    { ACTION_STEAM_LOGIN, STEAM_LOGIN_URL_DEF },
    { ACTION_STEAM_LINK_TOKEN, STEAM_LINK_URL_DEF },
    { ACTION_TENCENT_BUY_GOLD, TENCENT_URLS_DEF },
    { ACTION_TENCENT_PAYMENT_CONF, TENCENT_URLS_DEF },
    { ACTION_DMM_LOGIN, DMM_LOGIN_URL_DEF },
    { ACTION_CHECK_NEWER_YUP, CHECK_NEWER_YUP_URL_DEF },
    { ACTION_GET_TWO_STEP_CODE, GET_TWO_STEP_CODE_URL_DEF },
    { ACTION_SSO_STOKEN_LOGIN, SSO_STOKEN_LOGIN_URL_DEF },
    { ACTION_XBOX_LOGIN, XBOX_LOGIN_URL_DEF },
    { ACTION_XBOX_LINK, XBOX_LINK_URL_DEF },
    { ACTION_NSWITCH_LOGIN, NSWITCH_LOGIN_URL_DEF },
    { ACTION_EPIC_LOGIN, EPIC_LOGIN_URL_DEF },
    { ACTION_APPLE_LOGIN, APPLE_LOGIN_URL_DEF },
    { ACTION_GOOGLE_PLAY_LOGIN, GOOGLE_PLAY_LOGIN_URL_DEF },
    { ACTION_WEB_LOGIN_SESSION_ID, WEB_LOGIN_SESSION_ID_URL_DEF },
    { ACTION_WEB_LOGIN, WEB_LOGIN_URL_DEF },
    { ACTION_WEGAME_LOGIN, WEGAME_LOGIN_URL_DEF },
    { ACTION_FACEBOOK_LOGIN, FACEBOOK_LOGIN_URL_DEF },
    { ACTION_FIREBASE_LOGIN, FIREBASE_LOGIN_URL_DEF },
    { ACTION_GUEST_LOGIN, GUEST_LOGIN_URL_DEF },
    { ACTION_GOOGLE_PURCHASE, GOOGLE_PURCHASE_URL_DEF },
    { ACTION_HUAWEI_PURCHASE, HUAWEI_PURCHASE_URL_DEF },
    { ACTION_APPLE_PURCHASE, APPLE_PURCHASE_URL_DEF },
    { ACTION_SAMSUNG_LOGIN, SAMSUNG_LOGIN_URL_DEF },
    { ACTION_GET_OPENID_JWT, GET_OPENID_URL_DEF },
    { ACTION_EXTERNAL_LOGIN, EXTERNAL_LOGIN_URL_DEF },
    { ACTION_WEB_LOGIN_BROWSER, WEB_LOGIN_BROWSER_URL_DEF },
    { ACTION_ITEM_INFO, ITEM_INFO_URL_DEF },
    { ACTION_BUY_ITEM, PURCHASE_ITEM_URL_DEF },
    { ACTION_ACTIVATE, REDEEM_CODE_URL_DEF },
    { ACTION_HUAWEI_LOGIN, HUAWEI_LOGIN_URL_DEF },
    { ACTION_NVIDIA_LOGIN, NVIDIA_LOGIN_URL_DEF },
  };

  memset(currentActions, 0, sizeof(currentActions));

  YuString host;
  for (size_t i = 0; i < __ACTION_LAST; ++i)
  {
    for (size_t ui = 0; ui < sizeof(urls) / sizeof(urls[0]); ++ui)
      if (urls[ui].act == i)
      {
        apiUrls[i] = urls[ui].url;

        const YuUrl& url = apiUrls[i];
        host = url.host();

        if (host == AUTH_SERVER_HOST)
          authUrls.set(i, true);
        else if (host == SSO_SERVER_HOST)
          ssoUrls.set(i, true);
        else if (host == GAIJIN_NET_API_HOST)
          gjApiUrls.set(i, true);
        else if (host == YUPMASTER_HOST)
          yupmasterUrls.set(i, true);

        break;
      }
  }

  if (client_name && *client_name)
  {
    cliName = client_name;

    //FIXME: We better use session-specific User Agent instead of global one
    YuHttp::self().setClientString(cliName);
  }

  if (distr && *distr)
    distrId = distr;

  if (game && *game)
    gameId = game;

  YuDebug::out("Created yuplay2 session for %s / %u / %s", cliName.c_str(), appId, distrId.c_str());
}


//==================================================================================================
IYuplay2Session* YuSession::createFromMe() const
{
  return new YuSession(cliName.c_str(), gameId.c_str(), appId, distrId.c_str());
}


//==================================================================================================
IYuplay2Session* YuSession::createFromMe(unsigned new_app_id) const
{
  return new YuSession(cliName.c_str(), gameId.c_str(), new_app_id, distrId.c_str());
}


//==================================================================================================
void YU2VCALL YuSession::free()
{
  sessionLock.lock();

  for (int i = 0; i < __ACTION_LAST; ++i)
  {
    AsyncAction* act = currentActions[i];

    if (act && act->running())
    {
      YuDebug::out("Action %d is active, stop it", act->getId());
      act->stop();
    }
  }

  sessionLock.unlock();

  for (;;)
  {
    bool done = true;

    sessionLock.lock();

    for (int i = 0; i < __ACTION_LAST; ++i)
    {
      AsyncAction* act = currentActions[i];

      if (act)
      {
        if (act->running())
        {
          YuDebug::out("Action %d is still active, waiting more", act->getId());

          done = false;
        }
        else
        {
          delete act;
          currentActions[i] = NULL;
        }
      }
    }

    sessionLock.unlock();

    if (done)
      break;
    else
      YuSystem::sleep(100);
  }

  delete this;
}


//==================================================================================================
unsigned YU2VCALL YuSession::getStateSync()
{
  YuWarden warden(sessionLock);
  return stateBits;
}


//==================================================================================================
IYuplay2Answer* YU2VCALL YuSession::answer(Yuplay2Handle req)
{
  if (req)
    return ((AsyncAction*)req)->getAnswer();

  return NULL;
}


//==================================================================================================
unsigned YU2VCALL YuSession::getTencentZoneIdSync()
{
  YuWarden warden(sessionLock);
  return tencentZoneId;
}


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getTencentAppDataSync()
{
  YuWarden warden(sessionLock);
  return new YuSessionString(tencentAppData);
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getTencentAppDataSync(eastl::string& data)
{
  YuWarden warden(sessionLock);
  data = tencentAppData;
}
#endif //YU2EASTL_STRING


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getUserTokenSync()
{
  YuWarden warden(sessionLock);
  return new YuSessionString(yuToken);
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getUserTokenSync(eastl::string& token)
{
  YuWarden warden(sessionLock);
  token = yuToken;
}
#endif //YU2EASTL_STRING


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getUserJwtSync()
{
  YuWarden warden(sessionLock);
  return new YuSessionString(yuJwt);
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getUserJwtSync(eastl::string& jwt)
{
  YuWarden warden(sessionLock);
  jwt = yuJwt;
}
#endif //YU2EASTL_STRING


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getCountryCodeSync()
{
  YuWarden warden(sessionLock);
  return new YuSessionString(yuCountryCode);
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getCountryCodeSync(eastl::string& code)
{
  YuWarden warden(sessionLock);
  code = yuCountryCode;
}
#endif //YU2EASTL_STRING


//==================================================================================================
int64_t YU2VCALL YuSession::getUserTokenExpirationTime()
{
  YuWarden warden(sessionLock);
  return tokenExpTime;
}


//==================================================================================================
int64_t YU2VCALL YuSession::getUserTokenLifeTime()
{
  YuWarden warden(sessionLock);
  return tokenLifeTime;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::setOptionIntSync(Yuplay2SessionOptions id, int val)
{
  YuWarden warden(sessionLock);

  switch (id)
  {
    case YU2_INTERNET_CONNECT_TIMEOUT_SEC:
      YuHttp::self().setConnectTimeoutSec(val);
      psnConnectTimeout = val;
      versionConnTimeout = val;
      return YU2_OK;

    case YU2_INTERNET_TIMEOUT_SEC:
      YuHttp::self().setTimeoutSec(val);
      psnRequestTimeout = val;
      versionReqTimeout = val;
      return YU2_OK;

    case YU2_CDN_TIMEOUT_SEC:
      YupmasterGetFile::setDownloadTimeoutSec(val);
      return YU2_OK;

    case YU2_PSN_AUTH_CONNECT_TIMEOUT_SEC:
      psnConnectTimeout = val;
      return YU2_OK;

    case YU2_PSN_AUTH_TIMEOUT_SEC:
      psnRequestTimeout = val;
      return YU2_OK;

    case YU2_VERSION_CONNECT_TIMEOUT_SEC:
      versionConnTimeout = val;
      return YU2_OK;

    case YU2_VERSION_TIMEOUT_SEC:
      versionReqTimeout = val;
      return YU2_OK;

    case YU2_DEBUG_CURL:
      YuHttp::self().setCurlDebug(val != 0);
      return YU2_OK;

    case YU2_USE_AUTH_FAILBACK_IP:
      useAuthFailbackIp = (val != 0);
      return YU2_OK;

    case YU2_USE_YUPMASTER_FAILBACK_IP:
      useYupmasterFailbackIp = (val != 0);
      return YU2_OK;

    case YU2_USE_GAIJIN_NET_FAILBACK_IP:
      useGaijinNetFailbackIp = (val != 0);
      return YU2_OK;

    case YU2_GET_LOGIN_META:
      getMeta = (val != 0);
      return YU2_OK;

    case YU2_WEB_LOGIN_TIMEOUT:
      webLoginStatusTimeout = val;
      return YU2_OK;

    default:
      break;
  }

  return YU2_WRONG_PARAMETER;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::setOptionStringSync(Yuplay2SessionOptions id, const char* val)
{
  YuString str(val && *val ? val : "");

  YuWarden warden(sessionLock);

  switch (id)
  {
    case YU2_LOGIN_URL:
      apiUrls[ACTION_LOGIN] = str.length() ? str : CHECK_LOGIN_URL_DEF;
      YuDebug::out("Overrided login URL: %s", apiUrls[ACTION_LOGIN].c_str());
      return YU2_OK;

    case YU2_TOKEN_URL:
      apiUrls[ACTION_TOKEN_LOGIN] = str.length() ? str : CHECK_TOKEN_URL_DEF;
      YuDebug::out("Overrided token login URL: %s", apiUrls[ACTION_TOKEN_LOGIN].c_str());
      return YU2_OK;

    case YU2_PSN_LOGIN_URL:
      apiUrls[ACTION_PSN_LOGIN] = str.length() ? str : PSN_LOGIN_URL_DEF;
      YuDebug::out("Overrided PSN login URL: %s", apiUrls[ACTION_PSN_LOGIN].c_str());
      return YU2_OK;

    case YU2_VERSION_URL:
      apiUrls[ACTION_GET_REMOTE_VERSION] = str.length() ? str : GET_VERSION_URL_DEF;
      YuDebug::out("Overrided game version URL: %s", apiUrls[ACTION_GET_REMOTE_VERSION].c_str());
      return YU2_OK;

    case YU2_YUP_URL:
      apiUrls[ACTION_GET_YUP] = str.length() ? str : GET_YUP_URL_DEF;
      YuDebug::out("Overrided .yup download URL: %s", apiUrls[ACTION_GET_YUP].c_str());
      return YU2_OK;

    case YU2_CHECK_NEWER_YUP_URL:
      apiUrls[ACTION_CHECK_NEWER_YUP] = str.length() ? str : CHECK_NEWER_YUP_URL_DEF;
      YuDebug::out("Overrided .yup version check URL: %s", apiUrls[ACTION_CHECK_NEWER_YUP].c_str());
      return YU2_OK;

    case YU2_USER_STOKEN_URL:
      apiUrls[ACTION_GET_STOKEN] = str.length() ? str : GET_USER_STOKEN_URL_DEF;
      YuDebug::out("Overrided short token URL: %s", apiUrls[ACTION_GET_STOKEN].c_str());
      return YU2_OK;

    case YU2_STOKEN_LOGIN_URL:
      apiUrls[ACTION_STOKEN_LOGIN] = str.length() ? str : STOKEN_LOGIN_URL_DEF;
      YuDebug::out("Overrided short token login URL: %s", apiUrls[ACTION_STOKEN_LOGIN].c_str());
      return YU2_OK;

    case YU2_TAG_USER_URL:
      apiUrls[ACTION_TAG_USER] = str.length() ? str : TAG_USER_URL_DEF;
      YuDebug::out("Overrided tag user URL: %s", apiUrls[ACTION_TAG_USER].c_str());
      return YU2_OK;

    case YU2_AUTH_FAILBACK_IP:
      failbackIp = str.length() ? str : AUTH_FAILBACK_IP_DEF;
      YuDebug::out("Overrided fallback IP: %s", failbackIp.c_str());
      return YU2_OK;

    case YU2_YUPMASTER_FAILBACK_IP:
      yupmasterFailbackIp = str.length() ? str : YUPMASTER_FAILBACK_IP_DEF;
      YuDebug::out("Overrided yupmaster fallback IP: %s", yupmasterFailbackIp.c_str());
      return YU2_OK;

    case YU2_GAIJIN_NET_FAILBACK_IP:
      gaijinNetApiIp = str.length() ? str : GAIJIN_NET_FAILBACK_IP_DEF;
      YuDebug::out("Overrided Gaijin.Net fallback IP: %s", gaijinNetApiIp.c_str());
      return YU2_OK;

    case YU2_STEAM_LOGIN_URL:
      apiUrls[ACTION_STEAM_LOGIN] = str.length() ? str : STEAM_LOGIN_URL_DEF;
      YuDebug::out("Overrided Steam login URL: %s", apiUrls[ACTION_STEAM_LOGIN].c_str());
      return YU2_OK;

    case YU2_STEAM_LINK_URL:
      apiUrls[ACTION_STEAM_LINK_TOKEN] = str.length() ? str : STEAM_LINK_URL_DEF;
      YuDebug::out("Overrided Steam link URL: %s", apiUrls[ACTION_STEAM_LINK_TOKEN].c_str());
      return YU2_OK;

    case YU2_TENCENT_GOLD_URL:
      apiUrls[ACTION_TENCENT_BUY_GOLD] = str.length() ? str : TENCENT_URLS_DEF;
      YuDebug::out("Overrided Tencent purchase gold URL: %s", apiUrls[ACTION_TENCENT_BUY_GOLD].c_str());
      break;

    case YU2_TENCENT_PAYMENT_CONF_URL:
      apiUrls[ACTION_TENCENT_PAYMENT_CONF] = str.length() ? str : TENCENT_URLS_DEF;
      YuDebug::out("Overrided Tencent payment conf URL: %s", apiUrls[ACTION_TENCENT_PAYMENT_CONF].c_str());
      break;

    case YU2_DMM_LOGIN_URL:
      apiUrls[ACTION_DMM_LOGIN] = str.length() ? str : DMM_LOGIN_URL_DEF;
      YuDebug::out("Overrided DMM login URL: %s", apiUrls[ACTION_DMM_LOGIN].c_str());
      return YU2_OK;

    case YU2_GET_TWO_STEP_CODE_URL:
      apiUrls[ACTION_GET_TWO_STEP_CODE] = str.length() ? str : GET_TWO_STEP_CODE_URL_DEF;
      YuDebug::out("Overrided two step code URL: %s", apiUrls[ACTION_GET_TWO_STEP_CODE].c_str());
      return YU2_OK;

    case YU2_SSO_STOKEN_LOGIN_URL:
      apiUrls[ACTION_SSO_STOKEN_LOGIN] = str.length() ? str : SSO_STOKEN_LOGIN_URL_DEF;
      YuDebug::out("Overrided SSO short token login URL: %s", apiUrls[ACTION_SSO_STOKEN_LOGIN].c_str());
      return YU2_OK;

    case YU2_WEGAME_LOGIN_URL:
      apiUrls[ACTION_WEGAME_LOGIN] = str.length() ? str : WEGAME_LOGIN_URL_DEF;
      YuDebug::out("Overrided WeGame login URL: %s", apiUrls[ACTION_WEGAME_LOGIN].c_str());
      return YU2_OK;

    case YU2_FIREBASE_SECRET:
      firebaseSecret = str.length() ? str : "";
      return YU2_OK;

    case YU2_GUEST_LOGIN_SECRET:
      guestSecret = str.length() ? str : "";
      return YU2_OK;

    case YU2_SYSTEM_ID_FILE_PATH:
      if (YuSystem::setSystemIdPath(str))
      {
        YuDebug::out("System ID path: %s", str.c_str());
        return YU2_OK;
      }
      return YU2_FAIL;

    case YU2_KONGZHONG_CHANNEL_CODE:
      kzChannelCode = str.length() ? str : "";
      YuDebug::out("Overrided channelCode: %s", str.c_str());
      return YU2_OK;

    case YU2_EXTERNAL_LOGIN_URL:
      apiUrls[ACTION_EXTERNAL_LOGIN] = str.length() ? str : EXTERNAL_LOGIN_URL_DEF;
      YuDebug::out("Overrided external login URL: %s", apiUrls[ACTION_EXTERNAL_LOGIN].c_str());
      return YU2_OK;

    case YU2_HUAWEI_LOGIN_URL:
      apiUrls[ACTION_HUAWEI_LOGIN] = str.length() ? str : HUAWEI_LOGIN_URL_DEF;
      YuDebug::out("Overrided huawei login URL: %s", apiUrls[ACTION_HUAWEI_LOGIN].c_str());
      return YU2_OK;

    case YU2_AUTH_HOST:
      updateApiHost(authUrls, str.length() ? str.c_str() : AUTH_SERVER_HOST);
      return YU2_OK;

    case YU2_SSO_HOST:
      updateApiHost(ssoUrls, str.length() ? str.c_str() : SSO_SERVER_HOST);
      return YU2_OK;

    case YU2_STORE_API_HOST:
      updateApiHost(gjApiUrls, str.length() ? str.c_str() : GAIJIN_NET_API_HOST);
      return YU2_OK;

    case YU2_YUPMASTER_API_HOST:
      updateApiHost(yupmasterUrls, str.length() ? str.c_str() : YUPMASTER_HOST);
      return YU2_OK;

    default:
      break;
  }

  return YU2_WRONG_PARAMETER;
}


//==================================================================================================
template<class Cont>
static bool getSessionStrings(Cont& container, const char** vals, unsigned count)
{
  for (unsigned i = 0; i < count; ++i)
  {
    if (!CHECK_STRING(vals[i]))
      return false;

    container.insert(container.end(), vals[i]);
  }

  return true;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::setOptionStringsSync(Yuplay2SessionOptions id, const char** vals,
                                                       unsigned count)
{
  if (count && !vals)
    return YU2_WRONG_PARAMETER;

  switch (id)
  {
    case YU2_CURL_CA:
      if (count < 2 || count % 2)
        return YU2_WRONG_PARAMETER;

      for (unsigned i = 0; i < count; i += 2)
      {
        if (!CHECK_STRING(vals[i])) //-V522
          return YU2_WRONG_PARAMETER;

        YuHttp::self().setHostCA(vals[i], vals[i + 1] ? vals[i + 1] : "");
      }
      return YU2_OK;

    case YU2_DNS_SERVERS:
    {
      YuStringSet dns;
      if (getSessionStrings(dns, vals, count))
      {
        YuHttp::self().setDns(dns);
        return YU2_OK;
      }
      break;
    }

    case YU2_YUPMASTER_FALLBACK_HOSTS:
    {
      YuStringTab hosts;
      hosts.reserve(count);

      if (getSessionStrings(hosts, vals, count))
      {
        if (count)
          YuDebug::out("%u yupmaster fallback hosts added", count);
        else
          YuDebug::out("yupmaster fallback hosts removed");

        yupmasterFallbackHosts = hosts;
        return YU2_OK;
      }
      break;
    }

    default:
      break;
  }

  return YU2_WRONG_PARAMETER;
}


//==================================================================================================
void YuSession::updateApiHost(const ApiHosts& hosts, const char* host)
{
  for (size_t i = 0; i < __ACTION_LAST; ++i)
    if (hosts.test(i))
      apiUrls[i].setHost(host);
}


//==================================================================================================
void YuSession::gcActions()
{
  YuWarden warden(sessionLock);

  for (int i = 0; i < __ACTION_LAST; ++i)
  {
    AsyncAction* act = currentActions[i];

    if (act && act->completed())
    {
      currentActions[i] = NULL;
      delete act;
    }
  }
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID>
Yuplay2Status YuSession::createAction(Yuplay2AsyncCall* call, ACTION*& action)
{
  gcActions();

  YuWarden warden(sessionLock);

  if (currentActions[ACT_ID])
    return YU2_BUSY;

  if (NEED_LOGIN && !(stateBits & YU2_SESSION_LOGGED_IN))
    return YU2_NOT_LOGGED_IN;

  action = new ACTION(*this, ACT_ID, call ? call->cb : NULL);

  if (action)
  {
    currentActions[ACT_ID] = action;

    return YU2_OK;
  }

  return YU2_FAIL;
}


//==================================================================================================
void YuSession::stopAction(ActionId id)
{
  YuWarden warden(sessionLock);

  AsyncAction* act = currentActions[id];
  if (act && act->running())
  {
    YuDebug::out("Action %d is active, stop it and wait", act->getId());

    act->stop();
    act->wait();
  }
}


//==================================================================================================
template<class ACTION, YuSession::ActionId ACT_ID>
Yuplay2Status YuSession::doSyncAction(ACTION* action, const void** result)
{
  action->wait();

  sessionLock.lock();
  currentActions[ACT_ID] = NULL;
  sessionLock.unlock();

  Yuplay2Status ret = action->getStatus();

  if (result)
    *result = action->getResult();

  delete action;

  return ret;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run())
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      currentActions[ACT_ID] = NULL;

      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2, class V3>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2, const V3& v3,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2, v3))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2, class V3, class V4>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2, v3, v4))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2, class V3, class V4, class V5>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                                  const V5& v5,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2, v3, v4, v5))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2, class V3, class V4, class V5, class V6>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                                  const V5& v5, const V6& v6,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2, v3, v4, v5, v6))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2, class V3, class V4, class V5, class V6, class V7>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                                  const V5& v5, const V6& v6, const V7& v7,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2, v3, v4, v5, v6, v7))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
template<bool NEED_LOGIN, class ACTION, YuSession::ActionId ACT_ID,
         class V1, class V2, class V3, class V4, class V5, class V6, class V7, class V8>
Yuplay2Status YuSession::doAction(Yuplay2AsyncCall* call,
                                  const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                                  const V5& v5, const V6& v6, const V7& v7, const V8& v8,
                                  const void** result)
{
  ACTION* action = NULL;
  Yuplay2Status res = createAction<NEED_LOGIN, ACTION, ACT_ID>(call, action);

  if (res == YU2_OK)
  {
    if (action->run(v1, v2, v3, v4, v5, v6, v7, v8))
    {
      if (call) //Async action, just return its handle
        call->handle = action;
      else //Sync action, wait it until done
        res = doSyncAction<ACTION, ACT_ID>(action, result);
    }
    else
    {
      delete action;
      res = YU2_FAIL;
    }
  }

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::loginSync(const char* login, const char* password,
                                            bool auth_client, const char* game_id)
{
  if (!CHECK_STRING(login) || !CHECK_STRING(password))
    return YU2_WRONG_PARAMETER;

  return doAction<false, LoginAction, ACTION_LOGIN,
                  YuString, YuString, bool, YuString>
                 (NULL, login, password, auth_client, game_id ? game_id : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::loginAsync(const char* login, const char* password,
                                             bool auth_client, Yuplay2AsyncCall* call,
                                             const char* game_id)
{
  if (!call || !CHECK_STRING(login) || !CHECK_STRING(password))
    return YU2_WRONG_PARAMETER;

  return doAction<false, LoginAction, ACTION_LOGIN,
                  YuString, YuString, bool, YuString>
                 (call, login, password, auth_client, game_id ? game_id : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::twoStepLoginSync(const char* login, const char* password,
                                                   const char* code, bool auth_client)
{
  if (!CHECK_STRING(login) || !CHECK_STRING(password)|| !CHECK_STRING(code))
    return YU2_WRONG_PARAMETER;

  return doAction<false, TwoStepLoginAction, ACTION_2STEP_LOGIN,
                  YuString, YuString, YuString, bool>
                 (NULL, login, password, code, auth_client, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::twoStepLoginAsync(const char* login, const char* password,
                                                    const char* code, bool auth_client,
                                                    Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(login) || !CHECK_STRING(password) || !CHECK_STRING(code))
    return YU2_WRONG_PARAMETER;

  return doAction<false, TwoStepLoginAction, ACTION_2STEP_LOGIN,
                  YuString, YuString, YuString, bool>
                 (call, login, password, code, auth_client, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::psnLoginSync(const char* auth_code, const char* game_id,
                                               int issuer, const char* lang,
                                               const char* title_id)
{
  if (!CHECK_STRING(auth_code) || !CHECK_STRING(game_id))
    return YU2_WRONG_PARAMETER;

  return doAction<false, PsnLoginAction, ACTION_PSN_LOGIN,
                  YuString, YuString, int, YuString, unsigned, unsigned, YuString>
                 (NULL, auth_code, game_id, issuer, lang ? lang : "",
                  psnConnectTimeout, psnRequestTimeout, title_id ? title_id : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::psnLoginAsync(const char* auth_code, const char* game_id,
                                                int issuer, const char* lang,
                                                Yuplay2AsyncCall* call,
                                                const char* title_id)
{
  if (!call || !CHECK_STRING(auth_code) || !CHECK_STRING(game_id))
    return YU2_WRONG_PARAMETER;

  return doAction<false, PsnLoginAction, ACTION_PSN_LOGIN,
                  YuString, YuString, int, YuString, unsigned, unsigned, YuString>
                 (call, auth_code, game_id, issuer, lang ? lang : "",
                  psnConnectTimeout, psnRequestTimeout, title_id ? title_id : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::steamLoginSync(bool link_with_auth_token, const void* auth_code,
                                                 unsigned auth_code_len, int steam_app_id,
                                                 const char* steam_nick, const char* lang,
                                                 bool only_known)
{
  if (!auth_code || !auth_code_len || !CHECK_STRING(lang) || !CHECK_STRING(steam_nick))
    return YU2_WRONG_PARAMETER;

  YuMemRange<char> codeRange((const char*)auth_code, auth_code_len);

  return doAction<false, SteamLoginAction, ACTION_STEAM_LOGIN,
                  YuString, YuMemRange<char>, int, YuString, YuString, bool>
                 (NULL, link_with_auth_token ? yuToken : "", codeRange, steam_app_id,
                  steam_nick, lang, only_known, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::steamLoginAsync(bool link_with_auth_token, const void* auth_code,
                                                  unsigned auth_code_len, int steam_app_id,
                                                  const char* steam_nick, const char* lang,
                                                  bool only_known, Yuplay2AsyncCall* call)
{
  if (!call || !auth_code || !auth_code_len || !CHECK_STRING(lang) || !CHECK_STRING(steam_nick))
    return YU2_WRONG_PARAMETER;

  YuMemRange<char> codeRange((const char*)auth_code, auth_code_len);

  return doAction<false, SteamLoginAction, ACTION_STEAM_LOGIN,
                  YuString, YuMemRange<char>, int, YuString, YuString, bool>
                 (call, link_with_auth_token ? yuToken : "", codeRange,
                  steam_app_id, steam_nick, lang, only_known, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::dmmLoginSync(bool link_with_auth_token, const char* dmm_user_id,
                                               const char* dmm_token, const char* dmm_app_id,
                                               const char* game_id)
{
  if (!CHECK_STRING(dmm_token) || !CHECK_STRING(dmm_user_id) ||
    !CHECK_STRING(dmm_app_id) || !CHECK_STRING(game_id))
      return YU2_WRONG_PARAMETER;

  return doAction<false, DmmLoginAction, ACTION_DMM_LOGIN,
                  YuString, YuString, YuString, YuString>
                 (NULL, dmm_user_id, dmm_token, dmm_app_id, game_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::dmmLoginAsync(bool link_with_auth_token, const char* dmm_user_id,
                                                const char* dmm_token, const char* dmm_app_id,
                                                const char* game_id, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(dmm_token) || !CHECK_STRING(dmm_user_id) ||
    !CHECK_STRING(dmm_app_id) || !CHECK_STRING(game_id))
      return YU2_WRONG_PARAMETER;

  return doAction<false, DmmLoginAction, ACTION_DMM_LOGIN,
                  YuString, YuString, YuString, YuString>
                 (call, dmm_user_id, dmm_token, dmm_app_id, game_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::xboxLoginSync(const char* key, const char* sig)
{
  if (!CHECK_STRING(key))
    return YU2_WRONG_PARAMETER;

  return doAction<false, XboxLoginAction, ACTION_XBOX_LOGIN,
                  YuString, YuString>(NULL, key, sig, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::xboxLoginAsync(const char* key, const char* sig, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(key))
    return YU2_WRONG_PARAMETER;

  return doAction<false, XboxLoginAction, ACTION_XBOX_LOGIN,
                  YuString, YuString>(call, key, sig, NULL);
}


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getXboxLoginUrlSync() const
{
  return new YuSessionString(getXboxLoginUrl());
}


//==================================================================================================
#if YU2EASTL_STRING
void YU2VCALL YuSession::getXboxLoginUrlSync(eastl::string& url) const
{
  url = getXboxLoginUrl();
}
#endif


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::nswitchLoginSync(const char* nintendo_jwt,
                                                   const char* nintendo_nick, const char* game_id)
{
  if (!nintendo_jwt || !nintendo_nick || !CHECK_STRING(game_id))
    return YU2_WRONG_PARAMETER;

  return doAction<false, NSwitchLoginAction, ACTION_NSWITCH_LOGIN,
                  YuString, YuString>
                  (NULL, nintendo_jwt, game_id, nintendo_nick, "en-US", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::nswitchLoginAsync(const char* nintendo_jwt,
                                                    const char* nintendo_nick, const char* game_id,
                                                    Yuplay2AsyncCall* call)
{
  if (!nintendo_jwt || !call || !nintendo_nick || !CHECK_STRING(game_id))
    return YU2_WRONG_PARAMETER;

  return doAction<false, NSwitchLoginAction, ACTION_NSWITCH_LOGIN,
                  YuString, YuString>
                  (call, nintendo_jwt, game_id, nintendo_nick, "en-US", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::epicLoginSync(const char* epic_token, const char* game_id)
{
  if (!CHECK_STRING(epic_token) || !CHECK_STRING(game_id))
    return YU2_WRONG_PARAMETER;

  return doAction<false, EpicLoginAction, ACTION_EPIC_LOGIN,
                  YuString, YuString>
                  (NULL, epic_token, game_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::epicLoginAsync(const char* epic_token, const char* game_id,
                                                 Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(epic_token) || !CHECK_STRING(game_id))
    return YU2_WRONG_PARAMETER;

  return doAction<false, EpicLoginAction, ACTION_EPIC_LOGIN,
                  YuString, YuString>
                  (call, epic_token, game_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::appleLoginSync(const char* id_token)
{
  if (!CHECK_STRING(id_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, AppleLoginAction, ACTION_APPLE_LOGIN,
                  YuString>
                  (NULL, id_token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::appleLoginAsync(const char* id_token, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(id_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, AppleLoginAction, ACTION_APPLE_LOGIN,
                  YuString>
                  (call, id_token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::googlePlayLoginSync(const char* code, const char* firebase_id)
{
  if (!CHECK_STRING(code))
    return YU2_WRONG_PARAMETER;

  return doAction<false, GooglePlayLoginAction, ACTION_GOOGLE_PLAY_LOGIN,
                  YuString, YuString>
                  (NULL, code, firebase_id ? firebase_id : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::googlePlayLoginAsync(const char* code, Yuplay2AsyncCall* call,
                                                       const char* firebase_id)
{
  if (!call || !CHECK_STRING(code))
    return YU2_WRONG_PARAMETER;

  return doAction<false, GooglePlayLoginAction, ACTION_GOOGLE_PLAY_LOGIN,
                  YuString, YuString>
                  (call, code, firebase_id ? firebase_id : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::facebookLoginSync(const char* access_token, bool limited)
{
  if (!CHECK_STRING(access_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, FacebookLoginAction, ACTION_FACEBOOK_LOGIN,
                  YuString, bool>
                  (NULL, access_token, limited, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::facebookLoginAsync(const char* access_token, bool limited,
                                                     Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(access_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, FacebookLoginAction, ACTION_FACEBOOK_LOGIN,
                  YuString, bool>
                  (call, access_token, limited, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::firebaseLoginSync(const char* firebase_id, bool only_known)
{
  if (!CHECK_STRING(firebase_id) || firebaseSecret.empty())
    return YU2_WRONG_PARAMETER;

  return doAction<false, FirebaseLoginAction, ACTION_FIREBASE_LOGIN,
                  YuString, YuString, bool>
                  (NULL, firebase_id, firebaseSecret, only_known, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::firebaseLoginAsync(const char* firebase_id,
                                                     Yuplay2AsyncCall* call, bool only_known)
{
  if (!CHECK_STRING(firebase_id) || firebaseSecret.empty())
    return YU2_WRONG_PARAMETER;

  return doAction<false, FirebaseLoginAction, ACTION_FIREBASE_LOGIN,
                  YuString, YuString, bool>
                  (call, firebase_id, firebaseSecret, only_known, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::guestLoginSync(const char* system_id, const char* nick,
                                                 bool only_known)
{
  if (!CHECK_STRING(system_id) || guestSecret.empty() || ::strlen(system_id) < 10)
    return YU2_WRONG_PARAMETER;

  return doAction<false, GuestLoginAction, ACTION_GUEST_LOGIN,
                  YuString, YuString, YuString, bool>
                  (NULL, system_id, guestSecret, nick ? nick : "", only_known, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::guestLoginAsync(const char* system_id, Yuplay2AsyncCall* call,
                                                  const char* nick, bool only_known)
{
  if (!CHECK_STRING(system_id) || guestSecret.empty() || ::strlen(system_id) < 10)
    return YU2_WRONG_PARAMETER;

  return doAction<false, GuestLoginAction, ACTION_GUEST_LOGIN,
                  YuString, YuString, YuString, bool>
                  (call, system_id, guestSecret, nick ? nick : "", only_known, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::samsungLoginSync(const char* samsung_token)
{
  if (!CHECK_STRING(samsung_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, SamsungLoginAction, ACTION_SAMSUNG_LOGIN,
                  YuString>
                  (NULL, samsung_token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::samsungLoginAsync(const char* samsung_token,
                                                    Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(samsung_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, SamsungLoginAction, ACTION_SAMSUNG_LOGIN,
                  YuString>
                  (call, samsung_token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::externalLoginSync(const char* jwt)
{
  if (!CHECK_STRING(jwt))
    return YU2_WRONG_PARAMETER;

  return doAction<false, ExternalLoginAction, ACTION_EXTERNAL_LOGIN,
                  YuString>
                  (NULL, jwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::externalLoginAsync(const char* jwt, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(jwt))
    return YU2_WRONG_PARAMETER;

  return doAction<false, ExternalLoginAction, ACTION_EXTERNAL_LOGIN,
                  YuString>
                  (call, jwt, NULL);
}

//==================================================================================================
Yuplay2Status YU2VCALL YuSession::huaweiLoginSync(const char* jwt)
{
  if (!CHECK_STRING(jwt))
    return YU2_WRONG_PARAMETER;

  return doAction<false, HuaweiLoginAction, ACTION_HUAWEI_LOGIN,
                  YuString>
                  (NULL, jwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::huaweiLoginAsync(const char* jwt, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(jwt))
    return YU2_WRONG_PARAMETER;

  return doAction<false, HuaweiLoginAction, ACTION_HUAWEI_LOGIN,
                  YuString>
                  (call, jwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::nvidiaLoginSync(const char* nvidia_jwt)
{
  if (!CHECK_STRING(nvidia_jwt))
    return YU2_WRONG_PARAMETER;

  return doAction<false, NvidiaLoginAction, ACTION_NVIDIA_LOGIN,
                  YuString>
                  (NULL, nvidia_jwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::nvidiaLoginAsync(const char* nvidia_jwt, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(nvidia_jwt))
    return YU2_WRONG_PARAMETER;

  return doAction<false, NvidiaLoginAction, ACTION_NVIDIA_LOGIN,
                  YuString>
                  (call, nvidia_jwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSteamLinkTokenSync(const void* auth_code,
                                                        unsigned auth_code_len,
                                                        int steam_app_id, IYuplay2String** tok)
{
  if (!tok)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  YuMemRange<char> codeRange((const char*)auth_code, auth_code_len);

  Yuplay2Status res = doAction<true, SteamLinkTokenAction, ACTION_STEAM_LINK_TOKEN,
                               YuString, YuMemRange<char>, int>
                              (NULL, yuToken, codeRange, steam_app_id, &ret);

  if (res == YU2_OK)
    *tok = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSteamLinkTokenSync(const void* auth_code,
                                                        unsigned auth_code_len,
                                                        int steam_app_id, eastl::string& tok)
{
  const void* ret = NULL;
  YuMemRange<char> codeRange((const char*)auth_code, auth_code_len);

  Yuplay2Status res = doAction<true, SteamLinkTokenAction, ACTION_STEAM_LINK_TOKEN,
                               YuString, YuMemRange<char>, int>
                              (NULL, yuToken, codeRange, steam_app_id, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    tok = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSteamLinkTokenAsync(const void* auth_code,
                                                        unsigned auth_code_len,
                                                        int steam_app_id, Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  YuMemRange<char> codeRange((const char*)auth_code, auth_code_len);

  return doAction<true, SteamLinkTokenAction, ACTION_STEAM_LINK_TOKEN,
                  YuString, YuMemRange<char>, int>
                 (call, yuToken, codeRange, steam_app_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::linkXboxSync(const char* email, const char* key, const char* sig)
{
  if (!CHECK_STRING(email) || !CHECK_STRING(key))
    return YU2_WRONG_PARAMETER;

  return doAction<false, XboxLinkAction, ACTION_XBOX_LINK,
                  YuString, YuString, YuString>(NULL, email, key, sig, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::linkXboxAsync(const char* email, const char* key,
                                                const char* sig, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(email) || !CHECK_STRING(key))
    return YU2_WRONG_PARAMETER;

  return doAction<false, XboxLinkAction, ACTION_XBOX_LINK,
                  YuString, YuString, YuString>(call, email, key, sig, NULL);
}


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getXboxLinkUrlSync() const
{
  return new YuSessionString(getXboxLinkUrl());
}


//==================================================================================================
#if YU2EASTL_STRING
void YU2VCALL YuSession::getXboxLinkUrlSync(eastl::string& url) const
{
  url = getXboxLinkUrl();
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::tokenLoginSync(const char* token)
{
  if (!CHECK_STRING(token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, TokenLoginAction, ACTION_TOKEN_LOGIN,
                  YuString>
                 (NULL, token, NULL);
}



//==================================================================================================
Yuplay2Status YU2VCALL YuSession::tokenLoginAsync(const char* token, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, TokenLoginAction, ACTION_TOKEN_LOGIN,
                  YuString>
                 (call, token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::stokenLoginSync(const char* token)
{
  if (!CHECK_STRING(token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, StokenLoginAction, ACTION_STOKEN_LOGIN,
                  YuString>
                  (NULL, token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::stokenLoginAsync(const char* token, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, StokenLoginAction, ACTION_STOKEN_LOGIN,
                  YuString>
                 (call, token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::ssoStokenLoginSync(const char* tok)
{
  if (!CHECK_STRING(tok))
    return YU2_WRONG_PARAMETER;

  return doAction<false, SsoStokenLoginAction, ACTION_SSO_STOKEN_LOGIN,
                  YuString>
                 (NULL, tok, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::ssoStokenLoginAsync(const char* tok, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(tok))
    return YU2_WRONG_PARAMETER;

  return doAction<false, SsoStokenLoginAction, ACTION_SSO_STOKEN_LOGIN,
                  YuString>
                 (call, tok, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getWebLoginSessionIdSync(IYuplay2String** session_id)
{
  if (!session_id)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, WebLoginSessionIdAction, ACTION_WEB_LOGIN_SESSION_ID>
                              (NULL, &ret);

  if (res == YU2_OK)
    *session_id = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getWebLoginSessionIdSync(eastl::string& session_id)
{
  const void* ret = NULL;
  Yuplay2Status res = doAction<false, WebLoginSessionIdAction, ACTION_WEB_LOGIN_SESSION_ID>
                              (NULL, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    session_id = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getWebLoginSessionIdAsync(Yuplay2AsyncCall* call)
{
   return doAction<false, WebLoginSessionIdAction, ACTION_WEB_LOGIN_SESSION_ID>
                  (call, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::webLoginSync(const char* session_id)
{
  if (!CHECK_STRING(session_id))
    return YU2_WRONG_PARAMETER;

  stopAction(ACTION_WEB_LOGIN);

  return doAction<false, WebLoginAction, ACTION_WEB_LOGIN,
                  YuString>
                 (NULL, session_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::webLoginAsync(const char* session_id, Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(session_id))
    return YU2_WRONG_PARAMETER;

  stopAction(ACTION_WEB_LOGIN);

  return doAction<false, WebLoginAction, ACTION_WEB_LOGIN,
                  YuString>
                 (call, session_id, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::kongZhongLoginSync(const char* kz_token)
{
  if (!CHECK_STRING(kz_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, KongZhongLoginAction, ACTION_KONGZHONG_LOGIN,
                  YuString>
                 (NULL, kz_token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::kongZhongLoginAsync(const char* kz_token,
                                                      Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(kz_token))
    return YU2_WRONG_PARAMETER;

  return doAction<false, KongZhongLoginAction, ACTION_KONGZHONG_LOGIN,
                  YuString>
                 (call, kz_token, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::tencentLoginSync()
{
  return doAction<false, TencentLoginAction, ACTION_TENCENT_LOGIN>
                 (NULL, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::tencentLoginAsync(Yuplay2AsyncCall* call)
{
  return doAction<false, TencentLoginAction, ACTION_TENCENT_LOGIN>
                 (call, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::wegameLoginSync(uint64_t wegame_id, const char* ticket,
                                                  const char* nick)
{
  if (!CHECK_STRING(ticket))
    return YU2_WRONG_PARAMETER;

  return doAction<false, WegameLoginAction, ACTION_WEGAME_LOGIN,
                  uint64_t, YuString, YuString>
                 (NULL, wegame_id, ticket, nick ? nick : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::wegameLoginAsync(uint64_t wegame_id, const char* ticket,
                                                   Yuplay2AsyncCall* call, const char* nick)
{
  if (!call || !CHECK_STRING(ticket))
    return YU2_WRONG_PARAMETER;

  return doAction<false, WegameLoginAction, ACTION_WEGAME_LOGIN,
                  uint64_t, YuString, YuString>
                 (call, wegame_id, ticket, nick ? nick : "", NULL);
}


//==================================================================================================
void YuSession::onLogin(const YuString& login, const YuString& nick, const YuString& user_id,
                        const YuString& token, const YuString& jwt, const YuString& tags,
                        const YuString& lang, const YuString& country, int64_t exp_time,
                        const YuString& nickorig, const YuString& nicksfx, const YuStrMap& meta_map)
{
  YuWarden warden(sessionLock);

  stateBits |= YU2_SESSION_LOGGED_IN;

  yuLogin = login;
  yuNick = nick;
  yuNickorig = nickorig;
  yuNicksfx = nicksfx;
  userId = user_id;
  yuToken = token;
  yuJwt = jwt;
  yuTags = tags;
  yuLang = lang;
  yuCountryCode = country;
  tokenExpTime = exp_time;
  tokenLifeTime = exp_time;
  meta = meta_map;

#if YU2_WINAPI
  tokenExpTime += ::_time64(NULL);
#else
  tokenExpTime += ::time(NULL);
#endif //YU2_WINAPI
}


//==================================================================================================
void YuSession::onTwoStepResponse(const YuString& req_id, const YuString& user_id,
                                  bool gp_totp, bool email_totp, bool wt_totp)
{
  twoStepRequestId = req_id;
  twoStepUserId = user_id;
  hasGaijinPassTwoStepType = gp_totp;
  hasEmailTwoStepType = email_totp;
  hasWTAssistantTwoStepType = wt_totp;
}


//==================================================================================================
void YuSession::onNewToken(const YuString& token, int64_t exp_time)
{
  YuWarden warden(sessionLock);

  yuToken = token;
  this->tokenExpTime = exp_time;
  this->tokenLifeTime = exp_time;

#if YU2_WINAPI
  this->tokenExpTime += ::_time64(NULL);
#else
  this->tokenExpTime += ::time(NULL);
#endif //YU2_WINAPI
}


//==================================================================================================
void YuSession::onNewJwt(const YuString& jwt)
{
  YuWarden warden(sessionLock);
  yuJwt = jwt;
}


//==================================================================================================
void YuSession::onNewTags(const YuString& tags)
{
  YuWarden warden(sessionLock);
  yuTags = tags;
}


//==================================================================================================
void YuSession::onTencentZoneId(unsigned zone_id)
{
  YuWarden warden(sessionLock);
  tencentZoneId = zone_id;
}


//==================================================================================================
void YuSession::onTencentAppData(const YuString &data)
{
  YuWarden warden(sessionLock);
  tencentAppData = data;
}


//==================================================================================================
void YuSession::onTencentPaymentConf(float gold_rate)
{
  YuWarden warden(sessionLock);
  tencentGoldRate = gold_rate;
}


//==================================================================================================
void YuSession::onNswitchLogin(int eshop_code, const YuString& eshop_msg)
{
  YuWarden warden(sessionLock);

  nintendoErrCode = eshop_code;
  nintendoErrText = eshop_msg;
}


//==================================================================================================
void YuSession::onPsnLogin(bool psn_plus)
{
  YuWarden warden(sessionLock);
  psnPlusOn = psn_plus;
}


//==================================================================================================
void YuSession::onXboxLogin(bool game_pass)
{
  YuWarden warden(sessionLock);
  gamePassOn = game_pass;
}


//==================================================================================================
void YuSession::onKongZhongLogin(int fcm)
{
  YuWarden warden(sessionLock);
  kzFcm = fcm;
}


//==================================================================================================
void YuSession::onKongZhongLoginFail(const YuString& msg)
{
  YuWarden warden(sessionLock);
  kzMsg = msg;
}


//==================================================================================================
void YuSession::onWegameLogin(uint64_t wegame_id)
{
  wegameId = wegame_id;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getUserInfoSync(IYuplay2UserInfo** user_info)
{
  if (!user_info)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res =
    doAction<true, BasicUserinfoAction, ACTION_USER_INFO,
             YuString, YuString, YuString, YuString, YuString, YuString, YuString, YuStrMap>
            (NULL, userId, yuLogin, yuNick, yuToken, yuTags, yuNickorig, yuNicksfx, meta, &ret);

  if (res == YU2_OK)
    *user_info = (IYuplay2UserInfo*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getUserInfoAsync(Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, BasicUserinfoAction, ACTION_USER_INFO,
                  YuString, YuString, YuString, YuString, YuString, YuString, YuString, YuStrMap>
                 (call, userId, yuLogin, yuNick, yuToken, yuTags, yuNickorig, yuNicksfx, meta, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getBasicUserInfoSync(IYuplay2UserInfo** user_info)
{
  if (!user_info)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res =
    doAction<true, BasicUserinfoAction, ACTION_BASIC_USER_INFO,
             YuString, YuString, YuString, YuString, YuString, YuString, YuString, YuStrMap>
            (NULL, userId, yuLogin, yuNick, yuToken, yuTags, yuNickorig, yuNicksfx, meta, &ret);

  if (res == YU2_OK)
    *user_info = (IYuplay2UserInfo*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getBasicUserInfoAsync(Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, BasicUserinfoAction, ACTION_BASIC_USER_INFO,
                  YuString, YuString, YuString, YuString, YuString, YuString, YuString, YuStrMap>
                 (call, userId, yuLogin, yuNick, yuToken, yuTags, yuNickorig, yuNicksfx, meta, NULL);
}


//==================================================================================================
bool YU2VCALL YuSession::checkActivationCode(const char* code)
{
  static const char* alphaNum = "ABCDEFGHJKLMNPQRSTUWXYZ123456789";

  if (!CHECK_STRING(code))
    return false;

  for (const char* c = code; *c; ++c)
    if (!isalpha(*c) && !isdigit(*c) && *c != '-')
      return false;

  if (const char* minus = strrchr(code, '-'))
  {
    YuString pureCode(code, minus);
    YuString checkSum(minus + 1);
    YuString calcSum;

    pureCode.make_upper();
    checkSum.make_upper();

    eastl::vector<unsigned char> md5buf(MD5_DIGEST_LENGTH, 0);
    ::MD5((const unsigned char*)pureCode.c_str(), pureCode.length(), md5buf.data());

    int intSum = md5buf[3] | (md5buf[2] << 8) | (md5buf[1] << 16) | (md5buf[0] << 24);

    for (int i = 0; i < 5; ++i, intSum >>= 5)
      calcSum += alphaNum[intSum & 0x1F];

    return checkSum == calcSum;
  }

  return false;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getPurchasesCountSync(const char* guid, int* count)
{
  if (!CHECK_STRING(guid) || !count)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, PurchCountAction, ACTION_PURCH_CNT,
                               YuString, YuString>
                              (NULL, yuToken, guid, &ret);

  if (res == YU2_OK)
    *count = *((int*)ret);

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getPurchasesCountAsync(const char* guid, Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, PurchCountAction, ACTION_PURCH_CNT,
                  YuString, YuString>
                  (call, yuToken, guid, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getMultiPurchasesCountSync(const char** guids,
                                                             unsigned guids_count,
                                                             IYuplay2ItemPurchases** purchases)
{
  if (!guids || !guids_count || !purchases)
    return YU2_WRONG_PARAMETER;

  YuStringSet checkGuids;

  for (unsigned i = 0; i < guids_count; ++i)
    if (CHECK_STRING(guids[i]))
      checkGuids.insert(guids[i]);
    else
      return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, MultiPurchCountAction, ACTION_MULTI_PURCH_CNT,
                               YuString, YuStringSet>
                              (NULL, yuToken, checkGuids, &ret);

  if (res == YU2_OK)
    *purchases = (IYuplay2ItemPurchases*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getMultiPurchasesCountAsync(const char** guids,
                                                              unsigned guids_count,
                                                              Yuplay2AsyncCall* call)
{
  if (!guids || !guids_count || !call)
    return YU2_WRONG_PARAMETER;

  YuStringSet checkGuids;

  for (unsigned i = 0; i < guids_count; ++i)
    if (CHECK_STRING(guids[i]))
      checkGuids.insert(guids[i]);
    else
      return YU2_WRONG_PARAMETER;

  return doAction<true, MultiPurchCountAction, ACTION_MULTI_PURCH_CNT,
                  YuString, YuStringSet>
                  (call, yuToken, checkGuids, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::buyItemSync(const char* guid, Yuplay2PaymentMethods payment)
{
  if (!CHECK_STRING(guid))
    return YU2_WRONG_PARAMETER;

  return doAction<true, PurchaseAction, ACTION_BUY_ITEM,
                  YuString, YuString, Yuplay2PaymentMethods>
                 (NULL, yuToken, guid, payment, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::buyItemAsync(const char* guid, Yuplay2PaymentMethods payment,
                                               Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, PurchaseAction, ACTION_BUY_ITEM,
                  YuString, YuString, Yuplay2PaymentMethods>
                 (call, yuToken, guid, payment, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::activateItemSync(const char* code)
{
  if (!CHECK_STRING(code))
    return YU2_WRONG_PARAMETER;

  return doAction<true, ActivateAction, ACTION_ACTIVATE,
                  YuString, YuString>
                 (NULL, yuToken, code, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::activateItemAsync(const char* code, Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(code) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, ActivateAction, ACTION_ACTIVATE,
                  YuString, YuString>
                 (call, yuToken, code, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getItemInfoSync(const char* guid, IYuplay2ItemInfo** info)
{
  if (!CHECK_STRING(guid) || !info)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, ItemInfoAction, ACTION_ITEM_INFO,
                               YuString, YuString>
                              (NULL, guid, yuToken, &ret);

  if (res == YU2_OK)
    *info = (IYuplay2ItemInfo*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getItemInfoAsync(const char* guid, Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<false, ItemInfoAction, ACTION_ITEM_INFO,
                  YuString, YuString>
                 (call, guid, yuToken, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::openItemPageSync(const char* guid)
{
  if (!CHECK_STRING(guid))
    return YU2_WRONG_PARAMETER;

  return doAction<false, OpenItemPageAction, ACTION_OPEN_ITEM_PAGE,
                  YuString, YuString>
                 (NULL, guid, yuToken, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::openItemPageAsync(const char* guid, Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<false, OpenItemPageAction, ACTION_OPEN_ITEM_PAGE,
                  YuString, YuString>
                 (call, guid, yuToken, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getMultiItemsInfoSync(const char** guids, unsigned guids_count,
                                                        IYuplay2ItemsInfo** info)
{
  if (!guids || !guids_count || !info)
    return YU2_WRONG_PARAMETER;

  YuStringSet infoGuids;

  for (unsigned i = 0; i < guids_count; ++i)
    if (CHECK_STRING(guids[i]))
      infoGuids.insert(guids[i]);
    else
      return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, MultiItemInfoAction, ACTION_MULTI_ITEM_INFO,
                               YuStringSet, YuString>
                              (NULL, infoGuids, yuToken, &ret);

  if (res == YU2_OK)
    *info = (IYuplay2ItemsInfo*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getMultiItemsInfoAsync(const char** guids, unsigned guids_count,
                                                         Yuplay2AsyncCall* call)
{
  if (!guids || !guids_count || !call)
    return YU2_WRONG_PARAMETER;

  YuStringSet infoGuids;

  for (unsigned i = 0; i < guids_count; ++i)
    if (CHECK_STRING(guids[i]))
      infoGuids.insert(guids[i]);
    else
      return YU2_WRONG_PARAMETER;

  return doAction<false, MultiItemInfoAction, ACTION_MULTI_ITEM_INFO,
                  YuStringSet, YuString>
                 (call, infoGuids, yuToken, NULL);
}


//==================================================================================================
bool YuSession::getDescFilePath(const YuString& folder, const YuString& guid, YuPath& path) const
{
  path = folder;

  path.append(guid);
  path.setExt("yup");

  return YuFs::fileExists(path);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::checkVersionAsync(const char* folder, const char* guid,
                                                    Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  YuPath torrent;
  if (!getDescFilePath(folder, guid, torrent))
    return YU2_NOT_FOUND;

  return doAction<false, CheckVersionAction, ACTION_CHECK_VERSION,
                  YuPath, YuString, YuString>
                 (call, torrent, guid, "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::checkVersionSync(const char* folder, const char* guid,
                                                   bool* new_version)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !new_version)
    return YU2_WRONG_PARAMETER;

  YuPath torrent;
  if (!getDescFilePath(folder, guid, torrent))
    return YU2_NOT_FOUND;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, CheckVersionAction, ACTION_CHECK_VERSION,
                               YuPath, YuString, YuString>
                              (NULL, torrent, guid, "", &ret);

  if (res == YU2_OK)
    *new_version = (ret != NULL);

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::checkTagVersionAsync(const char* folder, const char* guid,
                                                       const char* tag, Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !CHECK_STRING(tag) || !call)
    return YU2_WRONG_PARAMETER;

  YuPath torrent;
  if (!getDescFilePath(folder, guid, torrent))
    return YU2_NOT_FOUND;

  return doAction<false, CheckVersionAction, ACTION_CHECK_VERSION,
                  YuPath, YuString, YuString>
                 (call, torrent, guid, tag, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::checkTagVersionSync(const char* folder, const char* guid,
                                                      const char* tag, bool* new_version)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !CHECK_STRING(tag) || !new_version)
    return YU2_WRONG_PARAMETER;

  YuPath torrent;
  if (!getDescFilePath(folder, guid, torrent))
    return YU2_NOT_FOUND;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, CheckVersionAction, ACTION_CHECK_VERSION,
                               YuPath, YuString, YuString>
                              (NULL, torrent, guid, tag, &ret);

  if (res == YU2_OK)
    *new_version = (ret != NULL);

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getLocalVersionAsync(const char* folder,
                                                       const char* guid,
                                                       Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  YuPath torrent;
  if (!getDescFilePath(folder, guid, torrent))
    return YU2_NOT_FOUND;

  return doAction<false, GetYupVersionAction, ACTION_GET_YUP_VERSION,
                  YuPath>
                 (call, torrent, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getLocalVersionSync(const char* folder,
                                                      const char* guid,
                                                      IYuplay2Binary** version)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !version)
    return YU2_WRONG_PARAMETER;

  YuPath torrent;
  if (!getDescFilePath(folder, guid, torrent))
    return YU2_NOT_FOUND;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetYupVersionAction, ACTION_GET_YUP_VERSION,
                               YuPath>
                              (NULL, torrent, &ret);

  if (res == YU2_OK)
    *version = (IYuplay2Binary*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getCurrentYupAsync(const char* guid,
                                                     Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<false, GetYupAction, ACTION_GET_YUP,
                  YuString, YuString>
                 (call, guid, "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getCurrentYupSync(const char* guid,
                                                    IYuplay2Binary** yup_file)
{
  if (!CHECK_STRING(guid) || !yup_file)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetYupAction, ACTION_GET_YUP,
                               YuString, YuString>
                              (NULL, guid, "", &ret);

  if (res == YU2_OK)
    *yup_file = (IYuplay2Binary*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTagCurrentYupAsync(const char* guid, const char* tag,
                                                        Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !CHECK_STRING(tag) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<false, GetYupAction, ACTION_GET_YUP,
                  YuString, YuString>
                 (call, guid, tag, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTagCurrentYupSync(const char* guid, const char* tag,
                                                       IYuplay2Binary** yup_file)
{
  if (!CHECK_STRING(guid) || !CHECK_STRING(tag) || !yup_file)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetYupAction, ACTION_GET_YUP,
                               YuString, YuString>
                              (NULL, guid, tag, &ret);

  if (res == YU2_OK)
    *yup_file = (IYuplay2Binary*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::checkNewerYupAsync(const char* folder,
                                                     const char* guid,
                                                     const char* tag,
                                                     Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid))
    return YU2_WRONG_PARAMETER;

  YuPath yupPath;
  getDescFilePath(folder, guid, yupPath);

  return doAction<false, CheckNewerYupAction, ACTION_CHECK_NEWER_YUP,
                  YuPath, YuString, YuString>
                  (call, yupPath, guid, tag ? tag : "", NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::checkNewerYupYupSync(const char* folder,
                                                       const char* guid,
                                                       const char* tag,
                                                       IYuplay2Binary** yup_file)
{
  if (!CHECK_STRING(folder) || !CHECK_STRING(guid) || !yup_file)
    return YU2_WRONG_PARAMETER;

  YuPath yupPath;
  getDescFilePath(folder, guid, yupPath);

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, CheckNewerYupAction, ACTION_CHECK_NEWER_YUP,
                               YuPath, YuString, YuString>
                              (NULL, yupPath, guid, tag ? tag : "", &ret);

  if (res == YU2_OK)
    *yup_file = (IYuplay2Binary*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getRemoteVersionAsync(const char* guid,
                                                        const char* tag,
                                                        Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(guid) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<false, GetVersionAction, ACTION_GET_REMOTE_VERSION,
                  YuString, YuString, unsigned, unsigned>
                  (call, guid, tag ? tag : "", versionConnTimeout, versionReqTimeout, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getRemoteVersionSync(const char* guid,
                                                       const char* tag,
                                                       IYuplay2String** version)
{
  if (!CHECK_STRING(guid) || !version)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetVersionAction, ACTION_GET_REMOTE_VERSION,
                               YuString, YuString, unsigned, unsigned>
                              (NULL, guid, tag ? tag : "", versionConnTimeout, versionReqTimeout,
                               &ret);

  if (res == YU2_OK)
    *version = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getRemoteVersionSync(const char* guid,
                                                       const char* tag,
                                                       eastl::string& version)
{
  if (!CHECK_STRING(guid))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetVersionAction, ACTION_GET_REMOTE_VERSION,
                               YuString, YuString, unsigned, unsigned>
                              (NULL, guid, tag ? tag : "", versionConnTimeout, versionReqTimeout,
                               &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    version = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::buyTencentGoldAsync(unsigned gold, Yuplay2AsyncCall* call)
{
  if (!gold || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, TencentBuyGoldAction, ACTION_TENCENT_BUY_GOLD,
                  YuString, unsigned>
                  (call, yuToken, gold, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::buyTencentGoldSync(unsigned gold)
{
  if (!gold)
    return YU2_WRONG_PARAMETER;

  return doAction<true, TencentBuyGoldAction, ACTION_TENCENT_BUY_GOLD,
                  YuString, unsigned>
                 (NULL, yuToken, gold, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTencentGoldRateAsync(Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  return doAction<false, TencentPaymentConfAction, ACTION_TENCENT_PAYMENT_CONF>
                  (call, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTencentGoldRateSync(float* rate)
{
  if (!rate)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, TencentPaymentConfAction, ACTION_TENCENT_PAYMENT_CONF>
                              (NULL, &ret);

  if (res == YU2_OK)
    *rate = *((float*)ret);

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyGooglePlayPurchaseAsync(const char* purchase_json,
                                                                bool do_acknowledge,
                                                                Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(purchase_json))
    return YU2_WRONG_PARAMETER;

  return doAction<true, GooglePurchaseAction, ACTION_GOOGLE_PURCHASE,
                  YuString, YuString, bool>
                  (call, yuToken, purchase_json, do_acknowledge, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyGooglePlayPurchaseSync(const char* purchase_json,
                                                               bool do_acknowledge,
                                                               IYuplay2GooglePurchaseInfo** info)
{
  if (!CHECK_STRING(purchase_json))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, GooglePurchaseAction, ACTION_GOOGLE_PURCHASE,
                               YuString, YuString, bool>
                               (NULL, yuToken, purchase_json, do_acknowledge, &ret);

  if (res == YU2_OK)
    *info = (IYuplay2GooglePurchaseInfo*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyHuaweiPurchaseAsync(const char* purchase_json,
                                                            bool do_acknowledge,
                                                            Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(purchase_json))
    return YU2_WRONG_PARAMETER;

  return doAction<true, HuaweiPurchaseAction, ACTION_HUAWEI_PURCHASE,
                  YuString, YuString, bool>
                  (call, yuToken, purchase_json, do_acknowledge, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyHuaweiPurchaseSync(const char* purchase_json,
                                                           bool do_acknowledge,
                                                           IYuplay2GooglePurchaseInfo** info)
{
  if (!CHECK_STRING(purchase_json))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, HuaweiPurchaseAction, ACTION_HUAWEI_PURCHASE,
                               YuString, YuString, bool>
                               (NULL, yuToken, purchase_json, do_acknowledge, &ret);

  if (res == YU2_OK)
    *info = (IYuplay2GooglePurchaseInfo*)ret;

  return res;
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyApplePurchaseAsync(const char* receipt_json,
                                                           Yuplay2AsyncCall* call)
{
  if (!call || !CHECK_STRING(receipt_json))
    return YU2_WRONG_PARAMETER;

  return doAction<true, ApplePurchaseAction, ACTION_APPLE_PURCHASE,
                  YuString, YuString>
                  (call, yuToken, receipt_json, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyApplePurchaseSync(const char* receipt_json,
                                                          IYuplay2String** apple_trans_id)
{
  if (!CHECK_STRING(receipt_json))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, ApplePurchaseAction, ACTION_APPLE_PURCHASE,
                               YuString, YuString>
                              (NULL, yuToken, receipt_json, &ret);

  if (res == YU2_OK)
    *apple_trans_id = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::notifyApplePurchaseSync(const char* receipt_json,
                                                          eastl::string& apple_trans_id)
{
  if (!CHECK_STRING(receipt_json))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, ApplePurchaseAction, ACTION_APPLE_PURCHASE,
                               YuString, YuString>
                              (NULL, yuToken, receipt_json, &ret);

  if (res == YU2_OK && ret)
  {
    YuSessionString* str = (YuSessionString*)ret;
    apple_trans_id = str->get(); //-V522
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::addUserTagSync(const char* tag)
{
  if (!CHECK_STRING(tag))
    return YU2_WRONG_PARAMETER;

  YuStringSet tags;
  tags.insert(tag);

  return doAction<true, TagUserAction, ACTION_TAG_USER,
                  YuString, YuStringSet>
                 (NULL, yuToken, tags, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::addUserTagAsync(const char* tag,
                                                 Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(tag) || !call)
    return YU2_WRONG_PARAMETER;

  YuStringSet tags;
  tags.insert(tag);

  return doAction<true, TagUserAction, ACTION_TAG_USER,
                  YuString, YuStringSet>
                 (call, yuToken, tags, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::addUserTagsSync(const char** tgs,
                                                  unsigned count)
{
  if (!tgs || !count)
    return YU2_WRONG_PARAMETER;

  YuStringSet tags;

  for (unsigned i = 0; i < count; ++i)
  {
    if (CHECK_STRING(tgs[i]))
      tags.insert(tgs[i]);
    else
      return YU2_WRONG_PARAMETER;
  }

  return doAction<true, TagUserAction, ACTION_TAG_USER,
                  YuString, YuStringSet>
                 (NULL, yuToken, tags, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::addUserTagsAsync(const char** tgs,
                                                   unsigned count,
                                                   Yuplay2AsyncCall* call)
{
  if (!tgs || !count || !call)
    return YU2_WRONG_PARAMETER;

  YuStringSet tags;

  for (unsigned i = 0; i < count; ++i)
  {
    if (CHECK_STRING(tgs[i]))
      tags.insert(tgs[i]);
    else
      return YU2_WRONG_PARAMETER;
  }

  return doAction<true, TagUserAction, ACTION_TAG_USER,
                  YuString, YuStringSet>
                 (call, yuToken, tags, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::renewUserTokenSync()
{
  return doAction<true, RenewTokenAction, ACTION_RENEW_TOKEN,
                  YuString>
                 (NULL, yuToken, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::renewUserTokenAsync(Yuplay2AsyncCall* call)
{
  return doAction<true, RenewTokenAction, ACTION_RENEW_TOKEN,
                  YuString>
                 (call, yuToken, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::renewUserJwtSync()
{
  if (yuJwt.empty())
    return YU2_NOT_LOGGED_IN;

  return doAction<true, RenewJwtAction, ACTION_RENEW_JWT,
                  YuString>
                 (NULL, yuJwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::renewUserJwtAsync(Yuplay2AsyncCall* call)
{
  if (yuJwt.empty())
    return YU2_NOT_LOGGED_IN;

  return doAction<true, RenewJwtAction, ACTION_RENEW_JWT,
                  YuString>
                 (call, yuJwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTwoStepCodeSync(IYuplay2String** code)
{
  if (!code)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetTwoStepCodeAction, ACTION_GET_TWO_STEP_CODE>
                              (NULL, &ret);

  if (res == YU2_OK)
    *code = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTwoStepCodeSync(eastl::string& code)
{
  const void* ret = NULL;
  Yuplay2Status res = doAction<false, GetTwoStepCodeAction, ACTION_GET_TWO_STEP_CODE>
                              (NULL, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    code = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getTwoStepCodeAsync(Yuplay2AsyncCall* call)
{
   return doAction<false, GetTwoStepCodeAction, ACTION_GET_TWO_STEP_CODE>
                  (call, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getLoggedUrlSync(const char* url, IYuplay2String** logged_url,
                                                   const char* host)
{
  if (!CHECK_STRING(url) || !logged_url)
    return YU2_WRONG_PARAMETER;

  YuString baseHost(host ? host : "");

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, LoggedUrlAction, ACTION_LOGGED_URL,
                               YuString, YuString, YuString>
                              (NULL, yuToken, url, baseHost, &ret);

  if (res == YU2_OK)
    *logged_url = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getLoggedUrlSync(const char* url, eastl::string& logged_url,
                                                   const char* host)
{
  if (!CHECK_STRING(url))
    return YU2_WRONG_PARAMETER;

  YuString baseHost(host ? host : "");

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, LoggedUrlAction, ACTION_LOGGED_URL,
                               YuString, YuString, YuString>
                              (NULL, yuToken, url, baseHost, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    logged_url = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getLoggedUrlAsync(const char* url, Yuplay2AsyncCall* call,
                                                    const char* host)
{
  if (!CHECK_STRING(url) || !call)
    return YU2_WRONG_PARAMETER;

  YuString baseHost(host ? host : "");

  return doAction<true, LoggedUrlAction, ACTION_LOGGED_URL,
                  YuString, YuString, YuString>
                  (call, yuToken, url, baseHost, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSsoLoggedUrlSync(const char* url, IYuplay2String** logged_url,
                                                      const char* host, const char* service)
{
  if (!CHECK_STRING(url) || !logged_url)
    return YU2_WRONG_PARAMETER;

  YuString baseHost(host ? host : "");
  YuString serviceName(service ? service : "");

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, SsoLoggedUrlAction, ACTION_SSO_LOGGED_URL,
                               YuString, YuString, YuString, YuString>
                              (NULL, yuToken, url, baseHost, serviceName, &ret);

  if (res == YU2_OK)
    *logged_url = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSsoLoggedUrlSync(const char* url, eastl::string& logged_url,
                                                      const char* host, const char* service)
{
  if (!CHECK_STRING(url))
    return YU2_WRONG_PARAMETER;

  YuString baseHost(host ? host : "");
  YuString serviceName(service ? service : "");

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, SsoLoggedUrlAction, ACTION_SSO_LOGGED_URL,
                               YuString, YuString, YuString, YuString>
                              (NULL, yuToken, url, baseHost, serviceName, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    logged_url = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSsoLoggedUrlAsync(const char* url, Yuplay2AsyncCall* call,
                                                       const char* host, const char* service)
{
  if (!CHECK_STRING(url) || !call)
    return YU2_WRONG_PARAMETER;

  YuString baseHost(host ? host : "");
  YuString serviceName(service ? service : "");

  return doAction<true, SsoLoggedUrlAction, ACTION_SSO_LOGGED_URL,
                  YuString, YuString, YuString, YuString>
                  (call, yuToken, url, baseHost, serviceName, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getKongZhongLoggedUrlSync(const char* url,
                                                            IYuplay2String** logged_url)
{
  if (!CHECK_STRING(url))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, KongZhongLoggedUrlAction, ACTION_KONGZHONG_LOGGED_URL,
                               YuString, uint64_t, YuString>
                              (NULL, yuLogin, wegameId, url, &ret);

  if (res == YU2_OK)
    *logged_url = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getKongZhongLoggedUrlSync(const char* url,
                                                            eastl::string& logged_url)
{
  if (!CHECK_STRING(url))
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, KongZhongLoggedUrlAction, ACTION_KONGZHONG_LOGGED_URL,
                               YuString, uint64_t, YuString>
                              (NULL, yuLogin, wegameId, url, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    logged_url = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getKongZhongLoggedUrlAsync(const char* url,
                                                             Yuplay2AsyncCall* call)
{
  if (!CHECK_STRING(url) || !call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, KongZhongLoggedUrlAction, ACTION_KONGZHONG_LOGGED_URL,
                  YuString, uint64_t, YuString>
                  (call, yuLogin, wegameId, url, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSsoShortTokenSync(IYuplay2String** ret_short_token)
{
  if (!ret_short_token)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, SsoGetShortTokenAction, ACTION_SSO_GET_SHORT_TOKEN, YuString>
                              (NULL, yuToken, &ret);

  if (res == YU2_OK)
    *ret_short_token = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSsoShortTokenSync(eastl::string& ret_short_token)
{
  const void* ret = NULL;
  Yuplay2Status res = doAction<true, SsoGetShortTokenAction, ACTION_SSO_GET_SHORT_TOKEN, YuString>
                              (NULL, yuToken, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    ret_short_token = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getSsoShortTokenAsync(Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, SsoGetShortTokenAction, ACTION_SSO_GET_SHORT_TOKEN, YuString>
                 (call, yuToken, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getUserStokenSync(IYuplay2String** stoken)
{
  if (!stoken)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, GetStokenAction, ACTION_GET_STOKEN,
                               YuString, YuString>
                              (NULL, yuToken, yuJwt, &ret);

  if (res == YU2_OK)
    *stoken = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getUserStokenSync(eastl::string& stoken)
{
  const void* ret = NULL;
  Yuplay2Status res = doAction<true, GetStokenAction, ACTION_GET_STOKEN,
                               YuString, YuString>
                              (NULL, yuToken, yuJwt, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    stoken = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getUserStokenAsync(Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, GetStokenAction, ACTION_GET_STOKEN,
                  YuString, YuString>
                 (call, yuToken, yuJwt, NULL);
}


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getOpenIdJwtSync(IYuplay2String** openid_jwt)
{
  if (!openid_jwt)
    return YU2_WRONG_PARAMETER;

  const void* ret = NULL;
  Yuplay2Status res = doAction<true, GetOpenIdJwtAction, ACTION_GET_OPENID_JWT,
                               YuString>
                              (NULL, yuJwt, &ret);

  if (res == YU2_OK)
    *openid_jwt = (IYuplay2String*)ret;

  return res;
}


#if YU2EASTL_STRING
//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getOpenIdJwtSync(eastl::string& openid_jwt)
{
  const void* ret = NULL;
  Yuplay2Status res = doAction<true, GetOpenIdJwtAction, ACTION_GET_OPENID_JWT,
                               YuString>
                              (NULL, yuJwt, &ret);

  if (res == YU2_OK)
  {
    YuSessionString* str = (YuSessionString*)ret;
    openid_jwt = str->get();
    delete str;
  }

  return res;
}
#endif //YU2EASTL_STRING


//==================================================================================================
Yuplay2Status YU2VCALL YuSession::getOpenIdJwtAsync(Yuplay2AsyncCall* call)
{
  if (!call)
    return YU2_WRONG_PARAMETER;

  return doAction<true, GetOpenIdJwtAction, ACTION_GET_OPENID_JWT,
                  YuString>
                 (call, yuJwt, NULL);
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getDistrSync(eastl::string& distr) const
{
  YuWarden warden(sessionLock);
  distr = getDistr();
}
#endif //YU2EASTL_STRING


//==================================================================================================
int YU2VCALL YuSession::getNswitchEshopErrorCodeSync() const
{
  YuWarden warden(sessionLock);
  return nintendoErrCode;
}


//==================================================================================================
IYuplay2String* YU2VCALL YuSession::getNswitchEshopErrorMessageSync() const
{
  YuWarden warden(sessionLock);
  return new YuSessionString(nintendoErrText);
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getNswitchEshopErrorMessageSync(eastl::string& err_text) const
{
  YuWarden warden(sessionLock);
  err_text = nintendoErrText;
}
#endif //YU2EASTL_STRING


//==================================================================================================
bool YU2VCALL YuSession::getPsnPlusStatusSync() const
{
  YuWarden warden(sessionLock);
  return psnPlusOn;
}


//==================================================================================================
bool YU2VCALL YuSession::getGamePassStatusSync() const
{
  YuWarden warden(sessionLock);
  return gamePassOn;
}


//==================================================================================================
int YU2VCALL YuSession::getKongZhongFcmSync() const
{
  YuWarden warden(sessionLock);
  return kzFcm;
}


#if YU2EASTL_STRING
//==================================================================================================
void YU2VCALL YuSession::getKongZhongMsgSync(eastl::string& msg_text) const
{
  YuWarden warden(sessionLock);
  msg_text = kzMsg;
}


//==================================================================================================
void YU2VCALL YuSession::getKongZhongAccountIdSync(eastl::string& accountid) const
{
  YuWarden warden(sessionLock);

  //We do not let client to get login. But KongZhong "login" is accountid, not e-mail
  //It is safe to get such login in client. But we must be sure it is valid KongZhong login
  if (kzFcm >= 0)
    accountid = yuLogin;
}
#endif //YU2EASTL_STRING
