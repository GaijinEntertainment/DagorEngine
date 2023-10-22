#include <osApiWrappers/dag_dbgStr.h>
#include <stdio.h>
#import <Foundation/Foundation.h>
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
#import <os/log.h>
#else
#import <asl.h>
#endif

const intptr_t invalid_console_handle = -1;
static intptr_t out_debug_console_handle = invalid_console_handle;

void set_debug_console_handle(intptr_t handle)
{
  out_debug_console_handle = handle;
}

intptr_t get_debug_console_handle()
{
  return out_debug_console_handle;
}

void out_debug_str(const char *str)
{
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  os_log_info(OS_LOG_DEFAULT, "%s", str);
#else
  static aslclient client = asl_open(NULL, "com.apple.console", 0);
  asl_log(client, NULL, ASL_LEVEL_NOTICE, "%s", str);
#endif
  if (out_debug_console_handle != invalid_console_handle) // do quiet while output not requested
  {
    fwrite(str, 1, strlen(str), (FILE*)out_debug_console_handle);
    fflush((FILE*)out_debug_console_handle);
  }
}


#if _TARGET_IOS|_TARGET_TVOS
static bool only_file_log = false;

void set_debug_console_ios_file_output()
{
  only_file_log = true;
  out_debug_console_handle = (intptr_t)stdout;
}

bool is_debug_console_ios_file_output()
{
  return only_file_log;
}
#endif
