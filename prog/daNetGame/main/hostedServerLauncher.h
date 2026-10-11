// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <daECS/core/event.h>
#include <generic/dag_enumBitMask.h>
#include <stdint.h>
#include <util/dag_globDef.h>
#include <util/dag_safeArg.h>
#include <util/dag_string.h>

class DataBlock;

namespace Sqrat
{
class Table;
}

// Monotonic HIS instance id. 0 = none. Log stamps use the decimal form.
using HostedServerUid = uint64_t;
inline constexpr HostedServerUid HOSTED_SERVER_UID_NONE = 0;

ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerDidStart, HostedServerUid /*uid*/);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerDidStop, HostedServerUid /*uid*/);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerToStart, ecs::List<ecs::string> /*cmd*/);
// A late stop must not take down a newer start.
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerToStop, HostedServerUid /*uid*/);

void bind_hosting_internal_server(Sqrat::Table &ns);
void shutdown_internal_server_on_host_exit();
void schedule_new_internal_server_with_args(int external_argc, char **external_argv);
void kill_internal_server(bool wait);
// No-op when uid is NONE or not the current HIS. Process-exit paths keep
// untargeted kill.
void kill_internal_server_uid(HostedServerUid uid, bool wait);
// Drops pending relaunch. Pair with kill_internal_server() on app-stopped
// paths.
void cancel_scheduled_internal_server_start();
void prelaunch_internal_server_if_needed();

bool is_hosted_internal_server_active();
// Pending/scheduled uid wins over a still-current dying HIS.
HostedServerUid get_hosted_internal_server_uid();
HostedServerUid allocate_hosted_server_uid();
void set_hosted_server_start_uid(HostedServerUid uid);
// passed=NONE -> preset or allocate.
HostedServerUid resolve_hosted_server_start_uid(HostedServerUid passed);

// True unless a start is already pending. A live or dying HIS does not block.
bool try_begin_hosted_server_start(HostedServerUid uid);
void clear_hosted_server_start_pending();
bool is_hosted_server_start_pending();

bool poll_hosted_ready_watchdog();

void hosted_internal_server_management_update();

// HIS file-log mirror (daNetGame default path, no Eden).
// Host stores a forwarder into the DLL; DagorWinMain arms an in-DLL on_log
// above visuallog that chains prev then calls the host forwarder (default
// signature below).
//
// Eden override: set_hosted_log_mirror(eden_fn,
// /*dll_side_override_expect_later*/ true). Launcher passes that flag into the
// DLL so default arm is skipped. Eden dedic does not install another on_log:
// its existing console_output_listener casts the stored void* to the Eden host
// signature (with from_das) and calls it.
typedef void(__cdecl *HostedServerLogForwarder)(int lev_tag, const char *message, const char *filename, int code_line);

void __cdecl default_hosted_server_log_forwarder(int lev_tag, const char *message, const char *filename, int code_line);
// Host-side registration. fn is HostedServerLogForwarder when
// expect_later=false; otherwise an opaque override (Eden-typed).
// dll_side_override_expect_later: skip DLL default arm.
void set_hosted_log_mirror(void *fn, bool dll_side_override_expect_later = false);
void *get_hosted_log_mirror();
bool hosted_log_mirror_expects_dll_override();

//! Host Tab of status pointers (nullptr = free, max 4). Scope set/unset via
//! this callback.
void __cdecl default_hosted_work_status_callback(bool set, const char *text);
//! ``s`` is borrowed and valid only for the duration of ``fn``.
void for_each_hosted_work_status(void (*fn)(const char *s, void *ctx), void *ctx);

#if DAGOR_HOSTED_INTERNAL_SERVER
// Dedic: opaque host forwarder stored in the DLL (cast to
// HostedServerLogForwarder or Eden type).
void *hosted_server_get_log_forwarder();
// Log-side stamp (decimal of HostedServerUid, or host argv override).
void hosted_server_store_instance_uid(const char *uid);
#endif
void hosted_server_arm_log_mirror_callback(); // HIS DLL: after
                                              // visual_err_log_setup (default
                                              // path only)

// Hosted-ready latch: each path signals a flag; ready when (got & required) ==
// required. Default required = Entities (DNG). Callers set the mask with
// hosted_server_set_ready_required.
enum class HostedReadyFlags : uint32_t
{
  None = 0,
  Entities = 1u << 0,
  Scripts = 1u << 1,
};
DAGOR_ENABLE_ENUM_BITMASK(HostedReadyFlags)

// After Eden Game on_initialize (and net-command publish). Latch ES signals
// Scripts.
ECS_BROADCAST_EVENT_TYPE(OnEdenScriptsInitialized);

void hosted_server_set_ready_required(HostedReadyFlags mask);
void hosted_server_signal_ready(HostedReadyFlags flags);

inline constexpr const char HOSTED_DEDIC_LOG_TAG[] = "[ded]";
inline void format_hosted_dedic_log_line(String &out, const char *message)
{
  out.printf(0, "%s %s", HOSTED_DEDIC_LOG_TAG, message ? message : "");
}

// Format a debug_log_callback payload. anum >= 0 is DagorSafeArg[]; anum < 0 is
// va_list* (clogmessage / matching g_message).
inline void format_hosted_log_message(String &out, const char *fmt, const void *arg, int anum)
{
  if (anum >= 0)
  {
    out.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);
    return;
  }
  DAG_VACOPY(ap, *(va_list *)arg);
  out.cvprintf(0, fmt ? fmt : "", ap);
  DAG_VACOPYEND(ap);
}

// debug/mirrorHostedDedicLogs (applies on next HIS start). Default off; Eden
// sets its own default.
void set_mirror_hosted_dedic_logs(bool on);
bool get_mirror_hosted_dedic_logs();

// test_engine log discovery: first -test_log_uid line in a log wins.
inline constexpr const char TEST_LOG_UID_ARG[] = "test_log_uid";
inline constexpr const char TEST_SERVER_LOG_UID_ARG[] = "test_server_log_uid";
inline constexpr const char TEST_LOG_UID_STAMP_PREFIX[] = "-test_log_uid:";
inline constexpr const char TEST_SERVER_LOG_UID_STAMP_PREFIX[] = "-test_server_log_uid:";
void stamp_test_log_uid_if_present();
// True for host-only identity argv (-test_log_uid / -test_server_log_uid, : or
// =).
bool is_host_identity_uid_argv(const char *arg);
void append_relayed_test_log_uid_arg(DataBlock &start_params);