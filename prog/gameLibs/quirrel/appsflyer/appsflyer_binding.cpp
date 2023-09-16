#include <quirrel/appsflyer/appsflyer_binding.h>
#include <quirrel/sqModules/sqModules.h>
#include <appsflyer/appsflyer.h>

namespace bindquirrel
{
void register_appsflyer_module(SqModules *module_mgr)
{
  ///@module appsFlyer
  Sqrat::Table appsFlyer(module_mgr->getVM());
  // clang-format off
  appsFlyer
    .Func("logEvent", appsflyer::logAppsFlyerEvent)
    .Func("getAppsFlyerUID", appsflyer::getAppsFlyerUID)
    .Func("setAppsFlyerCUID", appsflyer::setAppsFlyerCUID)
  ;
  // clang-format on
  module_mgr->addNativeModule("appsFlyer", appsFlyer);
  ///@resetscope
}
} // namespace bindquirrel
