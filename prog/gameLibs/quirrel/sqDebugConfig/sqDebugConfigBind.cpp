// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqDebugConfig/sqDebugConfig.h>
#include <quirrel/sqModules/sqModules.h>

namespace debug_config
{
static bool is_screenshot_applied = false;
void raise_screenshot_apply_flag() { is_screenshot_applied = true; }
bool was_screenshot_applied_to_config() { return is_screenshot_applied; }
void register_debug_config(SqModules *module_mgr)
{
  Sqrat::Table debugConfig(module_mgr->getVM());

  debugConfig.Func("was_screenshot_applied_to_config", was_screenshot_applied_to_config);

  module_mgr->addNativeModule("debug.config", debugConfig);
}
} // namespace debug_config