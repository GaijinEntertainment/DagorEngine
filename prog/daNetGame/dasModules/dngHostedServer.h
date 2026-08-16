// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <daECS/core/entityManager.h>
#include <util/dag_delayedAction.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>

#include "main/hostedServerLauncher.h"


#if DAGOR_HOSTED_INTERNAL_SERVER
void hosted_server_signal_ready();
void hosted_server_disable_auto_ready();
#endif

namespace bind_dascript
{

#if DAGOR_HOSTED_INTERNAL_SERVER
inline void signal_hosted_server_ready() { ::hosted_server_signal_ready(); }
inline void disable_auto_hosted_server_ready() { ::hosted_server_disable_auto_ready(); }
#else
inline void signal_hosted_server_ready() {}
inline void disable_auto_hosted_server_ready() {}
#endif

inline bool is_hosted_internal_server_active() { return ::is_hosted_internal_server_active(); }

inline bool should_server_invoke_ready_manually()
{
  return dgs_get_settings()->getBlockByNameEx("debug")->getBool("hostedServerManualReady", false);
}

inline void request_start_hosted_server(const das::TArray<char *> &cmds,
  const char *main_das_path,
  const char *circuit,
  bool allow_unsafe_das_code,
  bool manual_ready,
  das::Context *ctx,
  das::LineInfoArg *at)
{
  // Compose argv in the order the child expects: main-das path first (positional), then
  // wrapper-controlled -config:* flags, then project-supplied cmds. Project cmds are filtered
  // so they cannot contain any `-config:*` -- engine settings overrides go through the typed
  // parameters, and project code can't sneak in privileged flags (allowUnsafeDasCode etc.)
  // bypassing the parent-state gate.
  static constexpr const char kEnableSerializationFlag[] = "-config:game_das_enable_serialization";
  dag::Vector<eastl::string> argv;
  argv.reserve(cmds.size + 5);
  if (main_das_path && *main_das_path)
    argv.emplace_back(main_das_path);
  if (circuit && *circuit)
  {
    eastl::string s = "-config:circuit:t=";
    s += circuit;
    argv.emplace_back(eastl::move(s));
  }
  if (allow_unsafe_das_code)
    argv.emplace_back("-config:debug/allowUnsafeDasCode:b=yes");
  if (manual_ready)
    argv.emplace_back("-config:debug/hostedServerManualReady:b=yes");
  // Host owns this policy (from host settings); reject project cmds that would clobber it.
  const bool enable_serialization = dgs_get_settings()->getBool("game_das_enable_serialization", false);
  {
    eastl::string s = kEnableSerializationFlag;
    s += enable_serialization ? ":b=yes" : ":b=no";
    argv.emplace_back(eastl::move(s));
  }
  for (uint32_t i = 0; i < cmds.size; ++i)
  {
    const char *c = cmds[i] ? cmds[i] : "";
    // Only `-config:debug/*` is the privilege-escalation vector -- everything under `debug/`
    // is controlled by the parent (allowUnsafeDasCode, hostedServerManualReady, etc.) and
    // must go through the typed params. Other `-config:*` (tickrate, scene, gameExecutionMode)
    // is legitimate game-side tuning; pass through unchanged.
    if (strncmp(c, "-config:debug/", 14) == 0 || strncmp(c, kEnableSerializationFlag, sizeof(kEnableSerializationFlag) - 1) == 0)
    {
      ctx->throw_error_at(at,
        "request_start_hosted_server: project-supplied '%s' is not allowed; "
        "privileged host flags must go through the typed parameters / host settings",
        c);
      return;
    }
    argv.emplace_back(c);
  }

  if (!try_begin_hosted_server_start())
    return;

  run_action_on_main_thread([argv = eastl::move(argv)]() mutable {
    if (!is_hosted_server_start_pending())
      return;
    ecs::List<ecs::string> cmdList;
    cmdList.reserve(argv.size());
    for (const eastl::string &a : argv)
      cmdList.emplace_back(a.c_str());
    g_entity_mgr->broadcastEvent(EventHostedInternalServerToStart{eastl::move(cmdList)});
  });
}

inline void request_stop_hosted_server(das::Context *, das::LineInfoArg *)
{
  clear_hosted_server_start_pending();
  run_action_on_main_thread([]() { g_entity_mgr->broadcastEvent(EventHostedInternalServerToStop{}); });
}

} // namespace bind_dascript
