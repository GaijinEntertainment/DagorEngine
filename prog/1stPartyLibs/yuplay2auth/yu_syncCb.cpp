#include "stdafx.h"
#include "yu_syncCb.h"


//==================================================================================================
void YU2VCALL YuSyncCb::onMessage(int msg, Yuplay2Handle req, int status, const void* data)
{
  this->status = status;
  onData(data);
  
  doneEvent.set();
}