#include "stdafx.h"
#include "yu_sessionAction.h"


//==================================================================================================
YuSession::AsyncAction::AsyncAction(YuSession& session_, ActionId id_, Yuplay2Msg msg_,
                                    IYuplay2Cb* cb_) :
    session(session_), id(id_), msg(msg_), cb(cb_)
{
  doneEvt.set(); //Done by default. run() must change this event state
}


//==================================================================================================
void YuSession::AsyncAction::beginAction()
{
  doneEvt.reset();
  worked = true;
}


//==================================================================================================
void YuSession::AsyncAction::done(Yuplay2Status st, const void* res)
{
  status = st;
  result = res;

  if (cb)
    cb->onMessage(msg, this, status, result);
}
