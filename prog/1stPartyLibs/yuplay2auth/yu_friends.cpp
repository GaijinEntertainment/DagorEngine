#include "stdafx.h"
#include "yu_friends.h"

//==================================================================================================
bool YuFriends::assign(const YuStrTable& map)
{
  for (YuStrTableCI row = map.begin(), end = map.end(); row != end; ++row)
  {
    if (row->size() < 2)
      return false;

#if YU2_WINAPI
    __int64 userId = ::_atoi64(row->at(0).c_str());
#elif YU2_POSIX
    int64_t userId = ::atoll(row->at(0).c_str());
#endif //YU2_WINDOWS

    Friend fr(userId, row->at(1));
    friends.push_back(fr);
  }

  return true;
}


//==================================================================================================
IYuplay2UserBase* YU2VCALL YuFriends::getUser(unsigned idx)
{
  if (idx < friends.size())
    return &friends[idx];

  return NULL;
}
