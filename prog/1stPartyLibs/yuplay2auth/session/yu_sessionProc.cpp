#include "stdafx.h"
#include "yu_sessionProc.h"
#include "yu_urls.h"
#include "yu_debug.h"
#include "yu_string.h"
#include "yu_userInfo.h"
#include "yu_binary.h"
#include "yu_friends.h"
#include "yu_system.h"
#include "yu_bencode.h"
#include "yu_fs.h"
#include "yu_url.h"
#include "yu_itemInfo.h"
#include "yu_yupmaster.h"
#include "yu_sysHandle.h"
#include "yu_fallback.h"

#include <time.h>

#if YU2_WINDOWS
#include <tchar.h>
#include <TCLSProxy/tclsproxydef.h>
#include <TCLSProxy/TenTenioProxyHelp.h>
#endif //YU2_WINDOWS

#ifdef YUPLAY2_USE_XCURL
#include <XCurl.h>
#else
#include <curl/curl.h>
#endif //YUPLAY2_USE_XCURL

#ifdef YUPLAY2_USE_GNOME
#include <gtk/gtk.h>
#endif //YUPLAY2_USE_GNOME

#include <openssl/md5.h>


#define USER_DATA_CHUNK_SIZE            (1024 * 256)


//==================================================================================================
static YuBinary* getBinary(const YuCharTab& from)
{
  return from.size() ? new YuBinary(from) : NULL;
}


//==================================================================================================
static YuString getYupVersion(const YuPath& path)
{
  YuCharTab file;
  if (!YuFs::loadFile(path, file))
    return "";

  YuBencode benc;
  if (!benc.decode(file))
    return "";

  return benc.at("yup").at("version").s();
}


//==================================================================================================
static YuString getFailbackUrl(const YuString& uri, const YuString& ip)
{
  YuUrl ret(ip);
  ret.setProto("https");
  ret.append(uri);

  return ret.str();
}


//==================================================================================================
bool YuSession::LoginAction::run(const YuString& login, const YuString& pass, bool auth_client,
                                 const YuString& game_id)
{
  beginAction();

  YuStrMap post;

  post["login"] = login;
  post["password"] = pass;
  post["v"] = "2";

  if (!session.getAppId())
    post["nojwt"] = "1";

  if (auth_client)
    post["client"] = YuSystem::getSystemId();
  else
    post["declient"] = YuSystem::getSystemId();

  setCommonLoginParams(post, game_id.c_str());

  session.onTwoStepResponse("", "", false, false, false);

  return sendPost(session.getLoginUrl(), post);
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
  beginAction();

  YuStrMap post;

  post["login"] = login;
  post["password"] = pass;
  post["2step"] = code;

  if (!session.getAppId())
    post["nojwt"] = "1";

  if (auth_client)
    post["client"] = YuSystem::getSystemId();
  else
    post["declient"] = YuSystem::getSystemId();

  setCommonLoginParams(post);

  return sendPost(session.getLoginUrl(), post);
}


//==================================================================================================
bool YuSession::TokenLoginAction::run(const YuString& token)
{
  beginAction();

  YuStrMap post;
  post["token"] = token;

  return sendPost(session.getTokenUrl(), post);
}


//==================================================================================================
bool YuSession::StokenLoginAction::run(const YuString& token)
{
  beginAction();

  YuStrMap uri;
  uri["stoken"] = token;

  return sendPost(session.getStokenLoginUrl(), uri);
}


//==================================================================================================
bool YuSession::NSwitchLoginAction::run(const YuString& nintendo_jwt,
                                        const YuString& game_id,
                                        const YuString& nintendo_nick,
                                        const YuString& lang)
{
  beginAction();

  YuStrMap post;

  post["nick"] = nintendo_nick;
  post["lang"] = lang;

  setCommonLoginParams(post, game_id.c_str());

  headers["Authorization"].sprintf("Bearer %s", nintendo_jwt.c_str());

  return sendPost(session.getNswitchLoginUrl(), post, headers);
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
  beginAction();

  YuStrMap postData;

  postData["stoken"] = tok;
  postData["format"] = "auth";

  return sendPost(session.getSsoStokenLoginUrl(), postData);
}


//==================================================================================================
bool YuSession::PsnLoginAction::run(const YuString& code, const YuString& game_id, int issuer,
                                    const YuString& lang, unsigned conn_timeout,
                                    unsigned request_timeout, const YuString& title_id)
{
  beginAction();

  YuStrMap post;

  post["code"] = code;
  post["issuer"].sprintf("%d", issuer);
  post["lang"] = lang;

  if (!title_id.empty())
    post["title"] = title_id;

  setCommonLoginParams(post, game_id.c_str());

  return sendPost(session.getPsnLoginUrl(), post, conn_timeout, request_timeout);
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
  beginAction();

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


  beginAction();

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
  beginAction();

  YuStrMap post;

  post["dmm_id"] = user_id;
  post["dmm_token"] = dmm_token;
  post["app_id"] = app_id;

  setCommonLoginParams(post, game_id.c_str());

  return sendPost(session.getDmmLoginUrl(), post);
}


//==================================================================================================
bool YuSession::XboxLoginAction::run(const YuString& key, const YuString& sig)
{
  beginAction();

  YuStrMap post;
  setCommonLoginParams(post);

  YuStrMap heads;
  heads["Authorization"] = key;
  heads["Signature"] = sig;

  return sendPost(session.getXboxLoginUrl(), post, heads);
}


//==================================================================================================
void YuSession::XboxLoginAction::onOkStatus(const YuJson& json)
{
  session.onXboxLogin(json.at("game_pass").b());
}


//==================================================================================================
bool YuSession::EpicLoginAction::run(const YuString& epic_token, const YuString& game_id)
{
  beginAction();

  YuStrMap post;
  post["epic_token"] = epic_token;

  setCommonLoginParams(post, game_id.c_str());

  return sendPost(session.getEpicLoginUrl(), post);
}


//==================================================================================================
bool YuSession::AppleLoginAction::run(const YuString& id_token)
{
  beginAction();

  YuStrMap post;
  post["id_token"] = id_token;

  setCommonLoginParams(post);

  return sendPost(session.getAppleLoginUrl(), post);
}


//==================================================================================================
bool YuSession::GooglePlayLoginAction::run(const YuString& code, const YuString& firebase_id)
{
  beginAction();

  YuStrMap post;
  post["code"] = code;

  if (firebase_id.length())
    post["firebase_id"] = firebase_id;

  setCommonLoginParams(post);

  return sendPost(session.getGooglePlayLoginUrl(), post);
}


//==================================================================================================
bool YuSession::FacebookLoginAction::run(const YuString& access_token, bool limited)
{
  beginAction();

  YuStrMap post;
  post["fb_token"] = access_token;

  if (limited)
    post["limited"] = "1";

  setCommonLoginParams(post);

  return sendPost(session.getFacebookLoginUrl(), post);
}


//==================================================================================================
bool YuSession::FirebaseLoginAction::run(const YuString& firebase_id, const YuString& secret,
                                         bool only_known)
{
  beginAction();

  YuStrMap post;
  post["firebase_id"] = firebase_id;

  YuString signStr = secret;
  signStr += firebase_id;

  post["sign"] = YuStr::sha1String(signStr);

  if (only_known)
    post["known"] = "1";

  setCommonLoginParams(post);

  return sendPost(session.getFirebaseLoginUrl(), post);
}


//==================================================================================================
bool YuSession::GuestLoginAction::run(const YuString& system_id, const YuString& secret,
                                      const YuString& nick, bool only_known)
{
  beginAction();

  YuString guestId(YuStr::sha1String(system_id));
  YuStrMap post;

  post["guest_id"] = guestId;

  if (nick.length())
    post["nick"] = nick;

  YuString signStr;
  signStr.reserve(secret.length() + guestId.length());
  signStr.assign(secret);
  signStr += guestId;

  post["sign"] = YuStr::sha1String(signStr);

  if (only_known)
    post["known"] = "1";

  setCommonLoginParams(post);

  return sendPost(session.getGuestLoginUrl(), post);
}


//==================================================================================================
bool YuSession::SamsungLoginAction::run(const YuString& samsung_token)
{
  beginAction();

  YuStrMap post;
  post["samsung_token"] = samsung_token;

  setCommonLoginParams(post);

  return sendPost(session.getSamsungLoginUrl(), post);
}


//==================================================================================================
bool YuSession::ExternalLoginAction::run(const YuString& pix_token)
{
  beginAction();

  headers["Authorization"].sprintf("Bearer %s", pix_token.c_str());

  YuStrMap post;
  setCommonLoginParams(post);

  return sendPost(session.getExternalLoginUrl(), post, headers);
}

//==================================================================================================
bool YuSession::HuaweiLoginAction::run(const YuString& huawei_token)
{
  beginAction();

  YuStrMap post;
  post["hw_token"] = huawei_token;

  setCommonLoginParams(post);

  return sendPost(session.getHuaweiLoginUrl(), post);
}

//==================================================================================================
bool YuSession::NvidiaLoginAction::run(const YuString& nvidia_jwt)
{
  beginAction();

  YuStrMap post;
  post["nvidia_token"] = nvidia_jwt;

  setCommonLoginParams(post);

  return sendPost(session.getNvidiaLoginUrl(), post);
}


//==================================================================================================
bool YuSession::WebLoginSessionIdAction::run()
{
  beginAction();

  YuStrMap post;

  if (!session.getAppId())
    post["nojwt"] = "1";

  setCommonLoginParams(post);

  return sendPost(session.getWebLoginSessionIdUrl(), post);
}


//==================================================================================================
void YuSession::WebLoginSessionIdAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    YuString gsid = answer.at("gsid").s();

    if (gsid.length())
    {
      IYuplay2String* res = new YuSessionString(gsid);
      done(YU2_OK, res);
      return;
    }
  }

  done(status);
}


//==================================================================================================
bool YuSession::WebLoginAction::run(const YuString& session_id)
{
  beginAction();

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
  beginAction();

  YuStrMap post;
  post["kz_token"] = kz_token;

  setCommonLoginParams(post);

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

  return sendPost(url.str(), post);
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
  beginAction();

  wegameId = wegame_id;

  YuStrMap post;

  post["ticket"] = ticket;
  post["wegame_id"].sprintf("%llu", wegame_id);

  if (nick.length())
    post["nick"] = nick;

  setCommonLoginParams(post);

  return sendPost(session.getWegameLoginUrl(), post);
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


//==================================================================================================
bool YuSession::GetTwoStepCodeAction::run()
{
  beginAction();

  if (!session.isHasTwoStepRequest())
  {
    doneEvt.set();
    return false;
  }

  YuStrMap get;
  get["requestId"] = session.getTwoStepRequestId();
  get["userId"] = session.getTwoStepUserId();

  YuHttpHandle http = YuHttp::self().getTimeout(session.getTwoStepUrl(), get, 60, 60, 60, this);
  setHttpAct(http);

  if (http)
    return true;

  doneEvt.set();
  return false;
}


//==================================================================================================
void YuSession::GetTwoStepCodeAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  if (answer.decode(data))
  {
    if (answer.in("Message"))
    {
      IYuplay2String* res = NULL;
      res = new YuSessionString(answer.at("Message").s());
      done(YU2_OK, res);
      return;
    }
  }

  done(YU2_FAIL, NULL);
}


//==================================================================================================
bool YuSession::XboxLinkAction::run(const YuString& email, const YuString& key, const YuString& sig)
{
  beginAction();

  YuStrMap post;
  post["game"] = session.getGameId();
  post["login"] = email;

  YuStrMap headers;
  headers["Authorization"] = key;
  headers["Signature"] = sig;

  return sendPost(session.getXboxLinkUrl(), post, headers);
}


//==================================================================================================
bool YuSession::BasicUserinfoAction::run(const YuString& user_id, const YuString& login,
                                         const YuString& nick, const YuString& token,
                                         const YuString& tags, const YuString& nickorig,
                                         const YuString& nicksfx, const YuStrMap& meta)
{
  beginAction();

  IYuplay2UserInfo* info = new YuBasicUserInfo(
    user_id, login, nick, token, tags, nickorig, nicksfx, meta);

  done(YU2_OK, info);

  doneEvt.set();
  return true;
}


//==================================================================================================
bool YuSession::PurchCountAction::run(const YuString& token, const YuString& item_guid)
{
  beginAction();

  guid = item_guid;

  YuStrMap uri;
  uri["token"] = token;
  uri["guid"] = guid;

  return sendPost(GET_PURCHASES_CNT_URL, uri);
}


//==================================================================================================
Yuplay2Status YuSession::PurchCountAction::parseServerAnswer(const YuCharTab& data, int& count)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    const YuParserVal::Dict& items = answer.at("items").o();
    count = 0;

    for (YuParserVal::Dict::const_iterator i = items.begin(), end = items.end(); i != end; ++i)
      if (!i->first.comparei(guid))
      {
        count = i->second.i();
        break;
      }
  }

  return status;
}


//==================================================================================================
void YuSession::PurchCountAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  //Is static to make it alive during getPurchasesCountSync() call
  //This action may be performed once so it is safe
  static int count = 0;

  Yuplay2Status st = parseServerAnswer(data, count);

  done(st, &count);
}


//==================================================================================================
bool YuSession::MultiPurchCountAction::run(const YuString& token, const YuStringSet& item_guids)
{
  beginAction();

  guids = item_guids;

  YuStrMap uri;
  uri["token"] = token;

  YuHttp::addArrayToUri(uri, "guids", guids);

  return sendPost(GET_PURCHASES_CNT_URL, uri);
}


//==================================================================================================
void YuSession::MultiPurchCountAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2ItemPurchases* purch = NULL;
  Yuplay2Status st = parseServerAnswer(data, purch);

  done(st, purch);
}


//==================================================================================================
Yuplay2Status YuSession::MultiPurchCountAction::parseServerAnswer(const YuCharTab& data,
                                                                  IYuplay2ItemPurchases*& purch)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
    purch = new YuItemPurchases(answer, guids);

  return status;
}


//==================================================================================================
bool YuSession::PurchaseAction::run(const YuString& token, const YuString& guid,
                                    Yuplay2PaymentMethods payment)
{
  struct PayName
  {
    int type;
    const char* name;
  };

  static PayName payNames[] =
  {
    { YU2_PAY_QIWI, "qiwi" },
    { YU2_PAY_YANDEX, "yam" },
    { YU2_PAY_PAYPAL, "pp" },
    { YU2_PAY_WEBMONEY, "wm" },
    { YU2_PAY_AMAZON, "amzn" },
    { YU2_PAY_GJN, "gjn" },
    { YU2_PAY_BRNTREE, "brntree" },
  };


  beginAction();

  YuStrMap post;

  post["token"] = token;
  post["guid"] = guid;

  for (int i = 0; i < sizeof(payNames) / sizeof(payNames[0]); ++i)
    if (payNames[i].type == payment)
    {
      post["pay"] = payNames[i].name;
      break;
    }

  return sendPost(session.getBuyItemUrl(), post);
}


//==================================================================================================
void YuSession::PurchaseAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  done(parseServerJson(data));
}


//==================================================================================================
bool YuSession::ActivateAction::run(const YuString& token, const YuString& code)
{
  beginAction();

  YuStrMap post;

  post["token"] = token;
  post["code"] = code;

  return sendPost(session.getActivateItemUrl(), post);
}


//==================================================================================================
void YuSession::ActivateAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  done(parseServerJson(data));
}


//==================================================================================================
Yuplay2Status YuSession::CommonVersionAction::parseServerAnswer(const YuCharTab& answer,
                                                                YuString& version)
{
  Yuplay2Status ret = checkServerAnswer(answer, YU2_UNKNOWN);

  if (ret == YU2_UNKNOWN)
  {
    version.assign(answer.begin(), answer.end());
    return YU2_OK;
  }

  return ret;
}


//==================================================================================================
bool YuSession::GetVersionAction::run(const YuString& guid, const YuString& tag,
                                      unsigned conn_timeout, unsigned req_timeout)
{
  beginAction();

  YuStrMap post;

  post["proj"] = guid;

  if (tag.length())
    post["tag"] = tag;

  return sendGet(session.getVersionUrl(), post, conn_timeout, req_timeout);
}


//==================================================================================================
void YuSession::GetVersionAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* version = NULL;
  YuString ver;

  Yuplay2Status st = parseServerAnswer(data, ver);

  if (st == YU2_OK)
    version = new YuSessionString(ver);

  done(st, version);
}


//==================================================================================================
bool YuSession::CheckVersionAction::run(const YuPath& torrent, const YuString& guid,
                                        const YuString& tag)
{
  beginAction();

  this->torrent = torrent;

  YuStrMap post;

  post["proj"] = guid;
  post["tag"] = tag;

  return sendGet(session.getVersionUrl(), post);
}


//==================================================================================================
Yuplay2Status YuSession::CheckVersionAction::compareVersions(const YuCharTab& ver,
                                                             const YuPath& torrent, bool& new_ver)
{
  new_ver = false;

  YuString servVersion;
  Yuplay2Status ret = parseServerAnswer(ver, servVersion);

  if (ret == YU2_OK)
  {
    YuString version = getYupVersion(torrent);
    new_ver = (servVersion != version);
  }

  return ret;
}


//==================================================================================================
void YuSession::CheckVersionAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  bool newVer = false;
  Yuplay2Status st = compareVersions(data, torrent, newVer);

  done(st, newVer ? &newVer : NULL);
}


//==================================================================================================
void YuSession::YupmasterAction::stop()
{
  if (yupmaster)
    yupmaster->stop();
  else
    YuSession::AsyncHttpAction::stop();
}


//==================================================================================================
void YuSession::YupmasterAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  Yuplay2Status st = checkServerAnswer(data, YU2_UNKNOWN);

  if (st == YU2_UNKNOWN)
  {
    IYuplay2Binary* binary = getBinary(data);

    done(binary ? YU2_OK : YU2_FAIL, binary);
  }
  else
    done(st);
}


//==================================================================================================
bool YuSession::YupmasterAction::getFromYupmaster(const YuString& url, const YuStrMap& uri)
{
  if (yupmaster)
    return false;

  if (session.doYupmasterFailback())
    yupmaster = new YupmasterGetFile(5, session.getYupmasterFallbackHosts(),
      session.getYupmasterFailbackIp());
  else
    yupmaster = new YupmasterGetFile(5, session.getYupmasterFallbackHosts(), "");

#if YUPMASTER_POST
  if (yupmaster->post(url, uri, this))
#else
  if (yupmaster->get(url, uri, this))
#endif //YUPMASTER_POST
    return true;

  delete yupmaster;
  yupmaster = NULL;

  return false;
}


//==================================================================================================
bool YuSession::GetYupAction::run(const YuString& guid, const YuString& tag)
{
  beginAction();

  YuStrMap uri;

  uri["proj"] = guid;
  uri["tag"] = tag;
  uri["consist"] = "1";

  if (getFromYupmaster(session.getYupUrl(), uri))
    return true;

  doneEvt.set();
  return false;
}


//==================================================================================================
bool YuSession::CheckNewerYupAction::run(const YuPath& yup_path, const YuString& guid,
                                         const YuString& tag)
{
  beginAction();

  YuString version;

  if (YuFs::fileExists(yup_path))
    version = getYupVersion(yup_path);

  YuStrMap uri;

  uri["proj"] = guid;
  uri["tag"] = tag;
  uri["ver"] = version;

  if (getFromYupmaster(session.getNewerYupUrl(), uri))
    return true;

  doneEvt.set();
  return false;
}


//==================================================================================================
bool YuSession::GetYupVersionAction::run(const YuPath& torrent)
{
  beginAction();

  YuString version = getYupVersion(torrent);

  if (version.length())
  {
    IYuplay2Binary* ver = new YuBinary(version);

    done(YU2_OK, ver);
  }
  else
    done(YU2_NOT_FOUND);

  doneEvt.set();
  return true;
}


//==================================================================================================
bool YuSession::RenewTokenAction::run(const YuString& token)
{
  beginAction();

  YuStrMap post;
  post["token"] = token;

  return sendPost(session.getTokenUrl(), post);
}


//==================================================================================================
Yuplay2Status YuSession::RenewTokenAction::parseServerAnswer(const YuCharTab& data, YuString& token,
                                                             int64_t& token_exp, YuString& tags)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    token = answer.at("token").s();
    token_exp = answer.at("token_exp").i();
    tags = answer.at("tags").s();
  }

  return status;
}


//==================================================================================================
void YuSession::RenewTokenAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  YuString newToken;
  int64_t tokenExp;
  YuString tags;

  Yuplay2Status st = parseServerAnswer(data, newToken, tokenExp, tags);

  if (st == YU2_OK)
  {
    session.onNewToken(newToken, tokenExp);
    session.onNewTags(tags);
  }

  done(st);
}


//==================================================================================================
bool YuSession::RenewJwtAction::run(const YuString& jwt)
{
  beginAction();

  //Why webdev invent this shitty bycicles instead of well known urlencoded or form-data POST?
  YuCharTab json;
  json.reserve(jwt.length() + 10);
  tab_append(json, "{\"jwt\":\"", 8);
  tab_append(json, jwt);
  tab_append(json, "\"}", 2);

  return sendPost(session.getJwtUrl(), json);
}


//==================================================================================================
Yuplay2Status YuSession::RenewJwtAction::parseServerAnswer(const YuCharTab& data, YuString& jwt)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
    jwt = answer.at("jwt").s();

  return status;
}


//==================================================================================================
void YuSession::RenewJwtAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  YuString newJwt;

  Yuplay2Status st = parseServerAnswer(data, newJwt);

  if (st == YU2_OK)
    session.onNewJwt(newJwt);

  done(st);
}


//==================================================================================================
bool YuSession::ItemInfoAction::run(const YuString& guid, const YuString& token)
{
  beginAction();

  YuStrMap post;
  post["guid"] = guid;

  if (token.length())
    post["token"] = token;

  return sendPost(session.getItemInfoUrl(), post);
}


//==================================================================================================
void YuSession::ItemInfoAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2ItemInfo* info = NULL;
  Yuplay2Status st = parseServerAnswer(data, info);

  done(st, info);
}


//==================================================================================================
Yuplay2Status YuSession::ItemInfoAction::parseServerAnswer(const YuCharTab& data,
                                                           IYuplay2ItemInfo*& info)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    status = YU2_FAIL;

    const YuParserVal::Dict& items = answer.at("items").o();

    if (items.size())
    {
      const YuString& guid = items.begin()->first;
      const YuParserVal& item = items.begin()->second;

      YuItemInfo* itemInfo = new YuItemInfo;

      if (itemInfo->assign(guid, item))
        status = YU2_OK;
      else
      {
        delete itemInfo;
        itemInfo = NULL;
      }

      info = itemInfo;
    }
  }

  return status;
}


//==================================================================================================
bool YuSession::MultiItemInfoAction::run(const YuStringSet& item_guids, const YuString& token)
{
  beginAction();

  guids = item_guids;

  YuStrMap uri;
  uri["token"] = token;

  YuHttp::addArrayToUri(uri, "guids", guids);

  return sendPost(session.getItemInfoUrl(), uri);
}


//==================================================================================================
Yuplay2Status YuSession::MultiItemInfoAction::parseServerAnswer(const YuCharTab& data,
                                                                IYuplay2ItemsInfo*& info)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
    info = new YuItemsInfo(answer, guids);

  return status;
}


//==================================================================================================
void YuSession::MultiItemInfoAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2ItemsInfo* info = NULL;
  Yuplay2Status st = parseServerAnswer(data, info);

  done(st, info);
}


//==================================================================================================
bool YuSession::OpenItemPageAction::run(const YuString& item_guid, const YuString& user_token)
{
  beginAction();

  openPageState = STATE_ITEM_INFO;

  guid = item_guid;
  token = user_token;

  YuStrMap uri;
  uri["guid"] = guid;

  if (token.length())
    uri["token"] = token;

  return sendPost(session.getItemInfoUrl(), uri);
}


//==================================================================================================
void YuSession::OpenItemPageAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  if (openPageState == STATE_ITEM_INFO && onItemInfoReceived(data) == YU2_OK)
  {
    if (token.length())
    {
      openPageState = STATE_TOKEN;

      YuStrMap uri;
      uri["token"] = token;

      YuHttpHandle http = YuHttp::self().post(session.getUserStokenUrl(), uri, this);
      setHttpAct(http);

      if (!http)
      {
        done(YU2_FAIL);
        doneEvt.set();
      }
    }
    else
      openUrlInBrowser(itemUrl);
  }
  else if (openPageState == STATE_TOKEN)
  {
    YuUrl openUrl = itemUrl;

    Yuplay2Status st = onUserStokenReceived(data);

    if (st == YU2_OK)
      openUrl = getLoginUrl();
    else if (st == YU2_PSN_RESTRICTED)
    {
      done(st);
      return;
    }

    openUrlInBrowser(openUrl);
  }
}


//==================================================================================================
void YuSession::OpenItemPageAction::onHttpError(const YuString& url, int err_no)
{
  if (openPageState == STATE_TOKEN)
    openUrlInBrowser(itemUrl);
  else
    YuSession::AsyncHttpAction::onHttpError(url, err_no);
}


//==================================================================================================
void YuSession::OpenItemPageAction::onHttpEnd(const YuString& url)
{
  if (openPageState == STATE_DONE)
    AsyncHttpAction::onHttpEnd(url);
}


//==================================================================================================
Yuplay2Status YuSession::OpenItemPageAction::onItemInfoReceived(const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    status = YU2_FAIL;

    itemUrl = answer.at("items").at(guid).at("url").s();

    return itemUrl.length() ? YU2_OK : YU2_FAIL;
  }

  return status;
}


//==================================================================================================
Yuplay2Status YuSession::OpenItemPageAction::onUserStokenReceived(const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    status = YU2_FAIL;

    stoken = answer.at("stoken").s();

    return stoken.length() ? YU2_OK : YU2_FAIL;
  }

  return status;
}


//==================================================================================================
YuUrl YuSession::OpenItemPageAction::getLoginUrl() const
{
  YuString uri = "/yuplay/token_login.php?stoken=";
  uri += YuUrl::urlencode(stoken);
  uri += "&ret=";
  uri += YuUrl::urlencode(itemUrl.c_str());

  return YuUrl::combine(itemUrl.site(), uri);
}


//==================================================================================================
void YuSession::OpenItemPageAction::openUrlInBrowser(const YuUrl& url)
{
#if YU2_WINDOWS

  YuDebug::out("Open in browser: %s", url.c_str());
  ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOW);

#else

  YuDebug::out("Open URL in browser not implemented on this platform");

#endif //YU2_WINDOWS

  openPageState = STATE_DONE;

  done(YU2_OK);
}


//==================================================================================================
bool YuSession::LoggedUrlAction::run(const YuString& token, const YuString& base_url,
                                     const YuString& base_host)
{
  beginAction();

  url = base_url;
  baseHost = base_host;

  YuStrMap uri;
  uri["token"] = token;

  return sendPost(session.getUserStokenUrl(), uri);
}


//==================================================================================================
void YuSession::LoggedUrlAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* retUrl = NULL;
  Yuplay2Status st = parseServerAnswer(data, retUrl);

  done(st, retUrl);
}


//==================================================================================================
Yuplay2Status YuSession::LoggedUrlAction::parseServerAnswer(const YuCharTab& data,
                                                            IYuplay2String*& ret_url)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    status = YU2_FAIL;

    YuString stoken = answer.at("stoken").s();
    if (stoken.length())
    {
      YuString uri = "/yuplay/token_login.php?stoken=";
      uri += YuUrl::urlencode(stoken);
      uri += "&ret=";
      uri += YuUrl::urlencode(url.c_str());

      YuUrl retUrl(DEFAULT_SITE_URL);

      if (baseHost.length())
        retUrl.setHost(baseHost);
      else if (url.proto().length())
        retUrl.setHost(url.host());

      retUrl.append(uri);

      ret_url = new YuSessionString(retUrl.c_str());
      status = YU2_OK;
    }
  }

  return status;
}


//==================================================================================================
bool YuSession::SsoLoggedUrlAction::run(const YuString& token, const YuString& base_url,
                                        const YuString& base_host, const YuString& service_name)
{
  beginAction();

  url = base_url;
  baseHost = base_host;
  serviceName = service_name;

  YuStrMap uri;
  uri["token"] = token;

  return sendPost(session.getSsoUserStokenUrl(), uri);
}


//==================================================================================================
void YuSession::SsoLoggedUrlAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* retUrl = NULL;
  Yuplay2Status st = parseServerAnswer(data, retUrl);

  done(st, retUrl);
}


//==================================================================================================
Yuplay2Status YuSession::SsoLoggedUrlAction::parseServerAnswer(const YuCharTab& data,
                                                               IYuplay2String*& ret_url)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    status = YU2_FAIL;

    YuString stoken = answer.at("short_token").s();
    if (stoken.length())
    {
      YuString loggedUrl(session.getSsoLoggedUrl());
      loggedUrl += "?short_token=";
      loggedUrl += YuUrl::urlencode(stoken);

      if (!serviceName.empty() && serviceName != "any")
      {
        loggedUrl += "&service=";
        loggedUrl += YuUrl::urlencode(serviceName);
      }

      loggedUrl += "&return_url=";
      loggedUrl += YuUrl::urlencode(url.c_str());

      ret_url = new YuSessionString(loggedUrl);
      status = YU2_OK;
    }
  }

  return status;
}


//==================================================================================================
static bool updateKzChannelCode(YuString& chan, const YuString& uri)
{
  size_t chanPos = uri.find("channelCode=");

  if (chanPos != uri.npos)
  {
    size_t chanStart = chanPos + 12;
    size_t chanEnd = uri.find("&", chanPos);
    YuString uriChan;

    if (chanEnd == uri.npos)
      uriChan = uri.substr(chanStart);
    else
      uriChan = uri.substr(chanStart, chanEnd - chanStart);

    if (uriChan.length())
    {
      chan = uriChan;
      return true;
    }
  }

  return false;
}


//==================================================================================================
bool YuSession::KongZhongLoggedUrlAction::run(const YuString& kz_user_id, uint64_t wegame_id,
                                              const YuString& url)
{
  beginAction();

  const char* signKey = "aaOvoZvflEu2iWZJ9xv8Dy8bCPfDuGsg7ZGdJ8GYRjvTGCvlKcfFivJjCIs9cQ13";

  int64_t ts = ::time(NULL);
  YuString kzChan(wegame_id ? "weg" : "963");

  YuUrl ret(url);
  bool customChanCode = updateKzChannelCode(kzChan, ret.uri());
  bool addChanToUrl = true;

  if (customChanCode)
    addChanToUrl = false;
  else if (session.getKzChannelCode().length())
  {
    customChanCode = true;
    kzChan = session.getKzChannelCode();
  }

  YuString kzId;

  if (wegame_id)
    kzId.sprintf("%llu", wegame_id);
  else
    kzId = kz_user_id;

  YuString payload(YuString::CtorSprintf(),
    "accountId%schannelCode%stimestamp%lld%s", kzId.c_str(), kzChan.c_str(), ts, signKey);

  eastl::vector<unsigned char> md5(MD5_DIGEST_LENGTH, 0);
  ::MD5((const unsigned char*)payload.data(), payload.length(), md5.data());

  YuString sign(YuStr::hexString((const char*)md5.data(), md5.size()));

  YuString uri;

  if (addChanToUrl)
    uri.sprintf("accountId=%s&channelCode=%s&timestamp=%lld&sign=%s", kzId.c_str(), kzChan.c_str(),
      ts, sign.c_str());
  else
    uri.sprintf("accountId=%s&timestamp=%lld&sign=%s", kzId.c_str(), ts, sign.c_str());

  ret.appendUri(uri);

  IYuplay2String* retUrl = new YuSessionString(ret.c_str());

  done(YU2_OK, retUrl);

  doneEvt.set();
  return true;
}


//==================================================================================================
bool YuSession::SsoGetShortTokenAction::run(const YuString& token)
{
  beginAction();

  YuStrMap uri;
  uri["token"] = token;

  return sendPost(session.getSsoUserStokenUrl(), uri);
}


//==================================================================================================
void YuSession::SsoGetShortTokenAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* retUrl = NULL;
  Yuplay2Status st = parseServerAnswer(data, retUrl);

  done(st, retUrl);
}


//==================================================================================================
Yuplay2Status YuSession::SsoGetShortTokenAction::parseServerAnswer(const YuCharTab& data,
                                                               IYuplay2String*& ret_short_token)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    YuString shortToken = answer.at("short_token").s();
    if (shortToken.length())
      ret_short_token = new YuSessionString(shortToken.c_str());
    else
      status = YU2_FAIL;
  }

  return status;
}


//==================================================================================================
bool YuSession::GetStokenAction::run(const YuString& token, const YuString& jwt)
{
  beginAction();

  YuStrMap uri;
  uri["token"] = token;

  if (jwt.length())
    uri["jwt"] = jwt;

  return sendPost(session.getUserStokenUrl(), uri);
}


//==================================================================================================
void YuSession::GetStokenAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* stoken = NULL;
  Yuplay2Status st = parseServerAnswer(data, stoken);

  done(st, stoken);
}


//==================================================================================================
Yuplay2Status YuSession::GetStokenAction::parseServerAnswer(const YuCharTab& data,
                                                            IYuplay2String*& stoken)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    YuString token = answer.at("stoken").s();

    if (token.length())
      stoken = new YuSessionString(token.c_str());
    else
      status = YU2_FAIL;
  }

  return status;
}


//==================================================================================================
bool YuSession::GetOpenIdJwtAction::run(const YuString& jwt)
{
  beginAction();

  YuStrMap post;
  YuStrMap headers;

  headers["Authorization"].sprintf("Bearer %s", jwt.c_str());

  return sendPost(session.getOpenIdJwtUrl(), post, headers);
}


//==================================================================================================
void YuSession::GetOpenIdJwtAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* openid_jwt = NULL;
  Yuplay2Status st = parseServerAnswer(data, openid_jwt);

  done(st, openid_jwt);
}


//==================================================================================================
Yuplay2Status YuSession::GetOpenIdJwtAction::parseServerAnswer(const YuCharTab& data,
                                                               IYuplay2String*& openid_jwt)
{
  if (answer.decode(data))
  {
    YuString openIdJwt(answer.at("id_token").s());

    if (openIdJwt.length())
    {
      openid_jwt = new YuSessionString(openIdJwt.c_str());
      return YU2_OK;
    }
  }

  return YU2_FAIL;
}


//==================================================================================================
bool YuSession::TagUserAction::run(const YuString& token, const YuStringSet& tags)
{
  beginAction();

  YuStrMap post;
  post["token"] = token;

  YuString key;

  unsigned i = 0;
  for (YuStringSetCI t = tags.begin(), end = tags.end(); t != end; ++t, ++i)
  {
    key.sprintf("tags[%u]", i);
    post[key] = *t;
  }

  return sendPost(session.getTagUserUrl(), post);
}


//==================================================================================================
void YuSession::TagUserAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  done(parseServerAnswer(data));
}


//==================================================================================================
Yuplay2Status YuSession::TagUserAction::parseServerAnswer(const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    YuString tags = answer.at("tags").s();
    session.onNewTags(tags);
  }

  return status;
}


//==================================================================================================
bool YuSession::TencentBuyGoldAction::run(const YuString& token, unsigned gold)
{
  beginAction();

  YuStrMap post;

  post["token"] = token;
  post["gold"] = YuString(YuString::CtorSprintf(), "%u", gold);

  return sendPost(session.getTencentGoldUrl(), post);
}


//==================================================================================================
bool YuSession::TencentPaymentConfAction::run()
{
  if (session.tencentGoldRate <= 0.0)
  {
    beginAction();

    YuHttpHandle http = YuHttp::self().get(session.getTencentPayConfUrl(), this);
    setHttpAct(http);

    if (http)
      return true;

    doneEvt.set();
    return false;
  }
  else
    beginAction();

  done(YU2_OK, &session.tencentGoldRate);

  doneEvt.set();
  return true;
}


//==================================================================================================
void YuSession::TencentPaymentConfAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
    session.onTencentPaymentConf(answer.at("itc_eagles").f());

  done(status, status == YU2_OK ? &session.tencentGoldRate : NULL);
}


//==================================================================================================
bool YuSession::SteamLinkTokenAction::run(const YuString& token, const YuMemRange<char>& code,
                                          int app_id)
{
  beginAction();

  YuString appId(YuString::CtorSprintf(), "%d", app_id);
  YuFormData postForm;

  postForm["token"] = token;
  postForm["code"] = code;
  postForm["app_id"] = appId;

  return sendPost(session.getSteamLinkUrl(), postForm);
}


//==================================================================================================
void YuSession::SteamLinkTokenAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2String* res = NULL;
  Yuplay2Status st = parseServerAnswer(data, res);

  done(st, res);
}


//==================================================================================================
Yuplay2Status YuSession::SteamLinkTokenAction::parseServerAnswer(const YuCharTab& data,
                                                                 IYuplay2String*& token)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    YuString tok = answer.at("token").s();

    if (tok.length())
      token = new YuSessionString(tok.c_str());
    else
      status = YU2_FAIL;
  }

  return status;
}


//==================================================================================================
bool YuSession::GooglePurchaseAction::run(const YuString& token, const YuString& purch_json,
                                          bool do_acknowledge)
{
  beginAction();

  YuJson json;
  if (json.decode(purch_json))
  {
    itemId = json.at("productId").s();
    purchToken = json.at("purchaseToken").s();
  }

  YuStrMap post;

  post["token"] = token;
  post["purch"] = purch_json;
  post["game"] = session.getGameId();

  if (do_acknowledge)
    post["acknowledge"] = "1";

  return sendPost(session.getGooglePurchaseUrl(), post);
}


//==================================================================================================
void YuSession::GooglePurchaseAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2GooglePurchaseInfo* res = NULL;
  int added = 0;

  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    added = (int)answer.at("added").i();
    res = new GooglePurchaseInfo(purchToken, itemId, added);
  }

  done(status, res);
}


//==================================================================================================
bool YuSession::HuaweiPurchaseAction::run(const YuString& token, const YuString& purch_json,
                                          bool do_acknowledge)
{
  beginAction();

  YuJson json;
  if (json.decode(purch_json))
  {
    itemId = json.at("productId").s();
    purchToken = json.at("purchaseToken").s();
  }

  YuStrMap post;

  post["token"] = token;
  post["purch"] = purch_json;
  post["game"] = session.getGameId();

  if (do_acknowledge)
    post["acknowledge"] = "1";

  return sendPost(session.getHuaweiPurchaseUrl(), post);
}


//==================================================================================================
void YuSession::HuaweiPurchaseAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2GooglePurchaseInfo* res = NULL;
  int added = 0;

  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    added = (int)answer.at("added").i();
    res = new GooglePurchaseInfo(purchToken, itemId, added);
  }

  done(status, res);
}


//==================================================================================================
bool YuSession::ApplePurchaseAction::run(const YuString& token, const YuString& purch_json)
{
  beginAction();

  YuStrMap post;

  post["token"] = token;
  post["purch"] = purch_json;
  post["game"] = session.getGameId();

  return sendPost(session.getApplePurchaseUrl(), post);
}


//==================================================================================================
void YuSession::ApplePurchaseAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  IYuplay2GooglePurchaseInfo* res = NULL;
  int added = 0;

  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    added = (int)answer.at("added").i();
    res = new GooglePurchaseInfo(answer.at("apple_transaction_id").s(), "", added);
  }

  done(status, res);
}
