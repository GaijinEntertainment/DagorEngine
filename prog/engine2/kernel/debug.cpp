#if !_TARGET_PC
!compile error;
#endif

#include <stdarg.h>
#include <stdio.h>
#include <float.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <supp/_platform.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/basePath.h>
#include <atomic>
#if _TARGET_PC_WIN
#include <direct.h>
#elif defined(__GNUC__)
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#endif
#include <time.h>
#include "debugPrivate.h"

#define DEBUG_DO_PERIODIC_FLUSHES (DAGOR_DBGLEVEL > 0 && !_TARGET_STATIC_LIB)
#define THREAD_IDS_BIT            0x40000000
#define ASYNC_WRITER_BUF_SIZE     (128 << 10) // Note: glibc default FILE buffer is 8K, MSVC - 4K
#define MEASURE_WRITE_TIME        0

#if DEBUG_DO_PERIODIC_FLUSHES
static constexpr int DEBUG_FLUSH_PERIOD = 10000;
static int next_flush_time = 0;
#endif

int debug_internal::maxWriteTimeUs = 0;
int64_t debug_internal::totalWriteTimeUs = 0, debug_internal::totalWriteCalls = 0;
char debug_internal::dbgCrashDumpPath[DAGOR_MAX_PATH];
char debug_internal::dbgFilepath[DAGOR_MAX_PATH];

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
static int debug_ctors_inited = 0;

debug_internal::write_stream_t debug_internal::dbgFile = NULL, debug_internal::logerrFile = NULL, debug_internal::logwarnFile = NULL;
static char logerrFilepath[DAGOR_MAX_PATH], logwarnFilepath[DAGOR_MAX_PATH], lastDbgPath[DAGOR_MAX_PATH];
static char fatalerrFilepath[DAGOR_MAX_PATH];
static char logDirPath[DAGOR_MAX_PATH];
static std::atomic<int> logFileSizes[LOGLEVEL_REMARK + 1];
static unsigned logsMaxSize = 0;

// debug_internal::Context debug_internal::debug_context = { NULL, -1 };
bool debug_internal::always_flush_debug = false;
bool debug_internal::flush_debug = false;
int debug_internal::debug_enabled_bits = 0;
bool debug_internal::level_files = true;

static debug_override_timestamp_cb_t override_timestamp_cb = NULL;

using namespace debug_internal;

static int next_thread_id = 1;
void debug_enable_thread_ids(bool en_tid)
{
  if (en_tid)
    debug_enabled_bits |= THREAD_IDS_BIT;
  else
    debug_enabled_bits &= ~THREAD_IDS_BIT;
}
void debug_set_thread_name(const char *persistent_thread_name_ptr)
{
  (&debug_internal::dbg_ctx)->threadName = persistent_thread_name_ptr;
}

static struct CritSecGlobal : public CritSecStorage
{
  CritSecGlobal() { create_critical_section(this); }
} writeCS; // never destroyed b/c it can be used really late (after all static dtors) (TODO: do noop locking in this case)
void debug_internal::write_stream_flush_locked(write_stream_t stream)
{
  WinAutoLock lock(writeCS);
  write_stream_flush(stream);
}

#if DAGOR_FORCE_LOGS
static unsigned char cryptKeyStorage[128], *cryptKey = NULL;
static volatile unsigned cryptLevCur[LOGLEVEL_REMARK + 1] = {0};

void crypt_debug_setup(const unsigned char *nkey, unsigned max_size)
{
  memcpy(cryptKeyStorage, nkey, sizeof(cryptKeyStorage));
  logsMaxSize = max_size;
  for (auto &sz : logFileSizes)
    std::atomic_init(&sz, 0);

  cryptKey = NULL;
  for (unsigned char *k = cryptKeyStorage; k < (cryptKeyStorage + sizeof(cryptKeyStorage)) && !cryptKey; ++k) // empty key?
    if (*k != '\0')
      cryptKey = cryptKeyStorage;
}

static void crypt_out_str(unsigned char *buf, size_t sz, int ik)
{
  if (!cryptKey)
    return;

  unsigned c = cryptLevCur[ik];
  for (int i = 0; i < sz; i++)
  {
    buf[i] = buf[i] ^ cryptKey[c];
    c = (c + 1) & 0x7F;
  }
  cryptLevCur[ik] = c;
}

static void uncrypt_tail_str(unsigned char *buf, size_t sz)
{
  if (!cryptKey)
    return;
  unsigned c = 0;
  for (int i = 0; i < sz; i++)
  {
    buf[i] = buf[i] ^ cryptKey[c];
    c = (c + 1) & 0x7F;
  }
}

int get_nearest_crypted_len(int max_len, int lev)
{
  G_ASSERT(max_len > 0);
  if (max_len <= 0)
    return 0;

  unsigned lastUsedKeySize = cryptLevCur[lev];
  size_t keySize = sizeof(cryptKeyStorage);
  size_t nKeys = max_len / keySize;
  if (max_len % keySize < lastUsedKeySize)
    nKeys--;

  return int(keySize * nKeys + lastUsedKeySize);
}
#endif

int tail_debug_file(char *dst, int max_bytes)
{
  const char *p = get_log_filename();
  if (!p || !*p)
    return 0;

  WinAutoLock lock(writeCS);
  flush_debug_file();

  file_ptr_t f = df_open(p, DF_READ | DF_REALFILE_ONLY);
  if (!f)
    return 0;

  max_bytes = min(df_length(f), max_bytes);
#if _TARGET_PC && DAGOR_FORCE_LOGS
  int size = get_nearest_crypted_len(max_bytes);
#else
  int size = max_bytes;
#endif

  int read = df_seek_end(f, -1 * size);
  if (!read)
    read = df_read(f, dst, size);

  df_close(f);

#if _TARGET_PC && DAGOR_FORCE_LOGS
  if (read > 0)
    uncrypt_tail_str((unsigned char *)dst, read);
#endif

  return read > 0 ? read : 0;
}

static bool DAGOR_NOINLINE prepare_file_impl(const char *path, write_stream_t &fp, int lev_bit);
static inline bool prepare_file(const char *path, write_stream_t &fp, int lev_bit)
{
  return interlocked_acquire_load_ptr(fp) || prepare_file_impl(path, fp, lev_bit);
}
static bool prepare_file_impl(const char *path, write_stream_t &fpout, int lev_bit)
{
  if (!(interlocked_acquire_load(debug_enabled_bits) & lev_bit) || !*path)
    return false;

  interlocked_and(debug_enabled_bits, ~lev_bit);
  dd_mkpath(path);
  if (lastDbgPath[0])
  {
    dd_mkpath(lastDbgPath);
    FILE *fp2 = fopen(lastDbgPath, "wt");
    if (fp2)
    {
      if (fwrite(dbgFilepath, 1, strlen(dbgFilepath) - 5, fp2))
      { // workaround against gcc's warn_unused_result warning
      }
      fclose(fp2);
    }
    lastDbgPath[0] = '\0';
  }

  write_stream_t fp;
  if (*path != '*')
  {
#if DAGOR_CAPTURE_STDERR
    static bool stderr_captured = false;
    if (!stderr_captured)
    {
      fp = write_stream_open(path, ASYNC_WRITER_BUF_SIZE, stderr, true);
      stderr_captured = true;
    }
    else
#endif
      fp = write_stream_open(path, ASYNC_WRITER_BUF_SIZE);
  }
  else
    fp = write_stream_open(NULL, ASYNC_WRITER_BUF_SIZE, stdout);
  if (!fp)
    return false;
  if (DAGOR_UNLIKELY(interlocked_compare_exchange_ptr(fpout, fp, (write_stream_t)NULL)))
  { // transaction failed, some other thread managed to write it first
    write_stream_close(fp);
    return true;
  }
  if (debug_internal::append_files && debug_internal::append_files_stage > 0)
  {
    WinAutoLock lock(writeCS);
    write_stream_write("\n-------\n\n", 10, fp);
  }
#if _TARGET_STATIC_LIB
  char sbuf[256];
  int sz = i_strlen(dagor_get_build_stamp_str(sbuf, sizeof(sbuf), "\n\n"));

#if DAGOR_DBGLEVEL > 0
  out_debug_str(sbuf);
#endif

  if (!debug_internal::append_files || debug_internal::append_files_stage < 2)
  {
    WinAutoLock lock(writeCS);
#if DAGOR_FORCE_LOGS
    if (cryptKey)
    {
      int ik = 0;
      if (lev_bit)
        while (!(lev_bit & 1))
        {
          ik++;
          lev_bit >>= 1;
        }
      crypt_out_str((unsigned char *)sbuf, sz, ik);
    }
#endif
    write_stream_write(sbuf, sz, fp);
  }
#endif
  interlocked_or(debug_enabled_bits, lev_bit);
  return true;
}

#define MAX_CRYPTO_LINE (4 << 10)

static void out_file(write_stream_t fp, int lc, int t, bool term, int ik, const char *format, const void *arg, int anum)
{
  if (logsMaxSize && ik != LOGLEVEL_FATAL && logFileSizes[ik] >= logsMaxSize)
    return;

  char sbuf[MAX_CRYPTO_LINE];

  int sz = 0;
  const int enabled_bits = interlocked_acquire_load(debug_enabled_bits);
  if (!(enabled_bits & THREAD_IDS_BIT) || (&dbg_ctx)->threadId)
    ; // do nothing
  else if (is_main_thread())
    (&dbg_ctx)->threadId = 1;
  else
  {
    (&dbg_ctx)->threadId = interlocked_increment(next_thread_id);
    sz = snprintf(sbuf, sizeof(sbuf), "---$%02X %s ---\n", (&dbg_ctx)->threadId - 1, (&dbg_ctx)->threadName);
  }

  int thread_id = (enabled_bits & THREAD_IDS_BIT) ? (&dbg_ctx)->threadId - 1 : -1;
  if (override_timestamp_cb)
  {
    sz += override_timestamp_cb(sbuf, sizeof(sbuf) - sz - 1, t);
    if (thread_id > 0)
      sz += _snprintf(sbuf + sz, sizeof(sbuf) - sz - 1, " %c%c%c%c $%02X ", _DUMP4C(lc), thread_id);
    else
      sz += _snprintf(sbuf + sz, sizeof(sbuf) - sz - 1, " %c%c%c%c ", _DUMP4C(lc));
  }
  else if (t >= 0 && thread_id > 0)
    sz += _snprintf(sbuf + sz, sizeof(sbuf) - sz - 1, "%3d.%02d %c%c%c%c $%02X ", t / 1000, (t % 1000) / 10, _DUMP4C(lc), thread_id);
  else if (t >= 0)
    sz += _snprintf(sbuf + sz, sizeof(sbuf) - sz - 1, "%3d.%02d %c%c%c%c ", t / 1000, (t % 1000) / 10, _DUMP4C(lc));
  else if (lc != debug_internal::stdTags[LOGLEVEL_DEBUG])
    sz += _snprintf(sbuf + sz, sizeof(sbuf) - sz - 1, "%c%c%c%c ", _DUMP4C(lc));

  debug_internal::Context *ctx = &debug_internal::dbg_ctx;
  if (ctx->file)
    sz += _snprintf(sbuf + sz, sizeof(sbuf) - sz - 1, ". %s,%d: ", ctx->file, ctx->line);

  int left = sizeof(sbuf) - sz - (term ? 2 : 1);
  int ret = DagorSafeArg::mixed_print_fmt(sbuf + sz, left, format, arg, anum);
  char *final_sbuf = sbuf;
  if ((unsigned)ret < (unsigned)left - 1)
    sz += ret;
  else
  {
    // sbuf is not big enough, so allocate temporary buffer on heap
    ret = (anum < 0) ? 128 << 10 : DagorSafeArg::count_len(format, (const DagorSafeArg *)arg, anum);
    if (ret > (1 << 20))
      ret = (1 << 20);
    final_sbuf = (char *)malloc(sz + ret + 16);
    memcpy(final_sbuf, sbuf, sz);
    left = ret + 8;
    sz += DagorSafeArg::mixed_print_fmt(final_sbuf + sz, left, format, arg, anum);
  }
  if (term)
    final_sbuf[sz++] = '\n';
  final_sbuf[sz] = '\0';
  G_ASSERT(strlen(final_sbuf) == sz);
  {
    WinAutoLock lock(writeCS); // both operations of crypto & file write should be not only atomic, but strictly ordered as well
#if DAGOR_DBGLEVEL > 0
    out_debug_str(final_sbuf);
#endif
#if DAGOR_FORCE_LOGS
    crypt_out_str((unsigned char *)final_sbuf, sz, ik);
#endif
#if MEASURE_WRITE_TIME
    int64_t ref = ref_time_ticks();
#endif
    write_stream_write(final_sbuf, sz, fp);
#if MEASURE_WRITE_TIME
    totalWriteCalls++;
    if (int spent = get_time_usec(ref))
    {
      maxWriteTimeUs = max(maxWriteTimeUs, spent);
      totalWriteTimeUs += spent;
    }
#endif
  }
  logFileSizes[ik].fetch_add(sz, std::memory_order_relaxed);
  if (final_sbuf != sbuf)
    free(final_sbuf);
}

#if DAGOR_DBGLEVEL > 0
static char vlog_buf_main[64 << 10];
#endif

void debug_internal::vlog(int lev, const char *_fmt, const void *arg, int anum)
{
  if (!debug_ctors_inited || !debug_internal::on_log_handler(lev, _fmt, arg, anum))
    return;

  const int lc = ((uint32_t)lev <= LOGLEVEL_REMARK) ? debug_internal::stdTags[lev] : lev;
  const char *fmt = NULL;
  bool term = !(&dbg_ctx)->holdLine;

  int t = (!(&dbg_ctx)->lastHoldLine && timestampEnabled) ? get_time_msec() : -1;
  if (prepare_file(dbgFilepath, dbgFile, 1 << LOGLEVEL_DEBUG))
  {
    if (!fmt)
      fmt = _fmt;

    out_file(dbgFile, lc, t, term, LOGLEVEL_DEBUG, fmt, arg, anum);

    if (flush_debug)
      write_stream_flush_locked(dbgFile);
  }
#if DAGOR_DBGLEVEL > 0
  else
  {
    if (!fmt)
      fmt = _fmt;
    char tmpBuf[16 << 10];
    char *buf = is_main_thread() ? vlog_buf_main : tmpBuf;
    int bufSize = is_main_thread() ? sizeof(vlog_buf_main) : sizeof(tmpBuf);

    if ((uint32_t)lev > LOGLEVEL_REMARK || lev == LOGLEVEL_DEBUG)
      DagorSafeArg::mixed_print_fmt(buf, bufSize - 3, fmt, arg, anum);
    else
    {
      buf[0] = '[';
      buf[1] = ("FEWDR")[lev];
      buf[2] = ']';
      buf[3] = ' ';
      DagorSafeArg::mixed_print_fmt(buf + 4, bufSize - 7, fmt, arg, anum);
    }

    if (term)
      strcat(buf, "\n");
    out_debug_str(buf);
  }
#endif

  if (debug_internal::level_files)
  {
    if (lev == LOGLEVEL_ERR && prepare_file(logerrFilepath, logerrFile, 1 << LOGLEVEL_ERR))
    {
      if (!fmt)
        fmt = _fmt;
      out_file(logerrFile, lc, t, term, LOGLEVEL_ERR, fmt, arg, anum);
      if (flush_debug)
        write_stream_flush_locked(logerrFile);
    }
    else if (lev == LOGLEVEL_WARN && prepare_file(logwarnFilepath, logwarnFile, 1 << LOGLEVEL_WARN))
    {
      if (!fmt)
        fmt = _fmt;
      out_file(logwarnFile, lc, t, term, LOGLEVEL_WARN, fmt, arg, anum);
      if (flush_debug)
        write_stream_flush_locked(logwarnFile);
    }
    else if (lev == LOGLEVEL_FATAL && fatalerrFilepath[0])
    {
      write_stream_t fp = file2stream(fopen(fatalerrFilepath, "at"));
      if (fp)
      {
        if (!fmt)
          fmt = _fmt;
        out_file(fp, lc, t, term, LOGLEVEL_FATAL, fmt, arg, anum);
        write_stream_close(fp);
      }
    }
  }

  (&dbg_ctx)->reset();
#if DEBUG_DO_PERIODIC_FLUSHES
  if (term && !flush_debug && get_time_msec() > next_flush_time)
  {
    debug_flush(false);
    next_flush_time = get_time_msec() + DEBUG_FLUSH_PERIOD;
  }
#endif
  (&dbg_ctx)->holdLine = false;
}

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3

static void get_home_or_temp_dir_name(char *buf, int size)
{
  const char *potentialDir = NULL;
  struct passwd *pw = getpwuid(getuid());
  if (pw)
    potentialDir = pw->pw_dir;
  if (!potentialDir)
  {
    potentialDir = getenv("TMPDIR");
    if (!potentialDir)
#if defined(P_tmpdir)
      potentialDir = P_tmpdir;
#else
      potentialDir = "/tmp";
#endif
  }
  G_ASSERT(potentialDir);
  strncpy(buf, potentialDir, size);
  buf[size - 1] = 0;
}

#endif

void start_debug_system(const char *exe_fname, const char *prefix, bool datetime_name)
{
  G_ASSERT(debug_ctors_inited);
  debug_enabled_bits = 0;
  maxWriteTimeUs = -1;

  memset(dbgFilepath, 0, sizeof(dbgFilepath));
  memset(logerrFilepath, 0, sizeof(logerrFilepath));
  memset(logwarnFilepath, 0, sizeof(logwarnFilepath));
  memset(fatalerrFilepath, 0, sizeof(fatalerrFilepath));
  memset(dbgCrashDumpPath, 0, sizeof(dbgCrashDumpPath));
  memset(logDirPath, 0, sizeof(logDirPath));

  memset(allowed_tags, 255, sizeof(allowed_tags));
  memset(ignored_tags, 255, sizeof(ignored_tags));
  memset(promoted_tags, 255, sizeof(promoted_tags));

  measure_cpu_freq();

  char buf[1024];
  prefix = prefix ? prefix : "";
  if (is_path_abs(prefix))
  {
    SNPRINTF(lastDbgPath, sizeof(lastDbgPath), "%s/last_debug", prefix);
    SNPRINTF(buf, sizeof(buf), "%s/%s", prefix, dd_get_fname(exe_fname));
  }
  else
  {
    G_VERIFY(getcwd(buf, sizeof(buf)) != NULL);
    buf[sizeof(buf) - 1] = 0;
#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3
    if (!buf[0] || strcmp(buf, "/") == 0)
      get_home_or_temp_dir_name(buf, sizeof(buf));
#endif
    SNPRINTF(lastDbgPath, sizeof(lastDbgPath), "%s/%slast_debug", buf, prefix);
    int sb = i_strlen(buf);
    _snprintf(buf + sb, sizeof(buf) - sb - 1, "/%s%s", prefix, dd_get_fname(exe_fname));
    buf[sizeof(buf) - 1] = 0;
  }

  if (datetime_name)
  {
    time_t rawtime;
    tm *t;

    time(&rawtime);
    t = localtime(&rawtime);
    int sb = i_strlen(buf);
    _snprintf(buf + sb, sizeof(buf) - sb - 1, "-%04d.%02d.%02d-%02d.%02d.%02d", 1900 + t->tm_year, t->tm_mon + 1, t->tm_mday,
      t->tm_hour, t->tm_min, t->tm_sec);
    if (dd_dir_exists(buf))
    {
      sb = i_strlen(buf);
      _snprintf(buf + sb, sizeof(buf) - sb - 1, "__%d", get_process_uid());
    }
  }

  SNPRINTF(dbgFilepath, DAGOR_MAX_PATH, "%s/debug", buf);
  SNPRINTF(logerrFilepath, DAGOR_MAX_PATH, "%s/logerr", buf);
  SNPRINTF(logwarnFilepath, DAGOR_MAX_PATH, "%s/logwarn", buf);
  SNPRINTF(fatalerrFilepath, DAGOR_MAX_PATH, "%s/fatalerr", buf);
  SNPRINTF(dbgCrashDumpPath, DAGOR_MAX_PATH, "%s/crashDump", buf);
  SNPRINTF(logDirPath, DAGOR_MAX_PATH, "%s/", buf);

  timestampEnabled = true;
  debug_enabled_bits = (1 << LOGLEVEL_DEBUG) | (1 << LOGLEVEL_ERR) | (1 << LOGLEVEL_WARN);
  unlink(dbgFilepath);
  unlink(logerrFilepath);
  unlink(logwarnFilepath);
  unlink(fatalerrFilepath);
#if DAGOR_FORCE_LOGS
  memset((void *)cryptLevCur, 0, sizeof(cryptLevCur));
#endif
#if DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0
  (&dbg_ctx)->threadId = 1;
#endif

#if DAGOR_DBGLEVEL > 0
  debug_enable_thread_ids(true);
#endif
}

void start_classic_debug_system(const char *debug_fname, bool en_time, bool hhmmss_fmt, int hhmmss_subsec, bool append_file)
{
  G_ASSERT(debug_ctors_inited);

  char fn_storage[DAGOR_MAX_PATH];
  const char *last_char_ptr = (debug_fname && debug_fname[0]) ? debug_fname + strlen(debug_fname) - 1 : NULL;
  bool crypted_logs = false;
  if (last_char_ptr && last_char_ptr > debug_fname + 1 && strcmp(last_char_ptr - 1, "/#") == 0)
  {
#if DAGOR_FORCE_LOGS
    // crypted logs
    crypt_debug_setup(get_default_log_crypt_key(), (60 << 20));
    crypted_logs = cryptKey != NULL;
#endif
    last_char_ptr--;
  }
  if (last_char_ptr && *last_char_ptr == '/')
  {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    SNPRINTF(fn_storage, sizeof(fn_storage) - 1, "%.*s/%04d_%02d_%02d_%02d_%02d_%02d__%d.%s", int(last_char_ptr - debug_fname),
      debug_fname, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, get_process_uid(),
      crypted_logs ? "clog" : "txt");
    debug_fname = fn_storage;
  }

  debug_enabled_bits = 0;
  maxWriteTimeUs = -1;
  memset(dbgFilepath, 0, sizeof(dbgFilepath));
  memset(logerrFilepath, 0, sizeof(logerrFilepath));
  memset(logwarnFilepath, 0, sizeof(logwarnFilepath));
  memset(lastDbgPath, 0, sizeof(lastDbgPath));
  memset(fatalerrFilepath, 0, sizeof(fatalerrFilepath));
  memset(dbgCrashDumpPath, 0, sizeof(dbgCrashDumpPath));
  memset(logDirPath, 0, sizeof(logDirPath));

  if (debug_fname && is_path_abs(debug_fname))
  {
    strcpy(fatalerrFilepath, debug_fname);
    strcat(fatalerrFilepath, ".fatal");
  }
  else
  {
    char *ret = getcwd(fatalerrFilepath, 255);
#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID
    if (!ret || !fatalerrFilepath[0] || strcmp(fatalerrFilepath, "/") == 0)
      get_home_or_temp_dir_name(fatalerrFilepath, sizeof(fatalerrFilepath));
#else
    G_UNUSED(ret);
#endif
    strncat(fatalerrFilepath, "/fatalerr", sizeof(fatalerrFilepath) - strlen(fatalerrFilepath) - 1);
  }

  debug_enable_timestamps(en_time, hhmmss_fmt, hhmmss_subsec);
  if (debug_fname)
  {
    measure_cpu_freq();
    if (*debug_fname == '*' || is_path_abs(debug_fname)) // stdout or abs path
      strncpy(dbgFilepath, debug_fname, sizeof(dbgFilepath) - 1);
    else
    {
      char buf[MAX_PATH];
      G_VERIFY(getcwd(buf, sizeof(buf)) != NULL);
      buf[sizeof(buf) - 1] = 0;
      SNPRINTF(dbgFilepath, DAGOR_MAX_PATH, "%s/%s", buf, debug_fname);
    }
    debug_enabled_bits = 1 << LOGLEVEL_DEBUG;

    strncpy(dbgCrashDumpPath, debug_fname, sizeof(dbgFilepath) - 1);
  }

  if (dbgFilepath[0])
  {
    debug_internal::append_files = append_file;
    if (append_file)
    {
      debug_internal::write_stream_set_fmode("ab");
      debug_internal::append_files_stage = dd_file_exists(dbgFilepath) ? 1 : 0;
    }
    else
      unlink(dbgFilepath);
    dd_get_fname_location(logDirPath, dbgFilepath);
  }
  unlink(fatalerrFilepath);
#if DAGOR_FORCE_LOGS
  memset((void *)cryptLevCur, 0, sizeof(cryptLevCur));
#endif
#if DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0
  (&dbg_ctx)->threadId = 1;
#endif

#if DAGOR_DBGLEVEL > 0
  debug_enable_thread_ids(true);
#endif
}

const char *debug_internal::get_logfilename_for_sending() { return dbgFilepath; }

const char *debug_internal::get_logging_directory() { return logDirPath; }

static struct DebugCppStaticCtorLast
{
  DebugCppStaticCtorLast() { debug_ctors_inited = 1; }
} staticCtorLast;

void debug_override_log_timestamp_format(debug_override_timestamp_cb_t cb) { override_timestamp_cb = cb; }

#endif // DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

void debug_internal::setupCrashDumpFileName()
{
  time_t rawtime;
  tm *t;
  time(&rawtime);
  t = localtime(&rawtime);

  size_t curlen = strlen(dbgCrashDumpPath);
  _snprintf(dbgCrashDumpPath + curlen, sizeof(dbgCrashDumpPath) - curlen, "-%02d.%02d.%02d.dmp", t->tm_hour, t->tm_min, t->tm_sec);

  logmessage(LOGLEVEL_FATAL, "setupCrashDumpFileName dbgCrashDumpPath = '%s'", dbgCrashDumpPath);
}

#define EXPORT_PULL dll_pull_kernel_debug
#include <supp/exportPull.h>
