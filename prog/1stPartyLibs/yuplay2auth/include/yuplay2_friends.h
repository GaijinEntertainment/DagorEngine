#ifndef __YUPLAY2_FRIENDS__
#define __YUPLAY2_FRIENDS__
#pragma once


#include <yuplay2_user.h>


struct IYuplay2Friends
{
  virtual void YU2VCALL free() = 0;

  virtual unsigned YU2VCALL count() = 0;
  virtual IYuplay2UserBase* YU2VCALL getUser(unsigned idx) = 0;
};


#endif //__YUPLAY2_FRIENDS__
