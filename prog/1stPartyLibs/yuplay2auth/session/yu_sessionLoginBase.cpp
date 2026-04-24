#include "stdafx.h"
#include "yu_sessionLoginBase.h"
#include "yu_json.h"
#include "yu_string.h"


//==================================================================================================
Yuplay2Status YuSession::CommonLoginAction::parseServerAnswer(const YuCharTab& data)
{
  YuJson json;
  if (json.decode(data))
  {
    YuString statusStr = json.at("status").s();
    Yuplay2Status status = checkJsonAnswer(statusStr, YU2_FAIL);

    if (status == YU2_OK)
    {
      YuString tags = json.at("tags").s();

      if (psnRestricted)
      {
        YuStringSet tagsSet = YuStr::explodeSet(tags, ',');

        if (tagsSet.find("psnrestricted") != tagsSet.end())
          return YU2_PSN_RESTRICTED;
      }

      YuString login(json.at("login").s());
      YuString nick(json.at("nick").s());
      YuString userId(json.at("user_id").s());
      YuString token(json.at("token").s());
      YuString jwt(json.at("jwt").s());
      YuString lang(json.at("lang").s());
      YuString country(json.at("country").s());
      int64_t expTime = json.at("token_exp").i();
      YuString nickorig(json.at("nickorig").s());
      YuString nicksfx(json.at("nicksfx").s());

      YuStrMap meta;

      if (session.doGetMeta())
      {
        const YuParserVal::Dict& metaDict = json.at("meta").d();

        for (auto mi = metaDict.begin(), end = metaDict.end(); mi != end; ++mi)
          meta[mi->first] = mi->second.s();
      }

      session.onLogin(login, nick, userId, token, jwt, tags, lang, country, expTime, nickorig,
        nicksfx, meta);

      onOkStatus(json);

      return YU2_OK;
    }

    return onErrorStatus(json, status);
  }

  return YU2_FAIL;
}


//==================================================================================================
Yuplay2Status YuSession::CommonLoginAction::onErrorStatus(const YuJson& answer,Yuplay2Status status)
{
  if (status == YU2_FROZEN)
  {
    YuString error = answer.at("error").s();

    if (error.find("Account bruteforce") != error.npos)
      return YU2_FROZEN_BRUTEFORCE;
  }

  return status;
}


//==================================================================================================
void YuSession::CommonLoginAction::onHttpResponse(const YuString& url, const YuCharTab& data)
{
  done(parseServerAnswer(data));
}
