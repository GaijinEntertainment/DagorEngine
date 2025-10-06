// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_dbgStr.h>
#include <supp/_platform.h>
#include <string.h>
#include <stdio.h>

#if _TARGET_PC_WIN
const intptr_t invalid_console_handle = (intptr_t)INVALID_HANDLE_VALUE;
#else
const intptr_t invalid_console_handle = -1;
#endif

static intptr_t out_debug_file_handle = invalid_console_handle;

void set_debug_console_handle(intptr_t handle) { out_debug_file_handle = handle; }

intptr_t get_debug_console_handle() { return out_debug_file_handle; }

#if _TARGET_PC_WIN
void out_debug_str(const char *str)
{
  OutputDebugString(str);

#if DAGOR_DBGLEVEL > 0
  if (out_debug_file_handle != invalid_console_handle)
  {
    DWORD cWritten = 0;
    ::WriteFile((HANDLE)out_debug_file_handle, str, (int)strlen(str), &cWritten, NULL);
  }
#endif
}
#elif _TARGET_XBOX
__declspec(thread) char lineBuf[256]; // xbWatson writes each OutputDebugString on a new line ignoring the '\n'.
void out_debug_str(const char *str)
{
  while (str && str[0])
  {
    const char *split = strchr(str, '\n');
    split = split ? split + 1 : split;
    size_t strLen = split ? split - str : strlen(str);

    size_t bufLen = strlen(lineBuf);
    if (strLen + bufLen < sizeof(lineBuf))
    {
      memcpy(lineBuf + bufLen, str, strLen);
      lineBuf[bufLen + strLen] = 0;
      if (lineBuf[bufLen + strLen - 1] == '\n')
      {
        OutputDebugString(lineBuf);
        lineBuf[0] = 0;
      }
    }
    else
    {
      if (bufLen > 0)
      {
        OutputDebugString(lineBuf);
        lineBuf[0] = 0;
      }

      OutputDebugString(str);
      break;
    }

    str = split;
  }
}
#elif _TARGET_C1 | _TARGET_C2











#elif _TARGET_PC_LINUX | _TARGET_C3
void out_debug_str(const char *str)
{
  if (out_debug_file_handle != invalid_console_handle) // do quiet while output not requested
  {
    size_t len = strlen(str), written;
    do
    {
      written = fwrite(str, 1, len, (FILE *)out_debug_file_handle);
      len -= written;
    } while (len && written);
    fflush((FILE *)out_debug_file_handle);
  }
}
#elif _TARGET_ANDROID
#include <android/log.h>

void out_debug_str(const char *str)
{
  __android_log_write(ANDROID_LOG_DEBUG, "dagor", str);

#if DAGOR_DBGLEVEL > 0
  if (out_debug_file_handle != invalid_console_handle)
  {
    fputs(str, (FILE *)out_debug_file_handle);
    fflush((FILE *)out_debug_file_handle);
  }
#endif
}
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_dbgStr
#include <supp/exportPull.h>
