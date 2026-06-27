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


namespace bind_dascript
{

#if DAGOR_HOSTED_INTERNAL_SERVER
extern "C" void hosted_server_signal_ready();
extern "C" void hosted_server_disable_auto_ready();
inline void signal_hosted_server_ready() { hosted_server_signal_ready(); }
inline void disable_auto_hosted_server_ready() { hosted_server_disable_auto_ready(); }
#else
inline void signal_hosted_server_ready() {}
inline void disable_auto_hosted_server_ready() {}
#endif

inline bool is_hosted_internal_server_active() { return ::is_hosted_internal_server_active(); }

inline bool is_hosted_server_manual_ready_enabled()
{
  return dgs_get_settings()->getBlockByNameEx("debug")->getBool("hostedServerManualReady", false);
}

inline void request_start_hosted_server(const das::TArray<char *> &cmds, das::Context *, das::LineInfoArg *)
{
  if (!try_begin_hosted_server_start())
    return;

  dag::Vector<eastl::string> argv;
  argv.reserve(cmds.size);
  for (uint32_t i = 0; i < cmds.size; ++i)
    argv.emplace_back(cmds[i] ? cmds[i] : "");

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
