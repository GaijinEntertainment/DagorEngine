#include <stdarg.h>
#include <stdio.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <supp/_platform.h>
#include "debugPrivate.h"
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <generic/dag_span.h>
#include <math/dag_mathBase.h>        // for min
#include <osApiWrappers/dag_dbgStr.h> // for min
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_logSys.h>

#if _TARGET_IOS | _TARGET_TVOS
#include <asl.h>

#if _TARGET_TVOS
namespace debug_internal
{
const char *get_logfilename_for_sending_tvos();
void debug_log_tvos(const char *);
} // namespace debug_internal
#endif
#endif

bool debug_internal::append_files = false;
int debug_internal::append_files_stage = 0;

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
static int logimpl_ctors_inited = 0;
debug_log_callback_t debug_internal::log_callback = NULL;

thread_local debug_internal::Context debug_internal::dbg_ctx;
bool debug_internal::timestampEnabled = true;

#define DEF_AR \
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
int debug_internal::allowed_tags[debug_internal::MAX_TAGS] = {DEF_AR};
int debug_internal::ignored_tags[debug_internal::MAX_TAGS] = {DEF_AR};
int debug_internal::promoted_tags[debug_internal::MAX_TAGS] = {DEF_AR};
#undef DEF_AR

int debug_internal::stdTags[LOGLEVEL_REMARK + 1] = {
  MAKE4C('[', 'F', ']', ' '),
  MAKE4C('[', 'E', ']', ' '),
  MAKE4C('[', 'W', ']', ' '),
  MAKE4C('[', 'D', ']', ' '),
  MAKE4C('[', 'R', ']', ' '),
};

#if !_TARGET_PC

static struct CritSecGlobal : public CritSecStorage
{
  CritSecGlobal() { create_critical_section(this); }
} writeCS; // never destroyed b/c it can be used really late (after all static dtors) (TODO: do noop locking in this case)

#if DAGOR_FORCE_LOGS && _TARGET_ANDROID
#define CRYPT_LOG_AVAILABLE 1
#else
#define CRYPT_LOG_AVAILABLE 0
#endif

#if CRYPT_LOG_AVAILABLE
#include <EASTL/array.h>

struct CryptLog
{
  eastl::array<uint8_t, 128> key;
  uint32_t cur = 0;

  CryptLog() { eastl::fill(key.begin(), key.end(), 0); }

  explicit operator bool() const { return key[0] != '\0'; }
};

static CryptLog cryptLog;

void crypt_debug_setup(const unsigned char *nkey, unsigned)
{
  if (nkey && *nkey)
    memcpy(cryptLog.key.data(), nkey, cryptLog.key.size());
}

static void crypt_out_str(unsigned char *buf, size_t sz)
{
  if (!cryptLog)
    return;

  uint32_t cur = cryptLog.cur;
  for (int i = 0; i < sz; i++)
  {
    buf[i] ^= cryptLog.key[cur];
    cur = (cur + 1) & 0x7F;
  }
  cryptLog.cur = cur;
}

int uncrypt_out_str(unsigned char *buf, size_t sz, int key_offset)
{
  if (!cryptLog)
    return 0;

  uint32_t cur = key_offset;
  for (int i = 0; i < sz; i++)
  {
    buf[i] ^= cryptLog.key[cur];
    cur = (cur + 1) & 0x7F;
  }
  return cur;
}
#else
int uncrypt_out_str(unsigned char *, size_t, int) { return 0; }
#endif

#if _TARGET_IOS | _TARGET_TVOS | _TARGET_ANDROID
extern char ios_global_log_fname[];
static FILE *ios_global_fp = NULL;
#elif _TARGET_XBOX
static file_ptr_t xbox_debug_file = NULL;
#endif

#define PFUN(fmt, ...)                                                                  \
  do                                                                                    \
  {                                                                                     \
    _snprintf(buf + buf_used_len, sizeof(vlog_buf) - buf_used_len, fmt, ##__VA_ARGS__); \
    buf_used_len += i_strlen(buf + buf_used_len);                                       \
  } while (0)

#define LOG_TAIL_BUF (_TARGET_XBOX || _TARGET_C1 || _TARGET_C2 || _TARGET_ANDROID || _TARGET_IOS)

#if DAGOR_DBGLEVEL > 0 || FORCE_THREAD_IDS
static int next_thread_id = 1;
#endif

static bool dbg_tid_enabled = false;

void debug_enable_thread_ids(bool en_tid) { dbg_tid_enabled = en_tid; }
void debug_set_thread_name(const char *persistent_thread_name_ptr)
{
#if !DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0 || FORCE_THREAD_IDS
  (&debug_internal::dbg_ctx)->threadName = persistent_thread_name_ptr;
#else
  (void)(persistent_thread_name_ptr);
#endif
}

static thread_local char vlog_buf[8 << 10];

#if LOG_TAIL_BUF
#include <util/dag_ringBuf.h>

struct LogTailBuf
{
  static constexpr int max_tail_size = 50 * 1024;
  WinCritSec mutex;
  DaFixedRingBuffer<50 * 1024> *ring_buf;

  LogTailBuf() : ring_buf(new DaFixedRingBuffer<50 * 1024>) {}
  ~LogTailBuf() { delete ring_buf; }
};

// It is better to use InitOnDemand, but it can't be used in kernel library due to NO_MEMBASE_INCLUDE define
static LogTailBuf log_tail_buf;
void put_to_tail_buf(const char *data, int len)
{
  WinAutoLock lock(log_tail_buf.mutex);
  log_tail_buf.ring_buf->write(data, len, true);
}

int get_from_tail_buf(char *out_buf, int len)
{
  WinAutoLock lock(log_tail_buf.mutex);
  return log_tail_buf.ring_buf->tail(out_buf, len);
}

#define PUT_TO_TAIL_BUF(data, len)      put_to_tail_buf(data, len)
#define GET_FROM_TAIL_BUF(out_buf, len) get_from_tail_buf(out_buf, len)

#else // LOG_TAIL_BUF

#define PUT_TO_TAIL_BUF(data, len)
#define GET_FROM_TAIL_BUF(out_buf, len) (0)

#endif // LOG_TAIL_BUF

#if _TARGET_XBOX
static const char *xbox_get_log_fn()
{
#if DAGOR_DBGLEVEL != 0
  return "D:\\gaijin_debug.txt";
#else
  return "T:\\gaijin_debug.txt";
#endif
}
#endif

static void log_write(const char *data, size_t len)
{
#if _TARGET_C1 || _TARGET_C2 || _TARGET_C3

#elif _TARGET_XBOX
  if (!xbox_debug_file)
    xbox_debug_file = df_open(xbox_get_log_fn(), DF_WRITE | DF_CREATE);
  if (xbox_debug_file)
    df_write(xbox_debug_file, data, len);
  out_debug_str(data); // There is no XR-008 anymore.

#elif _TARGET_IOS | _TARGET_TVOS
#if _TARGET_SIMD_NEON
  if (is_debug_console_ios_file_output() && get_debug_console_handle() != invalid_console_handle)
  {
    if (!ios_global_fp && ios_global_log_fname[0])
    {
      ios_global_fp = fopen(ios_global_log_fname, "wt");
      out_debug_str_fmt("Creating log file: %s", ios_global_log_fname);
      if (!ios_global_fp)
      {
        out_debug_str("failed to create log file");
        ios_global_log_fname[0] = 0;
        set_debug_console_handle(invalid_console_handle);
      }
    }
    if (ios_global_fp)
      fprintf(ios_global_fp, "%s", data);
  }
  else
#endif
  {
    static aslclient client = asl_open(NULL, "com.apple.console", 0);
    asl_log(client, NULL, ASL_LEVEL_NOTICE, "%s", data);
#if _TARGET_TVOS
    debug_internal::debug_log_tvos(data);
#endif
  }
#elif CRYPT_LOG_AVAILABLE
  WinAutoLock lock(writeCS);
  if (!cryptLog)
  {
    out_debug_str(data);
    PUT_TO_TAIL_BUF(data, len);
  }
  else if (get_debug_console_handle() != invalid_console_handle)
  {
    fwrite(data, 1, len, (FILE *)get_debug_console_handle());
    fflush((FILE *)get_debug_console_handle());
  }
#else
  out_debug_str(data);
#endif

#if !CRYPT_LOG_AVAILABLE
  PUT_TO_TAIL_BUF(data, len);
#endif
}

void debug_internal::vlog(int tag, const char *format, const void *arg, int anum)
{
  if (!logimpl_ctors_inited)
    return;
  char *buf = vlog_buf;
  int buf_used_len = i_strlen(buf);

#if !DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0 || FORCE_THREAD_IDS
  if (!dbg_tid_enabled || (&dbg_ctx)->threadId)
    ; // do nothing
  else if (is_main_thread())
    (&dbg_ctx)->threadId = 1;
  else
  {
    (&dbg_ctx)->threadId = interlocked_increment(next_thread_id);
    PFUN("---$%02X %s ---\n", (&dbg_ctx)->threadId - 1, (&dbg_ctx)->threadName);
  }
#endif

  if (!debug_internal::on_log_handler(tag, format, arg, anum))
    return;

  int t = debug_internal::timestampEnabled ? get_time_msec() : -1;
#if !DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0 || FORCE_THREAD_IDS
  int thread_id = dbg_tid_enabled ? (&dbg_ctx)->threadId - 1 : -1;
#else
  int thread_id = -1;
#endif
  bool addSlashN = !(&dbg_ctx)->holdLine;
  bool new_debug_line = !(&dbg_ctx)->nextSameLine;
  if (thread_id > 0)
  {
    if ((&dbg_ctx)->file)
    {
      if (debug_internal::timestampEnabled)
        PFUN("%d.%02d $%02X . %s,%d: ", t / 1000, (t % 1000) / 10, thread_id, (&dbg_ctx)->file, (&dbg_ctx)->line);
      else
        PFUN("$%02X %s,%d: ", thread_id, (&dbg_ctx)->file, (&dbg_ctx)->line);
      (&dbg_ctx)->reset();
    }
    else if (new_debug_line && debug_internal::timestampEnabled)
      PFUN("%d.%02d $%02X ", t / 1000, (t % 1000) / 10, thread_id);
  }
  else if ((&dbg_ctx)->file)
  {
    if (debug_internal::timestampEnabled)
      PFUN("%d.%02d . %s,%d: ", t / 1000, (t % 1000) / 10, (&dbg_ctx)->file, (&dbg_ctx)->line);
    else
      PFUN("%s,%d: ", (&dbg_ctx)->file, (&dbg_ctx)->line);
    (&dbg_ctx)->reset();
  }
  else if (new_debug_line && debug_internal::timestampEnabled)
    PFUN("%d.%02d ", t / 1000, (t % 1000) / 10);

  const int rt = (uint32_t)tag <= LOGLEVEL_REMARK ? debug_internal::stdTags[tag] : tag;
  if (rt != debug_internal::stdTags[LOGLEVEL_DEBUG]) // I'm not sure whether we need or not print debug tag
    PFUN("%c%c%c%c ", _DUMP4C(rt));

  DagorSafeArg::mixed_print_fmt(buf + buf_used_len, sizeof(vlog_buf) - buf_used_len, format, arg, anum);
  buf[sizeof(vlog_buf) - 2] = 0;

  size_t bufLen = buf_used_len + strlen(buf + buf_used_len);
  if (addSlashN)
    strcpy(buf + bufLen, "\n");
  bool term = addSlashN || (bufLen > 0 && buf[bufLen - 1] == '\n');

  if (term)
  {
#if _TARGET_XBOX
    for (char *pos = buf; *pos; pos++) // Watson is sensitive to non-printable characters and can terminate the log with 0x83750007
                                       // WEB_E_INVALID_JSON_STRING.
      if ((uint8_t)*pos < ' ' && *pos != '\n' && *pos != '\r' && *pos != '\t')
        *pos = '*';
#endif

#if CRYPT_LOG_AVAILABLE
    PUT_TO_TAIL_BUF(buf, bufLen + (int)addSlashN);

    {
      WinAutoLock lock(writeCS);
      crypt_out_str((unsigned char *)buf, bufLen + (int)addSlashN);
    }
#endif

    log_write(buf, bufLen + (int)addSlashN);
    buf[0] = 0;
  }

  (&dbg_ctx)->nextSameLine = !term;
  if (!term)
    (&dbg_ctx)->holdLine = false;
}

#undef PFUN

void debug_override_log_timestamp_format(debug_override_timestamp_cb_t) {}

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
int tail_debug_file(char *out_buf, int buf_size) { return GET_FROM_TAIL_BUF(out_buf, buf_size); }
#endif

#endif // !_TARGET_PC

void cvlogmessage(int lev, const char *s, va_list arg)
{
  DAG_VACOPY(copy, arg);
  debug_internal::vlog(lev, s, &copy, -1);
  DAG_VACOPYEND(copy);
}

void logmessage_fmt(int lev, const char *fmt, const DagorSafeArg *arg, int anum) { debug_internal::vlog(lev, fmt, arg, anum); }

void __log_set_ctx(const char *fn, int ln, int, bool new_ln)
{
  if (!logimpl_ctors_inited)
    return;

  while (*fn == '/' || *fn == '\\' || *fn == '.')
    fn++;

  (&debug_internal::dbg_ctx)->file = fn;
  (&debug_internal::dbg_ctx)->line = ln;
  (&debug_internal::dbg_ctx)->holdLine = !new_ln;
}
void __log_set_ctx_ln(bool new_ln) { (&debug_internal::dbg_ctx)->holdLine = !new_ln; }

void __debug_cp(const char *fn, int ln)
{
  while (*fn == '/' || *fn == '\\' || *fn == '.')
    fn++;
  debug("cp: %s,%d", fn, ln);
}

extern "C" void cdebug(const char *f, ...)
{
  va_list ap;
  va_start(ap, f);
  cvlogmessage(LOGLEVEL_DEBUG, f, ap);
  va_end(ap);
}

extern "C" void cdebug_(const char *f, ...)
{
  va_list ap;
  va_start(ap, f);
  (&debug_internal::dbg_ctx)->holdLine = true;
  cvlogmessage(LOGLEVEL_DEBUG, f, ap);
  va_end(ap);
}

static inline bool feq(int t, int *ta, int n)
{
  for (int i = 0; i < n; ++i)
    if (ta[i] == t)
      return true;
  return false;
}

static inline bool fneq(int t, int *ta, int n)
{
  for (int i = 0; i < n; ++i)
    if (ta[i] != t)
      return true;
  return false;
}

bool debug_internal::on_log_handler(int tag, const char *fmt, const void *arg, int anum)
{
  if (tag == -1) // special case - invalid tag, should never happens
    return false;

  bool r = true;
  if (fneq(-1, allowed_tags, countof(allowed_tags))) // if some allowed tags exist
  {
    r = feq(tag, allowed_tags, countof(allowed_tags));
    goto ret_l;
  }

  if (feq(tag, ignored_tags, countof(ignored_tags)))
    r = false;

ret_l:

  if (r && dgs_on_promoted_log_tag && feq(tag, promoted_tags, countof(promoted_tags)))
    (*dgs_on_promoted_log_tag)(tag, fmt, arg, anum);

  if (debug_internal::log_callback)
    r = debug_internal::log_callback(tag, fmt, arg, anum, (&dbg_ctx)->file, (&dbg_ctx)->line) > 0;
  return r;
}

void debug_setup_tags(const dag::ConstSpan<int> *allowed_tags_, const dag::ConstSpan<int> *ignored_tags_,
  const dag::ConstSpan<int> *promoted_tags_)
{
#define _CP(x)                                                                                           \
  if (x##_ && x##_->size())                                                                              \
    memcpy(debug_internal::x, x##_->data(), min((unsigned)sizeof(debug_internal::x), data_size(*x##_))); \
  else                                                                                                   \
    memset(debug_internal::x, 255, sizeof(debug_internal::x));

  _CP(allowed_tags);
  _CP(ignored_tags);
  _CP(promoted_tags);

#undef _CP
}

static int start_t_sec = 0, start_cpu_t_msec = 0, hhmmss_add = 0, hhmmss_div = 1;
static int hhmmss_timestamp_cb(char *dest, int dest_sz, int64_t t_msec)
{
  t_msec -= start_cpu_t_msec;
  int t = start_t_sec + int(t_msec / 1000), sz = 0;
  if (hhmmss_add)
    sz = _snprintf(dest, dest_sz, "%2d:%02d:%02d.%0*d", (t / 3600) % 24, (t % 3600) / 60, t % 60, hhmmss_add - 1,
      int(t_msec % 1000) / hhmmss_div);
  else
    sz = _snprintf(dest, dest_sz, "%2d:%02d:%02d", (t / 3600) % 24, (t % 3600) / 60, t % 60);
  return (unsigned)sz < (unsigned)dest_sz ? sz : 0;
}

void debug_enable_timestamps(bool e, bool hhmmss_fmt, int hhmmss_subsec)
{
  debug_internal::timestampEnabled = e;
  if (hhmmss_fmt)
  {
    DagorDateTime t;
    memset(&t, 0, sizeof(t));
    get_local_time(&t);

    start_cpu_t_msec = get_time_msec() - t.microsecond / 1000;
    start_t_sec = t.hour * 3600 + t.minute * 60 + t.second;

    if (hhmmss_subsec)
    {
      hhmmss_subsec = clamp(hhmmss_subsec, 1, 3);
      hhmmss_add = hhmmss_subsec + 1;
      hhmmss_div = 1000;
      while (hhmmss_subsec)
        hhmmss_div /= 10, hhmmss_subsec--;
    }
    else
      hhmmss_add = 0, hhmmss_div = 1;
    debug_override_log_timestamp_format(hhmmss_timestamp_cb);
  }
}

#if _TARGET_PC
#define DIF(x) debug_internal::x##File
void flush_debug_file()
{
  debug_internal::write_stream_t files[] = {DIF(dbg), DIF(logerr), DIF(logwarn)};
  for (int i = 0; i < countof(files); ++i)
    if (files[i])
      debug_internal::write_stream_flush_locked(files[i]);
}

void close_debug_files()
{
  if (debug_internal::totalWriteCalls > 0)
  {
    debug("max log write time %d us, average %d us in %d calls", debug_internal::maxWriteTimeUs,
      debug_internal::totalWriteTimeUs / debug_internal::totalWriteCalls, debug_internal::totalWriteCalls);
    debug_internal::maxWriteTimeUs = 0;
    debug_internal::totalWriteTimeUs = 0;
    debug_internal::totalWriteCalls = 0;
  }

  debug_internal::write_stream_t *files[] = {&DIF(dbg), &DIF(logerr), &DIF(logwarn)};
  for (int i = 0; i < countof(files); ++i)
    if (*files[i])
    {
      debug_internal::write_stream_close(*files[i]);
      *files[i] = NULL;
    }

#ifdef DAGOR_CAPTURE_STDERR
  // We need to catch whatever atexit destructors have to say (e.g. ASan's Leak Detector)
  FILE *canLeakThisFp = freopen(debug_internal::dbgFilepath, "at", stderr);
#endif

  debug_internal::append_files = true;
  debug_internal::append_files_stage = 2;
  debug_internal::write_stream_set_fmode("ab");
}

#undef DIF

void debug_flush(bool df)
{
  flush_debug_file();
  debug_internal::flush_debug = df || debug_internal::always_flush_debug;
}

void force_debug_flush(bool df)
{
  debug_internal::always_flush_debug = df;
  debug_flush(df);
}

void debug_allow_level_files(bool en) { debug_internal::level_files = en; }

#elif _TARGET_IOS | _TARGET_TVOS
void flush_debug_file()
{
  if (ios_global_fp)
  {
    out_debug_str_fmt("flushing %s", ios_global_log_fname);
    fflush(ios_global_fp);
  }
}

void close_debug_files()
{
  if (ios_global_fp)
  {
    out_debug_str_fmt("closing %s", ios_global_log_fname);
    fclose(ios_global_fp);
  }
  ios_global_fp = NULL;
}

void debug_flush(bool) {}
void debug_allow_level_files(bool) {}
void force_debug_flush(bool) {}

#elif _TARGET_XBOX

void flush_debug_file()
{
  if (xbox_debug_file)
    df_flush(xbox_debug_file);
}

void debug_flush(bool)
{
  if (xbox_debug_file)
    df_flush(xbox_debug_file);
}

void force_debug_flush(bool) {}

void close_debug_files()
{
  if (xbox_debug_file)
    df_close(xbox_debug_file);
  xbox_debug_file = NULL;
}

void debug_allow_level_files(bool) {}

#else

void flush_debug_file() {}
void debug_flush(bool) {}
void force_debug_flush(bool) {}
void close_debug_files() {}
void debug_allow_level_files(bool) {}

#endif // _TARGET_PC

static struct LogimplCppStaticCtorLast
{
  LogimplCppStaticCtorLast() { logimpl_ctors_inited = 1; }
} staticCtorLast;

const char *get_log_directory() { return debug_internal::get_logging_directory(); }

const char *get_log_filename()
{
#if _TARGET_TVOS
  return debug_internal::get_logfilename_for_sending_tvos();
#else
  return debug_internal::get_logfilename_for_sending();
#endif
}

debug_log_callback_t debug_set_log_callback(debug_log_callback_t cb)
{
  debug_log_callback_t prev = debug_internal::log_callback;
  debug_internal::log_callback = cb;
  return prev;
}
#endif // DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#if _TARGET_PC
#define EXPORT_PULL dll_pull_kernel_log
#include <supp/exportPull.h>
#endif
