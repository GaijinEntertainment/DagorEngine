//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <stdio.h>
#include <string.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <sys/utime.h>
#elif _TARGET_PC | _TARGET_ANDROID | _TARGET_APPLE
#include <sys/time.h>
#else
#define DAG_CANNOT_SET_FTIME 1
#endif

namespace dag
{
inline bool remove_dirtree(const char *path)
{
  if (!path || !*path)
    return false;

  const char *realPath = ::df_get_real_folder_name(path);
  if (!realPath || !*realPath)
    return false;

  char mask[DAGOR_MAX_PATH], pathBuf[DAGOR_MAX_PATH];
  snprintf(mask, DAGOR_MAX_PATH, "%s/%s", realPath, "*"); // "*" is compatible with posix/windows, while "*.*" is windows-only

  alefind_t ff;
  for (bool ok = ::dd_find_first(mask, DA_FILE | DA_SUBDIR, &ff); ok; ok = ::dd_find_next(&ff))
    if (strcmp(ff.name, ".") != 0 && strcmp(ff.name, "..") != 0)
    {
      snprintf(pathBuf, DAGOR_MAX_PATH, "%s/%s", realPath, ff.name);
      dd_simplify_fname_c(pathBuf);
      if (ff.attr & DA_SUBDIR)
      {
        if (!remove_dirtree(pathBuf))
          break;
      }
      else if (!::dd_erase(pathBuf))
        break;
    }

  ::dd_find_close(&ff);
  return (bool)::dd_rmdir(realPath);
}

inline bool copy_file(const char *src_path, const char *dest_path)
{
  if (!dd_file_exists(src_path))
    return false;
  if (!dd_mkpath(dest_path))
    return false;

  bool result = false;
  if (file_ptr_t fpSrc = df_open(src_path, DF_READ))
  {
    if (file_ptr_t fpDest = df_open(dest_path, DF_CREATE | DF_WRITE))
    {
      static constexpr int BUF_SZ = 4 << 10;
      char buf[BUF_SZ];
      int size = df_length(fpSrc), len;
      while (size > 0)
      {
        len = (size > BUF_SZ) ? BUF_SZ : size;
        len = df_read(fpSrc, buf, len);
        if (len <= 0)
          break;
        if (df_write(fpDest, buf, len) != len)
          break;
        size -= len;
      }
      result = (size == 0);
      df_close(fpDest);

#ifndef DAG_CANNOT_SET_FTIME
      DagorStat st;
      if (df_fstat(fpSrc, &st) == 0)
      {
#if _TARGET_PC_WIN | _TARGET_XBOX
        utimbuf times = {st.atime, st.mtime};
        utime(dest_path, &times);
#elif _TARGET_PC | _TARGET_ANDROID | _TARGET_APPLE
        timeval times[2] = {{(time_t)st.atime, 0}, {(time_t)st.mtime, 0}};
        utimes(dest_path, times);
#endif
      }
#endif
    }
    df_close(fpSrc);
  }
  return result;
}
} // namespace dag

#undef DAG_CANNOT_SET_FTIME
