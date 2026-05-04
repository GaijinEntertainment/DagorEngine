// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/adjust/adjust_binding.h>
#include <sqmodules/sqmodules.h>
#include <adjust/adjust.h>

namespace bindquirrel
{
void register_adjust_module(SqModules *module_mgr)
{
  ///@module adjust
  Sqrat::Table adjust(module_mgr->getVM());
  adjust //
    .Func("startAdjust", adjust::start)
    .Func("setAdjustId", adjust::setId)
    .Func("setAdjustStore", adjust::setStore)
    .Func("logAdjustEvent", adjust::logEvent)
    .Func("logAdjustAdRevenue", adjust::logAdRevenue)
    .Func("setEnableAdjust", adjust::setEnable)
    .Func("setOnlineAdjust", adjust::setOnline)
    /**/;
  module_mgr->addNativeModule("adjust", adjust);
  ///@resetscope
}
} // namespace bindquirrel
