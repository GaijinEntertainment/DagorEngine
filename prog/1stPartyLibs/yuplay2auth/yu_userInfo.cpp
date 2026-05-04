#include "stdafx.h"
#include "yu_userInfo.h"
#include "yu_string.h"

#include <yuplay2_session.h>


//==================================================================================================
YuBasicUserInfo::YuBasicUserInfo(const YuString& user_id, const YuString& login_,
                                 const YuString& nick_, const YuString& tok, const YuString& tags_,
                                 const YuString& norig, const YuString& nsfx,
                                 const YuStrMap& meta_) :
    payMethods(YU2_PAY_GJN), login(login_), nick(nick_), token(tok), nickorig(norig), nicksfx(nsfx),
    meta(meta_)
{
  struct PayTag
  {
    const char* tag;
    int flag;
  };

  static PayTag payTags[] =
  {
    { "autopay_qiwi", YU2_PAY_QIWI },
    { "autopay_yam",  YU2_PAY_YANDEX },
    { "autopay_pp",   YU2_PAY_PAYPAL },
    { "autopay_wm",   YU2_PAY_WEBMONEY },
    { "autopay_amzn", YU2_PAY_AMAZON },
    { "autopay_brntree", YU2_PAY_BRNTREE },
  };


#if YU2_WINAPI
  userId = ::_atoi64(user_id.c_str());
#elif YU2_POSIX
  userId = ::atoll(user_id.c_str());
#endif //YU2_WINAPI

  tags = YuStr::explodeSet(tags_, ',');

  for (int i = 0; i < sizeof(payTags) / sizeof(payTags[0]); ++i)
    if (tags.find(payTags[i].tag) != tags.end())
      payMethods |= payTags[i].flag;
}


//==================================================================================================
bool YU2VCALL YuBasicUserInfo::getNextMeta(const char*& key, const char*& val)
{
  if (curMeta == meta.end())
    return false;

  key = curMeta->first.c_str();
  val = curMeta->second.c_str();

  ++curMeta;

  return true;
}
