// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <contentUpdater/version.h>

typedef struct SQVM *HSQUIRRELVM;
namespace Sqrat
{
class Table;
}
class SqModules;

namespace gamescripts
{
enum class ReloadMode
{
  FULL_RELOAD,
  SILENT_FULL_RELOAD, // Do not emit any events. Just reload the scripts.
};

HSQUIRRELVM init();
bool run();
bool run_with_serialization();
void load_sq_module(const char *fn, bool is_entrypoint);
bool reload_sq_modules(ReloadMode mode = ReloadMode::FULL_RELOAD);
void reload_das_init();
bool reload_das_modules();
bool main_thread_post_load();
void set_das_aot(bool value);
bool get_das_aot();
void set_das_log_aot_errors(bool value);
bool get_das_log_aot_errors();
int set_load_threads_num(int num);
int get_game_das_reload_threads();
int get_load_threads_num();
void update_deferred();
void collect_das_garbage();
bool reload_changed();
void collect_garbage();
void shutdown_before_ecs();
void shutdown();
HSQUIRRELVM get_vm();
SqModules *get_module_mgr();

bool run_user_game_mode_entry_scripts(const char *das_entry_fn, const char *sq_entry_fn);
void unload_user_game_mode_scripts(const char *fn_prefix);

// internal game mods, without restrictions
bool run_game_mode_entry_scripts(const char *das_entry_fn);
bool unload_game_mode_scripts();
void unload_all_das_scripts_without_debug_agents();
void das_load_ecs_templates();

// Versions of the scripts.
// The updater will use this function in order to reload scripts
// in case of changed version.
// Reload took place after leaving a session.
void set_sq_version(updater::Version version);
void set_das_version(updater::Version version);
updater::Version get_sq_version();
updater::Version get_das_version();
} // namespace gamescripts
