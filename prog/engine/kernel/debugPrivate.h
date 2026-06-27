// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <util/dag_safeArg.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_atomic_types.h>
#include "writeStream.h"

#define FORCE_THREAD_IDS _TARGET_XBOX || _TARGET_C1 || _TARGET_C2

namespace debug_internal
{
extern dag::AtomicInteger<bool> flush_debug;
extern bool always_flush_debug;
extern bool level_files;
extern bool append_files;
extern int append_files_stage;
extern int debug_enabled_bits;
// NOTE: log_callback should point to a static function.
// Also note that, when changing it via debug_set_log_callback(), the old
// callback MIGHT still be called again if another thread was racing with the one calling
// debug_set_log_callback().
extern volatile debug_log_callback_t log_callback;

struct Context
{
#if DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0 || FORCE_THREAD_IDS
  const char *threadName;
#endif
  const char *file;
#if DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0 || FORCE_THREAD_IDS
  int threadId;
#endif
  int line;
  unsigned int hash;
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

#if (_TARGET_PC || _TARGET_ANDROID || _TARGET_IOS) && (DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS)
const char *get_logfilename_for_sending();
const char *get_logging_directory();
#else
inline const char *get_logfilename_for_sending() { return ""; }
inline const char *get_logging_directory() { return ""; }
#endif

bool on_log_handler(int tag, const char *fmt, const void *arg, int anum);

template <int INLINE_LEN = 8 << 10, int MAX_CAP = 128 << 10>
class PrintBuffer
{
  G_STATIC_ASSERT(INLINE_LEN <= MAX_CAP);

  char *buf = inlineBuf;
  int len = 0, capacity = INLINE_LEN;
  char inlineBuf[INLINE_LEN];

  void reserve(int new_cap)
  {
    if (new_cap <= capacity)
      return;

    new_cap = min(max(new_cap, capacity * 2), MAX_CAP);
    char *newBuf;
    if (buf == inlineBuf)
    {
      if ((newBuf = (char *)::malloc(new_cap)))
        memcpy(newBuf, buf, len + 1);
    }
    else
      newBuf = (char *)::realloc(buf, new_cap);
    G_ASSERT_RETURN(newBuf, );
    buf = newBuf;
    capacity = new_cap;
  }

  char *appendPos() { return &buf[len]; }
  int spaceLeft() const
  {
    int ls = capacity - len - 1; // leave one byte after the formatter buffer for final NUL
    return ls < 0 ? 0 : ls;
  }

  int printfLen(const char *format, const void *arg)
  {
    DAG_VACOPY(args_copy, *(va_list *)arg);
    int ret = vsnprintf(nullptr, 0, format, args_copy);
    DAG_VACOPYEND(args_copy);
    G_FAST_ASSERT(ret >= 0); // Note: C99 snprintf behaviour supported since MSVC2015
    return ret;
  }

public:
  char *data() { return buf; }
  int length() const { return len; }
  bool endsWithNewline() const { return len > 0 && buf[len - 1] == '\n'; }

  PrintBuffer() { inlineBuf[0] = '\0'; }
  PrintBuffer(const PrintBuffer &) = delete;
  PrintBuffer &operator=(const PrintBuffer &) = delete;
  ~PrintBuffer() { (buf != inlineBuf) ? ::free(buf) : (void)0; }

  void reset()
  {
    if (buf != inlineBuf)
      ::free(buf);
    buf = inlineBuf;
    capacity = INLINE_LEN;
    len = 0;
    buf[0] = '\0';
  }

  template <class... Args>
  void sprintf(const char *format, Args... args)
  {
    int ls = spaceLeft();
    int ret = snprintf(appendPos(), ls, format, args...);
    len += (ret >= 0 && ret < ls) ? ret : strlen(appendPos());
  }

  void overrideTimestamp(debug_override_timestamp_cb_t callback, int64_t t_msec)
  {
    callback(appendPos(), spaceLeft(), t_msec);
    len += strlen(appendPos());
  }

  // Only this final append grows the buffer; sprintf() prefixes must fit inline.
  void appendFmt(bool term, const char *format, const void *arg, int anum)
  {
    int ls = spaceLeft();
    int ret = DagorSafeArg::mixed_print_fmt(appendPos(), ls, format, arg, anum);
    if (ls == 0 || (unsigned)ret >= (unsigned)ls - 1)
    {
      // buf is not big enough, so allocate temporary buffer on heap
      ret = (anum < 0) ? printfLen(format, arg) : DagorSafeArg::count_len(format, (const DagorSafeArg *)arg, anum);
      reserve(len + ret + 16);
      ret = DagorSafeArg::mixed_print_fmt(appendPos(), (ls = spaceLeft()), format, arg, anum);
      ret = (ret >= 0 && ret < ls) ? ret : strlen(appendPos());
    }
    len += ret;

    if (term)
    {
      if (len + 1 < capacity)
        ++len;
      buf[len - 1] = '\n';
    }
    buf[len] = '\0';
  }
};

} // namespace debug_internal

extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);
#if DAGOR_FORCE_LOGS
extern const unsigned char *get_dagor_log_crypt_key();
#endif
