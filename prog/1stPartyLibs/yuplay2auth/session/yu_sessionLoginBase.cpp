#include "stdafx.h"
#include "yu_sessionLoginBase.h"
#include "yu_string.h"


//==================================================================================================
Yuplay2Status YuSession::CommonLoginAction::parseServerAnswer(const YuCharTab& data)
{
  Yuplay2Status status = AsyncHttpAction::parseServerJson(data);

  if (status == YU2_OK)
  {
    YuString tags = answer.at("tags").s();

    if (psnRestricted)
    {
      YuStringSet tagsSet = YuStr::explodeSet(tags, ',');

      if (tagsSet.find("psnrestricted") != tagsSet.end())
        return YU2_PSN_RESTRICTED;
    }

    YuString login(answer.at("login").s());
    YuString nick(answer.at("nick").s());
    YuString userId(answer.at("user_id").s());
    YuString token(answer.at("token").s());
    YuString jwt(answer.at("jwt").s());
    YuString lang(answer.at("lang").s());
    YuString country(answer.at("country").s());
    int64_t expTime = answer.at("token_exp").i();
    YuString nickorig(answer.at("nickorig").s());
    YuString nicksfx(answer.at("nicksfx").s());

    YuStrMap meta;

    if (session.doGetMeta())
    {
      const YuParserVal::Dict& metaDict = answer.at("meta").d();

      for (auto mi = metaDict.begin(), end = metaDict.end(); mi != end; ++mi)
        meta[mi->first] = mi->second.s();
    }

    session.onLogin(login, nick, userId, token, jwt, tags, lang, country, expTime, nickorig,
      nicksfx, meta);

    onOkStatus(answer);

    return YU2_OK;
  }

  return onErrorStatus(status);
}


//==================================================================================================
Yuplay2Status YuSession::CommonLoginAction::onErrorStatus(Yuplay2Status status)
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
