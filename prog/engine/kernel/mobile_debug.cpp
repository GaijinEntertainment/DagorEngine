// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_dbgStr.h>
#include "debugPrivate.h"
#include <errno.h>
#include <unistd.h>

char debug_internal::dbgFilepath[DAGOR_MAX_PATH];
static char logDirPath[DAGOR_MAX_PATH];

#if DAGOR_FORCE_LOGS && DAGOR_DBGLEVEL == 0
extern const unsigned MAX_LOG_FILE_SIZE;
#endif

using namespace debug_internal;

#if _TARGET_ANDROID
#include <android/log.h>
#include <android/native_activity.h>
#include <supp/android/jni_native_reg.h>
#define console_warning(fmt, ...) __android_log_print(ANDROID_LOG_WARN, "dagor", fmt, __VA_ARGS__)
#elif _TARGET_IOS
#import <asl.h>
static void ios_console_warning(const char *fmt, ...)
{
  static aslclient client = asl_open(NULL, "com.apple.console", 0);
  va_list ap;
  va_start(ap, fmt);
  asl_vlog(client, NULL, ASL_LEVEL_WARNING, fmt, ap);
  va_end(ap);
}
#define console_warning(fmt, ...) ios_console_warning(fmt, __VA_ARGS__)
#else
#error "Platform is not supported"
#endif

static int rotate_debug_files(const char *debug_path, const char *debug_name, const char *debug_ext, const int count)
{
  char debug[DAGOR_MAX_PATH];
  memset(debug, '\0', DAGOR_MAX_PATH);
  SNPRINTF(debug, sizeof(debug), "%s/", debug_path);

  const int filenameOffset = (int)strlen(debug);
  const int debugBufferLenght = sizeof(debug) - filenameOffset;
  char *debugFilename = debug + filenameOffset;

  SNPRINTF(debugFilename, debugBufferLenght, "%s0%s", debug_name, debug_ext);

  // Step 0: Check 'debug[0..Count]' and return first not existing log index
  for (int logIndex = 0; logIndex <= count;)
  {
    if (!dd_file_exists(debug))
      return logIndex;

    SNPRINTF(debugFilename, debugBufferLenght, "%s%d%s", debug_name, ++logIndex, debug_ext);
  }

  // Step 1: Remove 'debug0' - the oldest log file
  SNPRINTF(debugFilename, debugBufferLenght, "%s0%s", debug_name, debug_ext);
  if (unlink(debug) != 0)
  {
    console_warning("Unable to remove debug log file: %s error: %d \n", debug, errno);
    return 0;
  }

  char debug1[DAGOR_MAX_PATH];
  memcpy(debug1, debug, DAGOR_MAX_PATH); // For copy nullterminators after string end

  // Step 2: Shift log names like debug1 -> debug0, debug2 -> debug1, etc.
  for (int logIndex = 1; logIndex <= count; ++logIndex)
  {
    SNPRINTF(debugFilename, debugBufferLenght, "%s%d%s", debug_name, logIndex, debug_ext);
    SNPRINTF(debug1 + filenameOffset, sizeof(debug1) - filenameOffset, "%s%d%s", debug_name, logIndex - 1, debug_ext);

    if (!dd_rename(debug, debug1))
    {
      console_warning("Unable to rename debug log file from: %s to: %s error: %d \n", debug, debug1, errno);
      return 0;
    }
  }

  return count;
}

void setup_debug_system(const char *exe_fname, const char *prefix, const char *debug_fname, bool datetime_name,
  const size_t rotated_count)
{
#if DAGOR_FORCE_LOGS && DAGOR_DBGLEVEL == 0
  const char *logExt = ".clog";
  crypt_debug_setup(get_dagor_log_crypt_key(), MAX_LOG_FILE_SIZE);
#else
#if _TARGET_IOS
  const char *logExt = ".log";
#else
  const char *logExt = "";
#endif
#endif

#if _TARGET_IOS
  const char *logFolder = "";
#else
  const char *logFolder = "/logs";
#endif

  char logFilename[DAGOR_MAX_PATH];
  memset(logFilename, 0, sizeof(logFilename));
  strcpy(logFilename, debug_fname);

  char lastDbgPath[DAGOR_MAX_PATH];
  memset(dbgFilepath, 0, sizeof(dbgFilepath));
  prefix = prefix ? prefix : "";

  SNPRINTF(lastDbgPath, sizeof(lastDbgPath), "%s%s/last_debug", prefix, logFolder);
  SNPRINTF(logDirPath, sizeof(logDirPath), "%s%s/%s", prefix, logFolder, dd_get_fname(exe_fname));

  if (datetime_name)
  {
    DagorDateTime t;
    get_local_time(&t);
    int sb = (int)strlen(logDirPath);
    SNPRINTF(logDirPath + sb, sizeof(logDirPath) - sb - 1, "-%04d.%02d.%02d-%02d.%02d.%02d", t.year, t.month, t.day, t.hour, t.minute,
      t.second);
  }
  else if (rotated_count > 0)
  {
    const int logIndex = rotate_debug_files(logDirPath, debug_fname, logExt, rotated_count);
    SNPRINTF(logFilename, sizeof(logFilename), "%s%d", debug_fname, logIndex);
  }

  if (logExt && logExt[0] != '\0')
    strcat(logFilename, logExt);

  SNPRINTF(dbgFilepath, DAGOR_MAX_PATH, "%s/%s", logDirPath, logFilename);
  dd_mkpath(dbgFilepath);
  dd_mkpath(lastDbgPath);
  FILE *fp2 = fopen(lastDbgPath, "wt");
  if (fp2)
  {
    if (fwrite(dbgFilepath, 1, strlen(dbgFilepath), fp2))
    { // workaround against gcc's warn_unused_result warning
    }
    fclose(fp2);
  }
  else
    console_warning("Unable to open lastDebug file: %s  error:%d \n", lastDbgPath, errno);

  FILE *dbgfd = NULL;
  if (dbgFilepath[0] != '\0')
  {
#if DAGOR_FORCE_LOGS && DAGOR_DBGLEVEL == 0
    static const char *mode = "wb";
#else
    static const char *mode = "w";
#endif
    dbgfd = fopen(dbgFilepath, mode); // No df_open becose df_open use log, but now we not normal initialize work with logs
  }

  if (!dbgfd)
    console_warning("Unable to open debug log file: %s error: %d \n", dbgFilepath, errno);
  else
  {
#if _TARGET_IOS && DAGOR_DBGLEVEL > 0
    printf("Creating log file %s\n", dbgFilepath);
#endif
    set_debug_console_handle((intptr_t)dbgfd);
  }

  if (!is_path_abs(prefix))
    console_warning("Not absolute path prefix (%s) to debug file", prefix);
}

const char *debug_internal::get_logfilename_for_sending() { return dbgFilepath; }
const char *debug_internal::get_logging_directory() { return logDirPath; }

#if _TARGET_ANDROID

static void nativeDebug(JNIEnv *jni, jclass, jstring msg)
{
  if (get_debug_console_handle() != invalid_console_handle)
  {
    const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
    logdbg("Android: %s", msgUtf8);
    jni->ReleaseStringUTFChars(msg, msgUtf8);
  }
}

static void nativeWarning(JNIEnv *jni, jclass, jstring msg)
{
  if (get_debug_console_handle() != invalid_console_handle)
  {
    const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
    logwarn("Android: %s", msgUtf8);
    jni->ReleaseStringUTFChars(msg, msgUtf8);
  }
}

static void nativeError(JNIEnv *jni, jclass, jstring msg)
{
  if (get_debug_console_handle() != invalid_console_handle)
  {
    const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
    logerr("Android: %s", msgUtf8);
    jni->ReleaseStringUTFChars(msg, msgUtf8);
  }
}

JNI_REG_NATIVES(DagorLoggerNatives, "com.gaijinent.common.DagorLogger",
  JNI_NATIVE_METHOD(nativeDebug, JNI_SIGNATURE(JNI_VOID, JNI_STRING)),
  JNI_NATIVE_METHOD(nativeWarning, JNI_SIGNATURE(JNI_VOID, JNI_STRING)),
  JNI_NATIVE_METHOD(nativeError, JNI_SIGNATURE(JNI_VOID, JNI_STRING)));

#endif // _TARGET_ANDROID
