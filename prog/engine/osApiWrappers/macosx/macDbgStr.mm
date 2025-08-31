// Copyright (C) Gaijin Games KFT.  All rights reserved.

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

#if _TARGET_IOS
extern char ios_global_log_fname[];

namespace debug_internal
{
const char *get_logfilename_for_sending() { return ios_global_log_fname; }
const char *get_logging_directory() { return ""; }
}
#endif

#if _TARGET_IOS|_TARGET_TVOS
static bool only_file_log = false;
static bool copy_log_to_console = false;

void set_debug_console_ios_file_output(bool val)
{
  only_file_log = val;
  out_debug_console_handle = (intptr_t)stdout;
}

bool is_debug_console_ios_file_output()
{
  return only_file_log;
}

void set_copy_debug_to_ios_console(bool val)
{
  copy_log_to_console = val;
}

bool is_enabled_copy_debug_to_ios_console()
{
  return copy_log_to_console;
}
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_dbgStr
#include <supp/exportPull.h>
