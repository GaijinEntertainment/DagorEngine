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

using namespace debug_internal;

static void rotate_debug_files(const char *debugPath, const int count)
{
  G_ASSERT(count <= 9);
  char debug[DAGOR_MAX_PATH];
  memset(debug, '\0', DAGOR_MAX_PATH);
  SNPRINTF(debug, DAGOR_MAX_PATH - 2, "%s/debug", debugPath);
  const size_t digitIndex = i_strlen(debug);
  char &digit = debug[digitIndex];
  auto convertIndex = [](const size_t index) { return (index == 0) ? '\0' : ('0' + index); };
  size_t index = 0;
  digit = convertIndex(index);
  while (index <= count && dd_file_exists(debug))
  {
    index++;
    digit = convertIndex(index);
  }

  if (index > count)
  {
    digit = convertIndex(count);
    if (unlink(debug) != 0)
    {
      __android_log_print(ANDROID_LOG_WARN, "dagor", "Unable to remove debug log file: %s error: %d \n", debug, errno);
      return;
    }
    index--;
  }
  char debug1[DAGOR_MAX_PATH];
  memcpy(debug1, debug, DAGOR_MAX_PATH); // For copy nullterminators after string end
  while (index > 0)
  {
    index--;
    digit = convertIndex(index);
    debug1[digitIndex] = convertIndex(index + 1);
    if (dd_rename(debug, debug1) == 0)
    {
      __android_log_print(ANDROID_LOG_WARN, "dagor", "Unable to rename debug log file from: %s to: %s error: %d \n", debug, debug1,
        errno);
      return;
    }
  }
}

void setup_debug_system(const char *exe_fname, const char *prefix, bool datetime_name, const size_t rotatedCount)
{
#if DAGOR_FORCE_LOGS && DAGOR_DBGLEVEL == 0
  crypt_debug_setup(get_dagor_log_crypt_key(), 60 << 20 /* MAX_LOGS_FILE_SIZE */);
#endif

  char lastDbgPath[DAGOR_MAX_PATH];
  memset(dbgFilepath, 0, sizeof(dbgFilepath));
  char buf[1024];
  prefix = prefix ? prefix : "";

  SNPRINTF(lastDbgPath, sizeof(lastDbgPath), "%s/logs/last_debug", prefix);
  SNPRINTF(buf, sizeof(buf), "%s/logs/%s", prefix, dd_get_fname(exe_fname));

  if (datetime_name)
  {
    DagorDateTime t;
    get_local_time(&t);
    int sb = i_strlen(buf);
    SNPRINTF(buf + sb, sizeof(buf) - sb - 1, "-%04d.%02d.%02d-%02d.%02d.%02d", t.year, t.month, t.day, t.hour, t.minute, t.second);
  }
  else
  {
    rotate_debug_files(buf, rotatedCount);
  }
  SNPRINTF(dbgFilepath, DAGOR_MAX_PATH, "%s/debug", buf);
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
const char *debug_internal::get_logging_directory() { return ""; }


#ifdef __cplusplus
extern "C"
{
#endif

  JNIEXPORT void JNICALL Java_com_gaijinent_common_DagorLogger_nativeDebug(JNIEnv *jni, jobject, jstring msg)
  {
    const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
    logdbg("Android: %s", msgUtf8);
    jni->ReleaseStringUTFChars(msg, msgUtf8);
  }

  JNIEXPORT void JNICALL Java_com_gaijinent_common_DagorLogger_nativeError(JNIEnv *jni, jobject, jstring msg)
  {
    const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
    logerr("Android: %s", msgUtf8);
    jni->ReleaseStringUTFChars(msg, msgUtf8);
  }

  JNIEXPORT void JNICALL Java_com_gaijinent_common_DagorLogger_nativeWarning(JNIEnv *jni, jobject, jstring msg)
  {
    const char *msgUtf8 = jni->GetStringUTFChars(msg, NULL);
    logwarn("Android: %s", msgUtf8);
    jni->ReleaseStringUTFChars(msg, msgUtf8);
  }


#ifdef __cplusplus
}
#endif
