#include "stdafx.h"
#include "yu_sessionProc.h"
#include "yu_urls.h"
#include "yu_debug.h"
#include "yu_string.h"
#include "yu_userInfo.h"
#include "yu_binary.h"
#include "yu_friends.h"
#include "yu_bencode.h"
#include "yu_fs.h"
#include "yu_url.h"
#include "yu_itemInfo.h"
#include "yu_yupmaster.h"
#include "yu_fallback.h"

#include <time.h>

#include <curl/curl.h>

#include <openssl/md5.h>


#define USER_DATA_CHUNK_SIZE            (1024 * 256)
#define MAX_VERSION_LENGTH              32


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
bool YuSession::WebLoginSessionIdAction::run()
{
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
bool YuSession::GetTwoStepCodeAction::run()
{
  if (!session.isHasTwoStepRequest())
  {
    doneEvt.set();
    return false;
  }

  YuStrMap get({
    { "requestId", session.getTwoStepRequestId() },
    { "userId", session.getTwoStepUserId() }
  });

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
  YuStrMap post({
    { "game", session.getGameId() },
    { "login", email }
  });

  YuStrMap headers({
    { "Authorization", key },
    { "Signature", sig }
  });

  return sendPost(session.getXboxLinkUrl(), post, headers);
}


//==================================================================================================
bool YuSession::BasicUserinfoAction::run(const YuString& user_id, const YuString& login,
                                         const YuString& nick, const YuString& token,
                                         const YuString& tags, const YuString& nickorig,
                                         const YuString& nicksfx, const YuStrMap& meta)
{
  IYuplay2UserInfo* info = new YuBasicUserInfo(
    user_id, login, nick, token, tags, nickorig, nicksfx, meta);

  done(YU2_OK, info);

  doneEvt.set();
  return true;
}


//==================================================================================================
bool YuSession::PurchCountAction::run(const YuString& token, const YuString& item_guid)
{
  guid = item_guid;

  YuStrMap uri({
    { "token", token },
    { "guid", guid }
  });

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
  guids = item_guids;

  YuStrMap uri({
    { "token", token }
  });

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


  YuStrMap post({
    { "token", token },
    { "guid", guid }
  });

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
  YuStrMap post({
    { "token", token },
    { "code", code }
  });

  return sendPost(session.getActivateItemUrl(), post);
}


//==================================================================================================
void YuSession::ActivateAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  done(parseServerJson(data));
}


//==================================================================================================
void YuSession::CommonVersionAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  Yuplay2Status status = checkServerAnswer(data, YU2_UNKNOWN);
  void* res = NULL;

  if (status == YU2_UNKNOWN)
  {
    bool validVersion = !data.empty() && data.size() <= MAX_VERSION_LENGTH;

    if (validVersion)
      for (size_t i = 0; i < data.size(); ++i)
      {
        unsigned char c = data[i];

        if (c < 32 || c > 127)
        {
          validVersion = false;
          break;
        }
      }

    if (!validVersion)
    {
      fallback->retry();
      return;
    }

    YuString version(data.begin(), data.end());

    status = YU2_OK;
    res = onVersion(version);
  }

  done(status, res);
};


//==================================================================================================
bool YuSession::GetVersionAction::run(const YuString& guid, const YuString& tag,
                                      unsigned conn_timeout, unsigned req_timeout)
{
  YuStrMap post({
    { "proj", guid }
  });

  if (tag.length())
    post["tag"] = tag;

  return sendGet(session.getVersionUrl(), post, conn_timeout, req_timeout);
}


//==================================================================================================
void* YuSession::GetVersionAction::onVersion(const YuString& version)
{
  return new YuSessionString(version);
}


//==================================================================================================
bool YuSession::CheckVersionAction::run(const YuPath& torrent, const YuString& guid,
                                        const YuString& tag)
{
  this->torrent = torrent;

  YuStrMap post({
    { "proj", guid },
    { "tag", tag }
  });

  return sendGet(session.getVersionUrl(), post);
}


//==================================================================================================
void* YuSession::CheckVersionAction::onVersion(const YuString& version)
{
  YuString localVer = getYupVersion(torrent);
  bool newVer = localVer != version;

  return newVer ? (void*)0xFFFF : NULL;
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
  YuStrMap uri({
    { "proj", guid },
    { "tag", tag },
    { "consist", "1" }
  });

  if (getFromYupmaster(session.getYupUrl(), uri))
    return true;

  doneEvt.set();
  return false;
}


//==================================================================================================
bool YuSession::CheckNewerYupAction::run(const YuPath& yup_path, const YuString& guid,
                                         const YuString& tag)
{
  YuString version;

  if (YuFs::fileExists(yup_path))
    version = getYupVersion(yup_path);

  YuStrMap uri({
    { "proj", guid },
    { "tag", tag },
    { "ver", version }
  });

  if (getFromYupmaster(session.getNewerYupUrl(), uri))
    return true;

  doneEvt.set();
  return false;
}


//==================================================================================================
bool YuSession::GetYupVersionAction::run(const YuPath& torrent)
{
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
  else if (st == YU2_WRONG_LOGIN)
    session.logoff();

  done(st);
}


//==================================================================================================
bool YuSession::RenewJwtAction::run(const YuString& jwt)
{
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
  YuStrMap post({
    { "guid", guid }
  });

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
  guids = item_guids;

  YuStrMap uri({
    { "token", token }
  });

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
  openPageState = STATE_ITEM_INFO;

  guid = item_guid;
  token = user_token;

  YuStrMap uri({
    { "guid", guid }
  });

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
  url = base_url;
  baseHost = base_host;

  YuStrMap uri({
    { "token", token }
  });

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
  url = base_url;
  baseHost = base_host;
  serviceName = service_name;

  YuStrMap uri({
    { "token", token }
  });

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
  YuStrMap uri({
    { "token", token }
  });

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
  YuStrMap uri({
    { "token", token }
  });

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
  YuStrMap post({
    { "token", token }
  });

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
  YuStrMap post({
    { "token", token }
  });

  post["gold"].sprintf("%u", gold);

  return sendPost(session.getTencentGoldUrl(), post);
}


//==================================================================================================
bool YuSession::TencentPaymentConfAction::run()
{
  if (session.tencentGoldRate <= 0.0)
  {
    YuHttpHandle http = YuHttp::self().get(session.getTencentPayConfUrl(), this);
    setHttpAct(http);

    if (http)
      return true;

    doneEvt.set();
    return false;
  }

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
  YuJson json;
  if (json.decode(purch_json))
  {
    itemId = json.at("productId").s();
    purchToken = json.at("purchaseToken").s();
  }

  YuStrMap post({
    { "token", token },
    { "purch", purch_json },
    { "game", session.getGameId() }
  });

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
  YuJson json;
  if (json.decode(purch_json))
  {
    itemId = json.at("productId").s();
    purchToken = json.at("purchaseToken").s();
  }

  YuStrMap post({
    { "token", token },
    { "purch", purch_json },
    { "game", session.getGameId() }
  });

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
  YuStrMap post({
    { "token", token },
    { "purch", purch_json },
    { "game", session.getGameId() }
  });

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
