//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>

namespace Sqrat
{
class Table;
}
class SqModules;

namespace bindquirrel
{
void register_reg_exp(SqModules *module_mgr);
void register_utf8(SqModules *module_mgr);

void bind_dagor_time(SqModules *module_mgr);

void bind_dagor_workcycle(SqModules *module_mgr, bool auto_update_from_idle_cycle);
void cleanup_dagor_workcycle_module(HSQUIRRELVM vm);
void dagor_workcycle_frame_update(HSQUIRRELVM vm);
void dagor_workcycle_skip_update(HSQUIRRELVM vm);
void clear_workcycle_actions(HSQUIRRELVM vm);

void sqrat_bind_dagor_math(SqModules *module_mgr);
void sqrat_bind_dagor_logsys(SqModules *module_mgr, bool console_mode = false);
// Adds reference to bound_class
void sqrat_bind_datablock(SqModules *module_mgr, bool allow_file_access = true);
void register_random(SqModules *module_mgr);
void register_hash(SqModules *module_mgr);

void register_dagor_system(SqModules *module_mgr);
void register_dagor_shell(SqModules *module_mgr);
void register_dagor_fs_module(SqModules *module_mgr);
void register_dagor_fs_vrom_module(SqModules *module_mgr);
void register_dagor_clipboard(SqModules *module_mgr);
void register_dagor_localization_module(SqModules *module_mgr);
void register_platform_module(SqModules *module_mgr);

void register_iso8601_time(SqModules *module_mgr);


typedef void (*SqExitFunctionPtr)(int);
void set_sq_exit_function_ptr(SqExitFunctionPtr func);

void setup_logerr_interceptor();
void clear_logerr_interceptors(HSQUIRRELVM vm);
void term_logerr_interceptor();

void apply_compiler_options_from_game_settings(SqModules *module_mgr);

} // namespace bindquirrel
