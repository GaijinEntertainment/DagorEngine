#ifndef __YUPLAY2_SESSION_PROC_ACTION__
#define __YUPLAY2_SESSION_PROC_ACTION__
#pragma once


#include "yu_session.h"
#include "yu_answer.h"


class YuSession::AsyncAction
{
public:
  AsyncAction(YuSession& session, ActionId id, Yuplay2Msg msg, IYuplay2Cb* cb);

  virtual ~AsyncAction() {}

  virtual void stop() = 0;

  bool running() { return !doneEvt.check(); }
  bool completed() const { return doneEvt.check() && worked; }
  bool wait() { return doneEvt.wait(); }

  ActionId getId() const { return id; }

  Yuplay2Status getStatus() const { return status; }
  const void* getResult() const { return result; }
  IYuplay2Answer* getAnswer() { return &answer; }

protected:
  YuSession& session;
  ActionId id;
  Yuplay2Msg msg;
  YuEvent doneEvt;
  YuApiAnswer answer;

  void beginAction();
  void done(Yuplay2Status status, const void* result = NULL);

private:
  volatile bool worked = false;
  const void* result = NULL;
  IYuplay2Cb* cb = NULL;
  Yuplay2Status status = YU2_FAIL;
};


#endif //__YUPLAY2_SESSION_PROC_ACTION__
