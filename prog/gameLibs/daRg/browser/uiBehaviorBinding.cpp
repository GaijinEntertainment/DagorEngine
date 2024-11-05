// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvBrowser.h"
#include <bindQuirrelEx/autoBind.h>
#include <sqModules/sqModules.h>

namespace darg
{

void bind_browser_behavior(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Class<BhvBrowserData, Sqrat::NoConstructor<BhvBrowserData>> sqBhvBrowserDataname(vm, "BhvBrowserData");

  Sqrat::Table tblBhv(vm);

  tblBhv.SetValue("Browser", (darg::Behavior *)&bhv_browser);

  module_mgr->addNativeModule("browser.behaviors", tblBhv);
}

} // namespace darg
