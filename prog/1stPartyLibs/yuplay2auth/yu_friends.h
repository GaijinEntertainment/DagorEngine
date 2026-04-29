#ifndef __YUPLAY2_FRIENDS_IMPL__
#define __YUPLAY2_FRIENDS_IMPL__
#pragma once


#include <yuplay2_friends.h>


class YuFriends : public IYuplay2Friends
{
public:
  bool assign(const YuStrTable& map);
  
private:
  class Friend : public IYuplay2UserBase
  {
  public:
    Friend(int64_t id_, const YuString& nick_) : id(id_), nick(nick_) {}

  private:
    YuString nick;
    int64_t id;

    virtual int64_t YU2VCALL getId() { return id; }
    virtual const char* YU2VCALL getNick() { return nick.c_str(); }
  };

  eastl::vector<Friend> friends;
  
  virtual void YU2VCALL free() { delete this; }

  virtual unsigned YU2VCALL count() { return (unsigned)friends.size(); }
  virtual IYuplay2UserBase* YU2VCALL getUser(unsigned idx);
};


#endif //__YUPLAY2_FRIENDS_IMPL__
