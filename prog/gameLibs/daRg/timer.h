#pragma once

#include <sqrat.h>
#include <generic/dag_tab.h>


namespace darg
{


class Timer
{
public:
  Timer(const Sqrat::Object &id_);

  void initOneShot(float timeout_, const Sqrat::Function &handler_);
  void initPeriodic(float period_, const Sqrat::Function &handler_);

  void update(float dt, Tab<Sqrat::Function> &cb_queue);
  bool isFinished() const;
  bool isLooped() const { return period > 0; }

public:
  float period;
  float timeout;

  Sqrat::Object id;
  Sqrat::Function handler;
};


} // namespace darg
