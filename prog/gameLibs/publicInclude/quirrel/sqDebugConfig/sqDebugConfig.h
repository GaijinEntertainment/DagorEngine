#pragma once

class SqModules;

namespace debug_config
{
void raise_screenshot_apply_flag();
bool was_screenshot_applied_to_config();
void register_debug_config(SqModules *module_mgr);
} // namespace debug_config
