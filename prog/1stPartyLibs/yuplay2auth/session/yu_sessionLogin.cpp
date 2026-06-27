#include "stdafx.h"
#include "yu_sessionLogin.h"
#include "yu_debug.h"
#include "yu_string.h"
#include "yu_system.h"
#include "yu_fs.h"
#include "yu_url.h"
#include "yu_sysHandle.h"

#include <time.h>

#if YU2_WINDOWS
#include <tchar.h>
#include <TCLSProxy/tclsproxydef.h>
#include <TCLSProxy/TenTenioProxyHelp.h>
#endif //YU2_WINDOWS

#ifdef YUPLAY2_USE_GNOME
#include <gtk/gtk.h>
#endif //YUPLAY2_USE_GNOME


//==================================================================================================
bool YuSession::LoginAction::run(const YuString& login, const YuString& pass, bool auth_client,
                                 const YuString& game_id)
{
  YuStrMap post({
    { "login", login },
    { "password", pass },
    { "v", "2" },
  });

  if (!session.getAppId())
    post["nojwt"] = "1";

  if (auth_client)
    post["client"] = YuSystem::getSystemId();
  else
    post["declient"] = YuSystem::getSystemId();

  session.onTwoStepResponse("", "", false, false, false);

  return runLoginAction(session.getLoginUrl(), post, game_id);
}


//==================================================================================================
Yuplay2Status YuSession::LoginAction::parseServerAnswer(const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_2STEP_AUTH)
  {
    YuString userId = answer.at("userId").s();
    YuString requestId = answer.at("requestId").s();

    bool gpTotp = answer.at("hasGjPass").b();
    bool emailTotp = answer.at("hasTwoStepEmail").b();
    bool wtTotp = answer.at("hasWTR").b();

    session.onTwoStepResponse(requestId, userId, gpTotp, emailTotp, wtTotp);

    return status;
  }

  return CommonLoginAction::parseServerAnswer(data);
}


//==================================================================================================
void YuSession::LoginAction::onOkStatus(const YuJson& json)
{
  //Kongzhong auth sometimes use login/password sent to specific KongZhong URL,
  //and we should check its answer for "fcm" (common login does not add "fcm" to JSON)
  session.onKongZhongLogin(json.in("fcm") ? (int)json.at("fcm").i() : -1);
}


//==================================================================================================
Yuplay2Status YuSession::LoginAction::onErrorStatus(Yuplay2Status status)
{
  //Kongzhong auth sometimes use login/password sent to specific KongZhong URL,
  //and we should check its LGINERROR answer for "msg" (common login does not add "msg" to JSON)
  if (status == YU2_WRONG_LOGIN && answer.in("msg"))
    session.onKongZhongLoginFail(answer.at("msg").s());

  return CommonLoginAction::onErrorStatus(status);
}


//==================================================================================================
bool YuSession::TwoStepLoginAction::run(const YuString& login, const YuString& pass,
                                        const YuString& code, bool auth_client)
{
  YuStrMap post({
    { "login", login },
    { "password", pass },
    { "2step", code },
  });

  if (!session.getAppId())
    post["nojwt"] = "1";

  if (auth_client)
    post["client"] = YuSystem::getSystemId();
  else
    post["declient"] = YuSystem::getSystemId();

  return runLoginAction(session.getLoginUrl(), post);
}


//==================================================================================================
bool YuSession::TokenLoginAction::run(const YuString& token)
{
  YuStrMap post({
    { "token", token }
  });

  return runLoginAction(session.getTokenUrl(), post, false);
}


//==================================================================================================
Yuplay2Status YuSession::TokenLoginAction::onErrorStatus(Yuplay2Status status)
{
  if (status == YU2_WRONG_LOGIN)
    session.logoff();

  return CommonLoginAction::onErrorStatus(status);
}


//==================================================================================================
bool YuSession::StokenLoginAction::run(const YuString& token)
{
  YuStrMap post({
    { "stoken", token }
  });

  return runLoginAction(session.getStokenLoginUrl(), post, false);
}


//==================================================================================================
bool YuSession::NSwitchLoginAction::run(const YuString& nintendo_jwt,
                                        const YuString& game_id,
                                        const YuString& nintendo_nick,
                                        const YuString& lang)
{
  YuStrMap post({
    { "nick", nintendo_nick },
    { "lang", lang }
  });

  headers["Authorization"].sprintf("Bearer %s", nintendo_jwt.c_str());

  return runLoginAction(session.getNswitchLoginUrl(), post, headers, game_id);
}


//==================================================================================================
void YuSession::NSwitchLoginAction::onOkStatus(const YuJson& json)
{
  int errCode = (int)json.at("eshop_error").i();
  YuString errMsg = json.at("eshop_msg").s();

  session.onNswitchLogin(errCode, errMsg);
}


//==================================================================================================
bool YuSession::SsoStokenLoginAction::run(const YuString& tok)
{
  YuStrMap post({
    { "stoken", tok },
    { "format", "auth" }
  });

  return runLoginAction(session.getSsoStokenLoginUrl(), post, false);
}


//==================================================================================================
bool YuSession::PsnLoginAction::run(const YuString& code, const YuString& game_id, int issuer,
                                    const YuString& lang, unsigned conn_timeout,
                                    unsigned request_timeout, const YuString& title_id)
{
  YuStrMap post({
    { "code", code },
    { "lang", lang }
  });

  post["issuer"].sprintf("%d", issuer);

  if (!title_id.empty())
    post["title"] = title_id;

  return runLoginAction(session.getPsnLoginUrl(), post, game_id, conn_timeout, request_timeout);
}


//==================================================================================================
void YuSession::PsnLoginAction::onOkStatus(const YuJson& json)
{
  session.onPsnLogin(json.at("psn_plus").b());
}


//==================================================================================================
bool YuSession::SteamLoginAction::run(const YuString& token, const YuMemRange<char>& code,
                                      int app_id, const YuString& steam_nick, const YuString& lang,
                                      bool only_known)
{
  YuString appId(YuString::CtorSprintf(), "%d", app_id);

  YuFormData postForm;

  postForm["token"] = token;
  postForm["code"] = code;
  postForm["app_id"] = appId;
  postForm["lang"] = lang;
  postForm["nick"] = steam_nick;

  if (only_known)
    postForm["known"] = "1";

  setCommonLoginParams(postForm);

  return sendPost(session.getSteamLoginUrl(), postForm);
}


//==================================================================================================
bool YuSession::TencentLoginAction::run()
{
#define STOP_ACTION(code) \
  { \
    done(code); \
    doneEvt.set(); \
    return false; \
  }

#if YU2_WINDOWS

  struct TencentClose
  {
    inline void operator()(Tenio::ITclsProxy* h)
    {
      if (h)
      {
        h->Close();
        h->NotifyExit();
        ::ReleaseTclsProxy(&h);
      }
    }
  };

  typedef YuSysHandle<Tenio::ITclsProxy*, TencentClose>   TencentHandle;

  static YuWin32Dll dll;


  if (!dll.valid())
  {
    dll.set(::LoadLibraryA("TenProxy.dll"));

    if (!dll.valid())
    {
      YuDebug::out("Couldn't load library TenProxy.dll");
      STOP_ACTION(YU2_TENCENT_CLIENT_DLL_LOST);
    }

    g_hTenProxyModule = dll;
  }

  TencentHandle tcHandle(::CreateTclsProxy());

  if (!tcHandle.valid())
  {
    YuDebug::out("Couldn't get Tencent proxy instance");
    STOP_ACTION(YU2_FAIL);
  }

  Tenio::ITclsProxy* tcProxy = tcHandle;

  if (!tcProxy->Initialize())
  {
    YuDebug::out("Couldn't init Tencent proxy");
    STOP_ACTION(YU2_TENCENT_CLIENT_NOT_RUNNING);
  }

  Tenio::LOGININFO qqLogin = { sizeof(qqLogin) };
  if (!tcProxy->GetLoginInfo(&qqLogin))
  {
    YuDebug::out("Couldn't get QQ login info");
    STOP_ACTION(YU2_FAIL);
  }

  YuDebug::out("QQ user ID: %u", qqLogin.stServerLoginInfo.unUIN);
  YuDebug::out("QQ sign length: %u", (unsigned)qqLogin.stServerLoginInfo.byGameSignatureLen);

  BYTE netCafeSign[MAXLEN_QQINFO] = {0};
  int netCafeLen = sizeof(netCafeSign);
  if (!tcProxy->GetBYTE(SM_BYTE_NETBAR_SIGNATURE, netCafeSign, &netCafeLen))
  {
    YuDebug::out("Couldn't get Cyber Cafe sign");
    netCafeLen = 0;
  }

  YuDebug::out("QQ Cyber Cafe sign length: %d", netCafeLen);

  YuMemRange<char> qqSig((const char*)qqLogin.stServerLoginInfo.byarrGameSignature,
    qqLogin.stServerLoginInfo.byGameSignatureLen);

  YuFormData tcUri;
  tcUri["qq_id"] = YuString(YuString::CtorSprintf(), "%u", qqLogin.stServerLoginInfo.unUIN);
  tcUri["qq_sign"] = qqSig;

  if (netCafeLen)
  {
    YuMemRange<char> ncSig((const char*)netCafeSign, netCafeLen);
    tcUri["qq_nc_sign"] = qqSig;
  }

  setCommonLoginParams(tcUri);

  YuString loginUrl;
  YuString loginTokenUrl;
  YuString buyGoldUrl;
  YuString paymentConfUrl;

  if (!qqLogin.stGameServerInfo.dwCountOfServInfo)
  {
    YuDebug::out("Tencent server list is empty");
    STOP_ACTION(YU2_FAIL);
  }

  const Tenio::SERVERINFO& tenServer = qqLogin.stGameServerInfo.ServInfo[0];

  if (tenServer.dwAppDataLen > 0)
  {
    YuString appData((const char *)tenServer.AppData, tenServer.dwAppDataLen);
    session.onTencentAppData(appData);
    YuDebug::out("Tencent AppData: '%s'", appData.c_str());
  }
  else
  {
    YuDebug::out("Tencent AppData: none");
    session.onTencentAppData(YuString(""));
  }

  YuString authHost = YuStr::unicodeToWin(tenServer.szConnectUrl);

  loginUrl = "https://" + authHost;

  if (tenServer.dwConnectPort != 443)
    loginUrl.append_sprintf(":%u", tenServer.dwConnectPort);

  loginTokenUrl = loginUrl + "/login_token.php";
  buyGoldUrl = loginUrl + "/purchase_gold.php";
  paymentConfUrl = loginUrl + "/payment_conf.php";

  loginUrl += "/login.php";

  //Override default login URL for prolonging token fetched from Tencent auth
  //Also set up all Tencent-related URLs
  session.setOptionStringSync(YU2_LOGIN_URL, loginUrl.c_str());
  session.setOptionStringSync(YU2_TOKEN_URL, loginTokenUrl.c_str());
  session.setOptionStringSync(YU2_TENCENT_GOLD_URL, buyGoldUrl.c_str());
  session.setOptionStringSync(YU2_TENCENT_PAYMENT_CONF_URL, paymentConfUrl.c_str());

  return sendPost(loginUrl, tcUri);

#else

  STOP_ACTION(YU2_FAIL);

#endif //YU2_WINDOWS

#undef STOP_ACTION
}


//==================================================================================================
bool YuSession::DmmLoginAction::run(const YuString& user_id, const YuString& dmm_token,
                                    const YuString& app_id, const YuString& game_id)
{
  YuStrMap post({
    { "dmm_id", user_id },
    { "dmm_token", dmm_token },
    { "app_id", app_id },
  });

  return runLoginAction(session.getDmmLoginUrl(), post, game_id);
}


//==================================================================================================
bool YuSession::XboxLoginAction::run(const YuString& key, const YuString& sig)
{
  YuStrMap post;

  YuStrMap heads({
    { "Authorization", key },
    { "Signature", sig }
  });

  return runLoginAction(session.getXboxLoginUrl(), post, heads);
}


//==================================================================================================
void YuSession::XboxLoginAction::onOkStatus(const YuJson& json)
{
  session.onXboxLogin(json.at("game_pass").b());
}


//==================================================================================================
bool YuSession::EpicLoginAction::run(const YuString& epic_token, const YuString& game_id)
{
  YuStrMap post({
    { "epic_token", epic_token }
  });

  return runLoginAction(session.getEpicLoginUrl(), post, game_id);
}


//==================================================================================================
bool YuSession::AppleLoginAction::run(const YuString& id_token)
{
  YuStrMap post({
    { "id_token", id_token }
  });

  return runLoginAction(session.getAppleLoginUrl(), post);
}


//==================================================================================================
bool YuSession::GooglePlayLoginAction::run(const YuString& code, const YuString& firebase_id)
{
  YuStrMap post({
    { "code", code }
  });

  if (firebase_id.length())
    post["firebase_id"] = firebase_id;

  return runLoginAction(session.getGooglePlayLoginUrl(), post);
}


//==================================================================================================
bool YuSession::FacebookLoginAction::run(const YuString& access_token, bool limited)
{
  YuStrMap post({
    { "fb_token", access_token }
  });

  if (limited)
    post["limited"] = "1";

  return runLoginAction(session.getFacebookLoginUrl(), post);
}


//==================================================================================================
bool YuSession::FirebaseLoginAction::run(const YuString& firebase_id, const YuString& secret,
                                         bool only_known)
{
  YuStrMap post({
    { "firebase_id", firebase_id }
  });

  YuString signStr = secret;
  signStr += firebase_id;

  post["sign"] = YuStr::sha1String(signStr);

  if (only_known)
    post["known"] = "1";

  return runLoginAction(session.getFirebaseLoginUrl(), post);
}


//==================================================================================================
bool YuSession::GuestLoginAction::run(const YuString& system_id, const YuString& secret,
                                      const YuString& nick, bool only_known)
{
  YuString guestId(YuStr::sha1String(system_id));

  YuStrMap post({
    { "guest_id", guestId }
  });

  if (nick.length())
    post["nick"] = nick;

  YuString signStr;
  signStr.reserve(secret.length() + guestId.length());
  signStr.assign(secret);
  signStr += guestId;

  post["sign"] = YuStr::sha1String(signStr);

  if (only_known)
    post["known"] = "1";

  return runLoginAction(session.getGuestLoginUrl(), post);
}


//==================================================================================================
bool YuSession::SamsungLoginAction::run(const YuString& samsung_token)
{
  YuStrMap post({
    { "samsung_token", samsung_token }
  });

  return runLoginAction(session.getSamsungLoginUrl(), post);
}


//==================================================================================================
bool YuSession::ExternalLoginAction::run(const YuString& external_jwt)
{
  YuStrMap post;

  headers["Authorization"].sprintf("Bearer %s", external_jwt.c_str());

  return runLoginAction(session.getExternalLoginUrl(), post, headers);
}


//==================================================================================================
bool YuSession::HuaweiLoginAction::run(const YuString& huawei_token)
{
  YuStrMap post({
    { "hw_token", huawei_token }
  });

  return runLoginAction(session.getHuaweiLoginUrl(), post);
}


//==================================================================================================
bool YuSession::NvidiaLoginAction::run(const YuString& nvidia_jwt)
{
  YuStrMap post({
    { "nvidia_token", nvidia_jwt }
  });

  return runLoginAction(session.getNvidiaLoginUrl(), post);
}


#if YU2_WINDOWS
//==================================================================================================
Yuplay2Status YuSession::NvidiaLoginAction::onErrorStatus(Yuplay2Status status)
{
  saveStatusJson(answer.at("status").s(), "", "", answer.at("error").s());

  return CommonLoginAction::onErrorStatus(status);
}


//==================================================================================================
void YuSession::NvidiaLoginAction::onOkStatus(const YuJson& json)
{
  saveStatusJson(answer.at("status").s(), answer.at("user_id").s(), answer.at("nick").s(), "");

  CommonLoginAction::onOkStatus(json);
}


//==================================================================================================
bool YuSession::NvidiaLoginAction::getStatusJsonPath(YuPath& path) const
{
  if (session.getGameId().empty())
    return false;

  path = YuFs::getSpecialFolder(CSIDL_LOCAL_APPDATA);

  if (path.length())
  {
    path.append("Gaijin/GFN");

    if (!YuFs::dirExists(path) && !YuFs::createTree(path))
      return false;

    path.append(session.getGameId());
    path.setExt(".json");

    return true;
  }

  return false;
}


//==================================================================================================
void YuSession::NvidiaLoginAction::saveStatusJson(const YuString& status, const YuString& user_id,
                                                  const YuString& nick, const YuString& error) const
{
  YuPath path;
  if (getStatusJsonPath(path))
  {
    YuJsoner json;

    json.at("status").setString(status);

    if (status == "OK")
    {
      json.at("user_id").setString(user_id);
      json.at("nick").setString(nick);
    }
    else
      json.at("error").setString(error);

    json.save(path);
  }
}
#endif //YU2_WINDOWS


//==================================================================================================
bool YuSession::VkLoginAction::run(const YuString& vk_token, const YuString& client_id)
{
  YuStrMap post({
    { "vk_token", vk_token },
    { "vk_client_id", client_id },
  });

  return runLoginAction(session.getVkLoginUrl(), post);
}


//==================================================================================================
bool YuSession::WebLoginAction::run(const YuString& session_id)
{
  YuString extUri(YuString::CtorSprintf(), "?gsid=%s", session_id.c_str());

  YuUrl extUrl(session.getWebLoginBrowserUrl());
  extUrl.append(extUri);

#if YU2_WINDOWS

  ::ShellExecuteA(NULL, NULL, extUrl.c_str(), NULL, NULL, SW_SHOW);

#elif defined(YUPLAY2_USE_GNOME)

  ::g_app_info_launch_default_for_uri_async(extUrl.c_str(), NULL, NULL, NULL, NULL);

#else

  YuDebug::out("In-browser web auth not implemented on this platform");

  done(YU2_WRONG_PARAMETER);
  doneEvt.set();
  return false;

#endif //YU2_WINDOWS

  post["gsid"] = session_id;

#if YU2_WINAPI
  startTime = ::_time64(NULL);
#else
  startTime = ::time(NULL);
#endif //YU2_WINAPI

  return sendPost(session.getWebLoginUrl(), post);
}


//==================================================================================================
void YuSession::WebLoginAction::stop()
{
  stopEvt.set();
  repeatingRequest = false;

  CommonLoginAction::stop();
}


//==================================================================================================
void YuSession::WebLoginAction::repeatRequest(const YuString& url)
{
  if (sendPost(url, post))
    repeatingRequest = true;
  else
    done(YU2_FAIL);
}


//==================================================================================================
void YuSession::WebLoginAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  repeatingRequest = false;

  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    CommonLoginAction::onHttpResponse(url, data);
    return;
  }
  else if (status == YU2_RETRY)
  {
    int delaySec = answer.at("delay").i();

    if (delaySec > 0)
    {
      const int delayTick = 100;
      for (int rest = delaySec * 1000; rest > 0; rest -= delayTick)
      {
        YuSystem::sleep(delayTick);

        if (stopEvt.check())
          return;
      }
    }

    repeatRequest(url);
    return;
  }

  done(status);
}


//==================================================================================================
void YuSession::WebLoginAction::onHttpError(const YuString& url, int err_no)
{
  repeatingRequest = false;

  if (curlError(err_no) == YU2_TIMEOUT)
  {
#if YU2_WINAPI
    int64_t now = ::_time64(NULL);
#else
    int64_t now = ::time(NULL);
#endif //YU2_WINAPI

    if (now - startTime < session.webLoginStatusTimeout)
    {
      YuDebug::out("Web auth still not complete, need to ask more");
      repeatRequest(url);

      return;
    }
  }

  CommonLoginAction::onHttpError(url, err_no);
}


//==================================================================================================
void YuSession::WebLoginAction::onHttpEnd(const YuString& url)
{
  if (!repeatingRequest)
    CommonLoginAction::onHttpEnd(url);
}


//==================================================================================================
bool YuSession::KongZhongLoginAction::run(const YuString& kz_token)
{
  YuStrMap post({
    { "kz_token", kz_token },
  });

  YuUrl url(session.getLoginUrl());

  if (session.getKzChannelCode().length())
  {
    YuString apiUri(url.uri());

    if (apiUri.find("channelcode=") == apiUri.npos)
    {
      apiUri.sprintf("channelcode=%s", session.getKzChannelCode().c_str());
      url.appendUri(apiUri);
    }
  }

  return runLoginAction(url.str(), post);
}


//==================================================================================================
void YuSession::KongZhongLoginAction::onOkStatus(const YuJson& json)
{
  session.onKongZhongLogin(json.in("fcm") ? (int)json.at("fcm").i() : -1);
}


//==================================================================================================
Yuplay2Status YuSession::KongZhongLoginAction::onErrorStatus(Yuplay2Status status)
{
  if (answer.in("msg"))
    session.onKongZhongLoginFail(answer.at("msg").s());

  return CommonLoginAction::onErrorStatus(status);
}


//==================================================================================================
bool YuSession::WegameLoginAction::run(uint64_t wegame_id, const YuString& ticket,
                                       const YuString& nick)
{
  wegameId = wegame_id;

  YuStrMap post({
    { "ticket", ticket },
  });

  post["wegame_id"].sprintf("%llu", wegame_id);

  if (nick.length())
    post["nick"] = nick;

  return runLoginAction(session.getWegameLoginUrl(), post);
}


//==================================================================================================
void YuSession::WegameLoginAction::onOkStatus(const YuJson& json)
{
  session.onWegameLogin(wegameId);
}


//==================================================================================================
Yuplay2Status YuSession::WegameLoginAction::onErrorStatus(Yuplay2Status status)
{
  if (answer.in("msg"))
    session.onKongZhongLoginFail(answer.at("msg").s());

  return CommonLoginAction::onErrorStatus(status);
}
