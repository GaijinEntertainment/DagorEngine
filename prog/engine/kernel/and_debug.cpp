// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_ANDROID
!compile error;
#endif
#include <osApiWrappers/basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <android/native_activity.h>
#include "debugPrivate.h"
#include <android/log.h>
#include <errno.h>
#include <unistd.h>

char debug_internal::dbgFilepath[DAGOR_MAX_PATH];
static char logDirPath[DAGOR_MAX_PATH];

using namespace debug_internal;

static int rotate_debug_files(const char *debugPath, const char *debugExt, const int count)
{
  char debug[DAGOR_MAX_PATH];
  memset(debug, '\0', DAGOR_MAX_PATH);
  SNPRINTF(debug, sizeof(debug), "%s/", debugPath);

  const int filenameOffset = (int)strlen(debug);
  const int debugBufferLenght = sizeof(debug) - filenameOffset;
  char *debugFilename = debug + filenameOffset;

  SNPRINTF(debugFilename, debugBufferLenght, "debug0%s", debugExt);

  // Step 0: Check 'debug[0..Count]' and return first not existing log index
  for (int logIndex = 0; logIndex <= count;)
  {
    if (!dd_file_exists(debug))
      return logIndex;

    SNPRINTF(debugFilename, debugBufferLenght, "debug%d%s", ++logIndex, debugExt);
  }

  // Step 1: Remove 'debug0' - the oldest log file
  SNPRINTF(debugFilename, debugBufferLenght, "debug0%s", debugExt);
  if (unlink(debug) != 0)
  {
    __android_log_print(ANDROID_LOG_WARN, "dagor", "Unable to remove debug log file: %s error: %d \n", debug, errno);
    return 0;
  }

  char debug1[DAGOR_MAX_PATH];
  memcpy(debug1, debug, DAGOR_MAX_PATH); // For copy nullterminators after string end

  // Step 2: Shift log names like debug1 -> debug0, debug2 -> debug1, etc.
  for (int logIndex = 1; logIndex <= count; ++logIndex)
  {
    SNPRINTF(debugFilename, debugBufferLenght, "debug%d%s", logIndex, debugExt);
    SNPRINTF(debug1 + filenameOffset, sizeof(debug1) - filenameOffset, "debug%d%s", logIndex - 1, debugExt);

    if (!dd_rename(debug, debug1))
    {
      __android_log_print(ANDROID_LOG_WARN, "dagor", "Unable to rename debug log file from: %s to: %s error: %d \n", debug, debug1,
        errno);
      return 0;
    }
  }

  return count;
}

void setup_debug_system(const char *exe_fname, const char *prefix, bool datetime_name, const size_t rotatedCount)
{
  static const char *BASE_LOG_NAME = "debug";

#if DAGOR_FORCE_LOGS && DAGOR_DBGLEVEL == 0
  const char *logExt = ".clog";
  crypt_debug_setup(get_dagor_log_crypt_key(), 60 << 20 /* MAX_LOGS_FILE_SIZE */);
#else
  const char *logExt = "";
#endif

  char logFilename[DAGOR_MAX_PATH];
  memset(logFilename, 0, sizeof(logFilename));
  strcpy(logFilename, BASE_LOG_NAME);

  char lastDbgPath[DAGOR_MAX_PATH];
  memset(dbgFilepath, 0, sizeof(dbgFilepath));
  prefix = prefix ? prefix : "";

  SNPRINTF(lastDbgPath, sizeof(lastDbgPath), "%s/logs/last_debug", prefix);
  SNPRINTF(logDirPath, sizeof(logDirPath), "%s/logs/%s", prefix, dd_get_fname(exe_fname));

  if (datetime_name)
  {
    DagorDateTime t;
    get_local_time(&t);
    int sb = (int)strlen(logDirPath);
    SNPRINTF(logDirPath + sb, sizeof(logDirPath) - sb - 1, "-%04d.%02d.%02d-%02d.%02d.%02d", t.year, t.month, t.day, t.hour, t.minute,
      t.second);
  }
  else
  {
    const int logIndex = rotate_debug_files(logDirPath, logExt, rotatedCount);
    SNPRINTF(logFilename, sizeof(logFilename), "%s%d", BASE_LOG_NAME, logIndex);
  }

#if DAGOR_FORCE_LOGS && DAGOR_DBGLEVEL == 0
  strcat(logFilename, logExt);
#endif

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
  {
    __android_log_print(ANDROID_LOG_WARN, "dagor", "Unable to open lastDebug file: %s  error:%d \n", lastDbgPath, errno);
  }

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
    __android_log_print(ANDROID_LOG_WARN, "dagor", "Unable to open debug log file: %s error: %d \n", dbgFilepath, errno);
  else
    set_debug_console_handle((intptr_t)dbgfd);

  if (!is_path_abs(prefix))
    __android_log_print(ANDROID_LOG_WARN, "dagor", "Not absolute path prefix (%s) to debug file", prefix);
}

const char *debug_internal::get_logfilename_for_sending() { return dbgFilepath; }
const char *debug_internal::get_logging_directory() { return logDirPath; }


#ifdef __cplusplus
extern "C"
{
#endif

  JNIEXPORT void JNICALL Java_com_gaijinent_common_DagorLogger_nativeDebug(JNIEnv *jni, jobject, jstring msg)
  {
    if (get_debug_console_handle() != invalid_console_handle)
    {
      const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
      logdbg("Android: %s", msgUtf8);
      jni->ReleaseStringUTFChars(msg, msgUtf8);
    }
  }

  JNIEXPORT void JNICALL Java_com_gaijinent_common_DagorLogger_nativeError(JNIEnv *jni, jobject, jstring msg)
  {
    if (get_debug_console_handle() != invalid_console_handle)
    {
      const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
      logerr("Android: %s", msgUtf8);
      jni->ReleaseStringUTFChars(msg, msgUtf8);
    }
  }

  JNIEXPORT void JNICALL Java_com_gaijinent_common_DagorLogger_nativeWarning(JNIEnv *jni, jobject, jstring msg)
  {
    if (get_debug_console_handle() != invalid_console_handle)
    {
      const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
      logwarn("Android: %s", msgUtf8);
      jni->ReleaseStringUTFChars(msg, msgUtf8);
    }
  }


#ifdef __cplusplus
}
#endif
