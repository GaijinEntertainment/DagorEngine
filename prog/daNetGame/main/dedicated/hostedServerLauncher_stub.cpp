// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/hostedServerLauncher.h"
void bind_hosting_internal_server(Sqrat::Table &) {}
void prelaunch_internal_server_if_needed() {}
void shutdown_internal_server_on_host_exit() {}
void kill_internal_server(bool) {}
bool is_hosted_internal_server_active() { return false; }
bool try_begin_hosted_server_start() { return false; }
void clear_hosted_server_start_pending() {}
bool is_hosted_server_start_pending() { return false; }
bool poll_manual_ready_watchdog() { return false; }
void hosted_internal_server_management_update() {}
