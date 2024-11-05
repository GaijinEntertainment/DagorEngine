// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_hlp.h"
#include <supp/_platform.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <errno.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <osApiWrappers/basePath.h>
#include <util/dag_globDef.h>
#if _TARGET_PC_WIN
#include <osApiWrappers/dag_unicode.h>
#include <windows.h>
#endif
#include "romFileReader.h"

#if !defined(_TARGET_PC_WIN) && !defined(_TARGET_XBOX)
#define _mkdir(f) mkdir(f)
#endif // !_TARGET_PC_WIN && !_TARGET_XBOX

// auxilary routines
static inline const char *remove_leading_dot_slash(const char *fn)
{
  if (fn[0] == '.' && (fn[1] == PATH_DELIM || fn[1] == PATH_DELIM_BACK))
    return fn + 2;
  return fn;
}

static bool rename_internal(const char *src, const char *dst)
{
#if _TARGET_PC_WIN
  wchar_t wsrc[DAGOR_MAX_PATH], wdst[DAGOR_MAX_PATH];
  int sleepMs = 13;
  do
  {
    if (MoveFileExW(utf8_to_wcs(src, wsrc, countof(wsrc)), utf8_to_wcs(dst, wdst, countof(wdst)),
          MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
      return true;
    if (GetLastError() != ERROR_ACCESS_DENIED)
      break;
    SleepEx(sleepMs, TRUE);
    sleepMs *= 2;
  } while (sleepMs < 100);
  return false;
#else
#if _TARGET_XBOX || _TARGET_C3
  remove(dst);
#endif
  return rename(src, dst) == 0;
#endif
}

// directory services wrappers
extern "C" bool dd_rename(const char *oldname, const char *newname)
{
  oldname = remove_leading_dot_slash(oldname);
  newname = remove_leading_dot_slash(newname);

  if (is_path_abs(oldname))
    return rename_internal(oldname, newname);

  return iterate_base_paths2(oldname, newname, [](const char *ofn, const char *nfn) { return rename_internal(ofn, nfn); });
}

extern "C" bool dd_erase(const char *filename)
{
  filename = remove_leading_dot_slash(filename);
  if (is_path_abs(filename))
  {
    int result = remove(filename);
    if (dag_on_file_was_erased)
      dag_on_file_was_erased(filename);

    return result == 0;
  }

  return iterate_base_paths_l(filename, [](const char *fn, int) {
    if (::remove(fn) != 0)
      return false;
    if (dag_on_file_was_erased)
      dag_on_file_was_erased(fn);
    return true;
  });
}

static bool isMountPointName(const char *fn)
{
  // mount points on:
  //  nintendo -> rom:/ save:/
  //  windows  -> c:/ d:/
  //  unix     -> /path
  int len = strlen(fn);
  return len > 1 && fn[len - 1] == ':';
}

static bool mkdirRecursive(char *fn, int start_pos)
{
  char *pslash = strchr(&fn[start_pos], PATH_DELIM);
#if _TARGET_C3



#endif
  while (pslash)
  {
    *pslash = '\0';
    int ret = fn[0] && !isMountPointName(fn) ? ::_mkdir(fn) : 0;
    (void)(ret);
    *pslash = PATH_DELIM;
    pslash = strchr(pslash + 1, PATH_DELIM);
  }

  if (!fn[0] || ::mkdir(fn) == 0 || errno == EEXIST)
    return true;
  return false;
}

extern "C" bool dd_mkdir(const char *filename)
{
  filename = remove_leading_dot_slash(filename);

  if (is_path_abs(filename))
  {
    char fn[DAGOR_MAX_PATH];
    strcpy(fn, filename);
    dd_simplify_fname_c(fn);
    return mkdirRecursive(fn, 0);
  }

  return iterate_base_paths_l(filename, [](char *fn, int base_len) { return mkdirRecursive(fn, base_len); });
}

extern "C" bool dd_rmdir(const char *filename)
{
  filename = remove_leading_dot_slash(filename);

  if (is_path_abs(filename))
  {
    char fn[DAGOR_MAX_PATH];
    strcpy(fn, filename);
    dd_simplify_fname_c(fn);
    return ::rmdir(fn) == 0;
  }

  return iterate_base_paths_l(filename, [](const char *fn, int) { return ::rmdir(fn) == 0; });
}

extern "C" bool dd_mkpath(const char *path)
{
  if (!path || !*path)
    return false;
  if (strncmp(path, "b64://", 6) == 0)
    return false;
  if (strncmp(path, "asset://", 8) == 0)
    return false;

  char fname[DAGOR_MAX_PATH];
  strncpy(fname, path, DAGOR_MAX_PATH - 1);
  fname[DAGOR_MAX_PATH - 1] = '\0';

  dd_simplify_fname_c(fname);

  char *pslash = strrchr(fname, PATH_DELIM);
  bool ret = true;
  if (pslash)
  {
    *pslash = '\0';

    if (!::dd_dir_exist(fname))
      ret = dd_mkdir(fname);

    *pslash = PATH_DELIM;
  }

  return ret;
}

extern "C" bool dd_file_exist(const char *filename)
{
  filename = remove_leading_dot_slash(filename);
  if (strncmp(filename, "b64://", 6) == 0)
    return true;
  if (const char *asset_fn = get_rom_asset_fpath(filename))
    return RomFileReader::getAssetSize(asset_fn) >= 0;

  if (char c = *filename ? filename[strlen(filename) - 1] : '\0'; !c || c == '\\' || c == '/')
    return false;

  if (vromfs_check_file_exists(filename))
    return true;

  if (is_path_abs(filename))
  {
    char fn[DAGOR_MAX_PATH];
    strncpy(fn, filename, DAGOR_MAX_PATH - 1);
    fn[DAGOR_MAX_PATH - 1] = 0;
    dd_simplify_fname_c(fn);
    if (check_file_exists(fn))
      return true;
    return false; // no need in searching abs path using base paths
  }

  return iterate_base_paths_fast(filename, [](const char *fn) { return check_file_exists(fn); });
}

extern "C" bool dd_dir_exist(const char *filename)
{
  filename = remove_leading_dot_slash(filename);

  // if it absolute path we not need find with other basepaths
  if (is_path_abs(filename))
    return check_dir_exists(filename);

  return iterate_base_paths_fast(filename, [](const char *fn) { return check_dir_exists(fn); });
}


const char *dd_get_fname_without_path_and_ext(char *buf, int buf_size, const char *path)
{
  if (!path || !path[0])
    return path;
  strncpy(buf, path, buf_size);
  dd_simplify_fname_c(buf);
  char *ext = (char *)dd_get_fname_ext(buf);
  if (ext)
    *ext = '\0';
  char *delim = strrchr(buf, PATH_DELIM);
  if (!delim)
    delim = strrchr(buf, PATH_DELIM_BACK);
  return delim ? delim + 1 : buf;
}

char *dd_get_fname_location(char *buf, const char *filename)
{
  if (strncmp(filename, "b64://", 6) == 0)
  {
    *buf = '\0';
    return buf;
  }
  if (buf != filename)
    strcpy(buf, filename);
  dd_simplify_fname_c(buf);

  char *p = strrchr(buf, PATH_DELIM);
  if (!p)
    p = strchr(buf, ':');

  if (p)
    p++;
  else
    p = buf;

  *p = '\0';
  return buf;
}

const char *dd_get_fname_ext(const char *path)
{
  if (!path || !path[0])
    return NULL;

  const char *p = strrchr(path, PATH_DELIM_BACK);
  if (!p)
    p = path;

  const char *p1 = strrchr(p, PATH_DELIM);
  if (p1)
    p = p1;

  return strrchr(p, '.');
}

const char *dd_get_fname(const char *path)
{
  if (!path || !path[0])
    return path;

  const char *p = strrchr(path, PATH_DELIM_BACK);
  if (p)
    p++;
  else
    p = path;

  const char *p1 = strrchr(p, PATH_DELIM);
  if (p1)
    p = p1 + 1;

  return p;
}

#include <startup/dag_fs_utf8.inc.cpp>

#define EXPORT_PULL dll_pull_osapiwrappers_directoryService
#include <supp/exportPull.h>
