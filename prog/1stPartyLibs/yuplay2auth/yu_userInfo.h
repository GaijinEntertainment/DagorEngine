#ifndef __YUPLAY2_UNSER_INFO_IMPL__
#define __YUPLAY2_UNSER_INFO_IMPL__
#pragma once


#include <yuplay2_user.h>

#include "yu_binary.h"


class YuBasicUserInfo : public IYuplay2UserInfo
{
public:
  YuBasicUserInfo(const YuString& user_id, const YuString& login, const YuString& nick,
                  const YuString& token, const YuString& tags, const YuString& nickorig,
                  const YuString& nicksfx, const YuStrMap& meta);

private:
  int64_t userId;
  YuString login;
  YuString nick;
  YuString nickorig;
  YuString nicksfx;
  YuString token;
  YuStringSet tags;
  unsigned payMethods;
  YuStrMap meta;
  YuStrMap::const_iterator curMeta;

  void YU2VCALL free() override { delete this; }

  int64_t YU2VCALL getId() override { return userId; }
  const char* YU2VCALL getNick() override { return nick.c_str(); }
  const char* YU2VCALL getOriginalNick() override { return nickorig.c_str(); }
  const char* YU2VCALL getNickSuffix() override { return nicksfx.c_str(); }
  const char* YU2VCALL getLogin() override { return login.c_str(); }
  const char* YU2VCALL getToken() override { return token.c_str(); }
  IYuplay2Strings* YU2VCALL getTags() override { return new YuSessionStrings(tags); }
  unsigned YU2VCALL getPaymentMethods() override { return payMethods; }
  void YU2VCALL resetMetaIterator() override { curMeta = meta.begin(); }
  bool YU2VCALL getNextMeta(const char*& key, const char*& val) override;
};

#endif //__YUPLAY2_UNSER_INFO_IMPL__
