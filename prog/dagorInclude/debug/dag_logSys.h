//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_log.h>
#include <generic/dag_span.h>

#include <supp/dag_define_KRNLIMP.h>

typedef int (*debug_override_timestamp_cb_t)(char *dest, int dest_sz, int64_t t_msec);

struct LogMessage
{
  const char *fmt;
  const void *arg;
  const char *ctxFile;
  int levTag;
  int anum;
  int ctxLine;
  unsigned flags;
};

enum : unsigned
{
  LOGMSG_SHOWN_TO_USER = 1u << 0,
  LOGMSG_SKIP_FILE = 1u << 1,
  LOGMSG_STOP_HANDLERS = 1u << 2,
};

enum : unsigned
{
  LOGMASK_FATAL = 1u << LOGLEVEL_FATAL,
  LOGMASK_ERR = 1u << LOGLEVEL_ERR,
  LOGMASK_WARN = 1u << LOGLEVEL_WARN,
  LOGMASK_DEBUG = 1u << LOGLEVEL_DEBUG,
  LOGMASK_REMARK = 1u << LOGLEVEL_REMARK,
  LOGMASK_ANY_TAG = 1u << 31,
  LOGMASK_ERRORS = LOGMASK_FATAL | LOGMASK_ERR,
  LOGMASK_ALL_LEVELS = LOGMASK_FATAL | LOGMASK_ERR | LOGMASK_WARN | LOGMASK_DEBUG | LOGMASK_REMARK,
  LOGMASK_ALL = LOGMASK_ALL_LEVELS | LOGMASK_ANY_TAG,
};

enum LogHandlerOrder : int
{
  LOGORDER_FILTER = -100,
  LOGORDER_CRASH_REPORT = -50,
  LOGORDER_DEFAULT = 0,
  LOGORDER_SHOW_GAME = 100,
  LOGORDER_SHOW_SCRIPT = 110,
  LOGORDER_SHOW_EDITOR = 120,
  LOGORDER_SHOW_FALLBACK = 200,
  LOGORDER_SEND_REPORT = 500,
  LOGORDER_FILE_MIRROR = 1000,
};

inline constexpr int LOGTAG_D3D_ERROR = _MAKE4C('D3DE');
inline constexpr int LOGTAG_NET_EVENT = _MAKE4C('nEvt');

struct LogHandlerDesc
{
  unsigned levelMask = LOGMASK_ERRORS;
  int tags[4] = {};
  int order = LOGORDER_DEFAULT;
};

typedef void (*log_handler_t)(void *ctx, LogMessage &msg);

enum class LogHandlerId : uint32_t
{
  Invalid = 0
};

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
KRNLIMP const char *get_log_directory();
KRNLIMP const char *get_log_filename();
KRNLIMP void close_debug_files();
KRNLIMP void flush_debug_file();
KRNLIMP void start_debug_system(const char *exe_fname, const char *prefix = ".logs/", bool datetime_name = true);
KRNLIMP void start_classic_debug_system(const char *debug_fname, bool en_time = true, bool hhmmss_fmt = false, int hhmmss_subsec = 0,
  bool append_file = false);
KRNLIMP void debug_enable_timestamps(bool en_time, bool hhmmss_fmt = false, int hhmmss_subsec = 0);
KRNLIMP void debug_setup_tags(const dag::ConstSpan<int> *allowed_tags, const dag::ConstSpan<int> *ignored_tags,
  const dag::ConstSpan<int> *promoted_tags);
#if DAGOR_FORCE_LOGS
KRNLIMP void crypt_debug_setup(const unsigned char *nkey, unsigned max_size = 0);
KRNLIMP int get_nearest_crypted_len(int maxlen, int lev = LOGLEVEL_DEBUG);
#endif
KRNLIMP void debug_enable_thread_ids(bool en_tid);
KRNLIMP void debug_set_thread_name(const char *persistent_thread_name_ptr);
KRNLIMP void debug_override_log_timestamp_format(debug_override_timestamp_cb_t);
KRNLIMP LogHandlerId debug_add_log_handler(log_handler_t handler, void *ctx, const LogHandlerDesc &desc);
KRNLIMP void debug_remove_log_handler(LogHandlerId id);
KRNLIMP void debug_remove_all_log_handlers();
KRNLIMP void debug_stop_calling_log_handlers();
#else
inline const char *get_log_directory() { return ""; }
inline const char *get_log_filename() { return ""; }
inline void close_debug_files() {}
inline void flush_debug_file() {}
inline void start_debug_system(const char *, const char *prefix = 0, bool = true) { (void)prefix; }
inline void start_classic_debug_system(const char *, bool = true, bool = false, int = 0, bool = false) {}
inline void debug_enable_timestamps(bool, bool = false, int = 0) {}
inline void debug_setup_tags(const dag::ConstSpan<int> *, const dag::ConstSpan<int> *, const dag::ConstSpan<int> *) {}
inline void debug_enable_thread_ids(bool) {}
inline void debug_set_thread_name(const char *) {}
inline void debug_override_log_timestamp_format(debug_override_timestamp_cb_t) {}
inline LogHandlerId debug_add_log_handler(log_handler_t, void *, const LogHandlerDesc &) { return LogHandlerId::Invalid; }
inline void debug_remove_log_handler(LogHandlerId) {}
inline void debug_remove_all_log_handlers() {}
inline void debug_stop_calling_log_handlers() {}
#endif

class ScopedLogHandler
{
  LogHandlerId id;

public:
  ScopedLogHandler(log_handler_t handler, void *ctx, const LogHandlerDesc &desc) : id(debug_add_log_handler(handler, ctx, desc)) {}
  ~ScopedLogHandler() { debug_remove_log_handler(id); }
  ScopedLogHandler(const ScopedLogHandler &) = delete;
  ScopedLogHandler &operator=(const ScopedLogHandler &) = delete;
};

#include <supp/dag_undef_KRNLIMP.h>
