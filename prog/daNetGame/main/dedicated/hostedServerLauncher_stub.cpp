// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/hostedServerLauncher.h"
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>

void bind_hosting_internal_server(Sqrat::Table &) {}
void prelaunch_internal_server_if_needed() {}
void shutdown_internal_server_on_host_exit() {}
void kill_internal_server(bool) {}
void kill_internal_server_uid(HostedServerUid, bool) {}
void cancel_scheduled_internal_server_start() {}
bool is_hosted_internal_server_active() { return false; }
HostedServerUid get_hosted_internal_server_uid() { return HOSTED_SERVER_UID_NONE; }
HostedServerUid allocate_hosted_server_uid() { return HOSTED_SERVER_UID_NONE; }
void set_hosted_server_start_uid(HostedServerUid) {}
HostedServerUid resolve_hosted_server_start_uid(HostedServerUid) { return HOSTED_SERVER_UID_NONE; }
bool try_begin_hosted_server_start(HostedServerUid) { return false; }
void clear_hosted_server_start_pending() {}
bool is_hosted_server_start_pending() { return false; }
bool poll_hosted_ready_watchdog() { return false; }
void hosted_internal_server_management_update() {}
void set_mirror_hosted_dedic_logs(bool) {}
bool get_mirror_hosted_dedic_logs() { return false; }

void __cdecl default_hosted_server_log_forwarder(int, const char *, const char *, int) {}
void set_hosted_log_mirror(void *, bool) {}
void *get_hosted_log_mirror() { return (void *)&default_hosted_server_log_forwarder; }
bool hosted_log_mirror_expects_dll_override() { return false; }

#if DAGOR_HOSTED_INTERNAL_SERVER
// Real defs in dedicated_dll__exports.cpp
#else
void hosted_server_arm_log_mirror_callback() {}
void hosted_server_set_ready_required(HostedReadyFlags) {}
void hosted_server_signal_ready(HostedReadyFlags) {}
#endif

void stamp_test_log_uid_if_present()
{
  if (const char *uid = ::dgs_get_argv(TEST_LOG_UID_ARG))
    debug("%s%s", TEST_LOG_UID_STAMP_PREFIX, uid);
}

bool is_host_identity_uid_argv(const char *) { return false; }
void append_relayed_test_log_uid_arg(DataBlock &) {}
