#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvRecalcHandler : public darg::Behavior
{
public:
  BhvRecalcHandler();

  virtual void onRecalcLayout(Element *);
};


extern BhvRecalcHandler bhv_recalc_handler;


} // namespace darg
