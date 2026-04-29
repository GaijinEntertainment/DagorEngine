// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_directUtils.h>
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
bool remove_dirtree(const char *path)
{
  if (!path || !*path)
    return false;

  const char *tempRealPath = ::df_get_real_folder_name(path);
  if (!tempRealPath || !*tempRealPath)
    return false;

  char realPath[DAGOR_MAX_PATH];
  strncpy(realPath, tempRealPath, DAGOR_MAX_PATH - 1);
  realPath[DAGOR_MAX_PATH - 1] = '\0';

  char mask[DAGOR_MAX_PATH], pathBuf[DAGOR_MAX_PATH];
  snprintf(mask, DAGOR_MAX_PATH, "%s/%s", realPath, "*"); // "*" is compatible with posix/windows, while "*.*" is windows-only

  for (const alefind_t &ff : dd_find_iterator(mask, DA_FILE | DA_SUBDIR))
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
  return (bool)::dd_rmdir(realPath);
}

bool copy_file(const char *src_path, const char *dest_path)
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

bool copy_folder(const char *src_folder, const char *dst_folder)
{
  if (src_folder == nullptr || dst_folder == nullptr)
    return false;
  if (!dd_dir_exist(dst_folder))
    dd_mkdir(dst_folder);

  char mask[DAGOR_MAX_PATH];
  snprintf(mask, DAGOR_MAX_PATH, "%s/*", src_folder);

  bool success = true;
  for (const alefind_t &ff : dd_find_iterator(mask, DA_FILE | DA_SUBDIR))
    if (strcmp(ff.name, ".") != 0 && strcmp(ff.name, "..") != 0)
    {
      char srcPath[DAGOR_MAX_PATH], dstPath[DAGOR_MAX_PATH];
      snprintf(srcPath, DAGOR_MAX_PATH, "%s/%s", src_folder, ff.name);
      snprintf(dstPath, DAGOR_MAX_PATH, "%s/%s", dst_folder, ff.name);
      if (ff.attr & DA_SUBDIR)
        success &= copy_folder(srcPath, dstPath);
      else
        success &= copy_file(srcPath, dstPath);
    }
  return success;
}

bool move_folder(const char *src_folder, const char *dst_folder)
{
  if (dd_rename(src_folder, dst_folder))
    return true;

  // can fail if folders are moved between drives - fallback to copy&delete
  if (!copy_folder(src_folder, dst_folder))
    return false;
  if (!remove_dirtree(src_folder))
  {
    // undo, and return an error
    remove_dirtree(dst_folder);
    return false;
  }
  return true;
}

} // namespace dag

#undef DAG_CANNOT_SET_FTIME

#define EXPORT_PULL dll_pull_osapiwrappers_directoryUtils
#include <supp/exportPull.h>
