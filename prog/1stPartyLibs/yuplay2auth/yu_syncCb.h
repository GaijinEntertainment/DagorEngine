#ifndef __YUPLAY2_SYNC_CB__
#define __YUPLAY2_SYNC_CB__
#pragma once


#include <yuplay2_callback.h>

#include "yu_event.h"


class YuSyncCb : public IYuplay2Cb
{
public:
  YuSyncCb() : status(YU2_FAIL) {}

  void wait() { doneEvent.wait(); }

protected:
  virtual void onData(const void* data) {}

private:
  int status;
  YuEvent doneEvent;

  virtual void YU2VCALL onMessage(int msg, Yuplay2Handle req, int status, const void* data);
};


#endif //__YUPLAY2_SYNC_CB__
