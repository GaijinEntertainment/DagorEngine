// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <daECS/core/entityManager.h>
#include <daScript/daScript.h>
#include <dag/dag_vector.h>
#include <dasModules/dasModulesCommon.h>
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <string.h>
#include <util/dag_delayedAction.h>

#include "main/hostedServerLauncher.h"
#include "net/dedicated.h"
#include "net/net.h"

namespace bind_dascript
{

inline bool is_hosted_internal_server_active() { return ::is_hosted_internal_server_active(); }

inline HostedServerUid get_hosted_internal_server_uid() { return ::get_hosted_internal_server_uid(); }

inline HostedServerUid allocate_hosted_server_uid() { return ::allocate_hosted_server_uid(); }

inline void set_hosted_server_start_uid(HostedServerUid uid) { ::set_hosted_server_start_uid(uid); }

inline bool is_main_thread_network() { return ::is_main_thread_network(); }

//! Copies each live host status under lock into one shared string, then invokes
//! ``block``. Block arg is string# (TTemporary): valid only for the call.
inline void for_each_work_status(
  const das::TBlock<void, const das::TTemporary<const char *>> &block, das::Context *ctx, das::LineInfoArg *at)
{
  struct Ctx
  {
    const das::TBlock<void, const das::TTemporary<const char *>> *block;
    das::Context *ctx;
    das::LineInfoArg *at;
  } c{&block, ctx, at};
  for_each_hosted_work_status(
    [](const char *s, void *p) {
      auto *c = (Ctx *)p;
      vec4f arg = das::cast<const char *>::from(s);
      c->ctx->invoke(*c->block, &arg, nullptr, c->at);
    },
    &c);
}

inline bool request_start_hosted_server(const das::TArray<char *> &cmds,
  const char *main_das_path,
  const char *circuit,
  HostedServerUid uid,
  const char *scene,
  int tickrate,
  const char *game_execution_mode,
  int max_connections,
  das::Context * /*ctx*/,
  das::LineInfoArg * /*at*/)
{
  dag::Vector<eastl::string> argv;
  argv.reserve(cmds.size + 15);
  if (main_das_path && *main_das_path)
    argv.emplace_back(main_das_path);
  if (circuit && *circuit)
  {
    eastl::string s = "-config:circuit:t=";
    s += circuit;
    argv.emplace_back(eastl::move(s));
  }

  auto add_cfg_b = [&](const char *path, bool v) {
    argv.emplace_back(eastl::string(eastl::string::CtorSprintf(), "-config:%s:b=%s", path, v ? "yes" : "no"));
  };

  const DataBlock &hostDebug = *dgs_get_settings()->getBlockByNameEx("debug");
  add_cfg_b("debug/allowUnsafeDasCode", hostDebug.getBool("allowUnsafeDasCode", false));
  if (hostDebug.paramExists("useAddonVromSrc"))
    add_cfg_b("debug/useAddonVromSrc", hostDebug.getBool("useAddonVromSrc", false));
  if (hostDebug.paramExists("vromfsFirstPriority"))
    add_cfg_b("debug/vromfsFirstPriority", hostDebug.getBool("vromfsFirstPriority", false));
  add_cfg_b("game_das_enable_serialization", dgs_get_settings()->getBool("game_das_enable_serialization", false));

  // Project scene/tickrate/mode are typed params so project cmds stay free of
  // -config:*.
  if (scene && *scene)
    argv.emplace_back(eastl::string(eastl::string::CtorSprintf(), "-config:scene:t=%s", scene));
  if (tickrate > 0)
    argv.emplace_back(eastl::string(eastl::string::CtorSprintf(), "-config:tickrate:i=%d", tickrate));
  if (game_execution_mode && *game_execution_mode)
    argv.emplace_back(eastl::string(eastl::string::CtorSprintf(), "-config:gameExecutionMode:t=%s", game_execution_mode));
  // DLL reads LISTEN_MAX_CONNECTIONS_ARG before settings load (net_init_early /
  // UserNet listen).
  if (max_connections > 0)
    argv.emplace_back(eastl::string(eastl::string::CtorSprintf(), dedicated::LISTEN_MAX_CONNECTIONS_STAMP_FMT, max_connections));

  if (::is_main_thread_network())
    argv.emplace_back("-force_main_net");
  else
    argv.emplace_back("-force_user_net");

  for (uint32_t i = 0; i < cmds.size; ++i)
  {
    const char *c = cmds[i] ? cmds[i] : "";
    if (strncmp(c, "-config:", 8) == 0)
    {
      logwarn("request_start_hosted_server: dropping project cmd '%s' (host "
              "stamps -config)",
        c);
      continue;
    }
    argv.emplace_back(c);
  }

  uid = resolve_hosted_server_start_uid(uid);
  argv.emplace_back(eastl::string(eastl::string::CtorSprintf(), "%s%llu", TEST_SERVER_LOG_UID_STAMP_PREFIX, (unsigned long long)uid));

  if (!try_begin_hosted_server_start(uid))
  {
    logwarn("request_start_hosted_server: refused (start already pending) uid=%llu", (unsigned long long)uid);
    return false;
  }

  run_action_on_main_thread([argv = eastl::move(argv)]() mutable {
    if (!is_hosted_server_start_pending())
      return;
    ecs::List<ecs::string> cmdList;
    cmdList.reserve(argv.size());
    for (const eastl::string &a : argv)
      cmdList.emplace_back(a.c_str());
    g_entity_mgr->broadcastEvent(EventHostedInternalServerToStart{eastl::move(cmdList)});
  });
  return true;
}

inline void request_stop_hosted_server(HostedServerUid uid, das::Context *, das::LineInfoArg *)
{
  run_action_on_main_thread([uid]() { g_entity_mgr->broadcastEvent(EventHostedInternalServerToStop{uid}); });
}

} // namespace bind_dascript
