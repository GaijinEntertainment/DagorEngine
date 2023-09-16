#include <quirrel/clevertap/clevertap_binding.h>
#include <quirrel/sqModules/sqModules.h>
#include <clevertap/clevertap.h>

namespace bindquirrel
{
void register_clevertap_module(SqModules *module_mgr)
{
  ///@module clevertap
  Sqrat::Table clevertap(module_mgr->getVM());
  // clang-format off
  clevertap
    .Func("clevertapLogEvent", clevertap::clevertapLogEvent)
    .Func("clevertapLogError", clevertap::clevertapLogError)
  ;
  // clang-format on
  module_mgr->addNativeModule("clevertap", clevertap);
  ///@resetscope
}
} // namespace bindquirrel
