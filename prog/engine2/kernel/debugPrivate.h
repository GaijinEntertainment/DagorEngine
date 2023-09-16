#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include "writeStream.h"

namespace debug_internal
{
extern bool flush_debug;
extern bool always_flush_debug;
extern bool level_files;
extern bool append_files;
extern int append_files_stage;
extern int debug_enabled_bits;
extern debug_log_callback_t log_callback;

struct Context
{
#if DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0
  int threadId;
  const char *threadName;
#endif
  const char *file;
  int line;
  bool holdLine, nextSameLine, lastHoldLine;

  void reset()
  {
    file = NULL;
    lastHoldLine = holdLine;
  }
};

extern thread_local Context dbg_ctx;

extern void vlog(int lev, const char *fmt, const void *arg, int anum);

#if _TARGET_PC && (DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS)
extern write_stream_t dbgFile, logerrFile, logwarnFile;
#endif

extern bool timestampEnabled;
extern char dbgCrashDumpPath[DAGOR_MAX_PATH];
extern char dbgFilepath[DAGOR_MAX_PATH];

static constexpr int MAX_TAGS = 32;
extern int allowed_tags[MAX_TAGS], ignored_tags[MAX_TAGS], promoted_tags[MAX_TAGS];
extern int stdTags[LOGLEVEL_REMARK + 1];

void write_stream_flush_locked(write_stream_t stream);
void setupCrashDumpFileName();
extern int maxWriteTimeUs; // max time of single log write
extern int64_t totalWriteTimeUs, totalWriteCalls;

#if (_TARGET_PC || _TARGET_ANDROID) && (DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS)
const char *get_logfilename_for_sending();
const char *get_logging_directory();
#else
inline const char *get_logfilename_for_sending() { return ""; }
inline const char *get_logging_directory() { return ""; }
#endif

bool on_log_handler(int tag, const char *fmt, const void *arg, int anum);
} // namespace debug_internal

extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);
