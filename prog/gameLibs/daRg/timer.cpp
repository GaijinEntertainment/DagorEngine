// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "timer.h"

#include <util/dag_globDef.h>

namespace darg
{

Timer::Timer(const Sqrat::Object &id_) : period(0), timeout(0), id(id_) {}


void Timer::initOneShot(float timeout_, const Sqrat::Function &handler_)
{
  period = 0;
  timeout = timeout_;
  handler = handler_;
}


void Timer::initPeriodic(float period_, const Sqrat::Function &handler_)
{
  timeout = period = period_;
  handler = handler_;
}


void Timer::update(float dt, Tab<Sqrat::Function> &cb_queue)
{
  timeout -= dt;
  if (timeout <= 0.0f)
  {
    G_ASSERT(!handler.IsNull());
    if (!handler.IsNull())
      cb_queue.push_back(handler);

    if (isLooped())
      timeout = period;
  }
}


bool Timer::isFinished() const { return !isLooped() && (timeout <= 0.0f); }


} // namespace darg
