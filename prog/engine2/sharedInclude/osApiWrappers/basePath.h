#ifndef _DAGOR_OSAPIWRAPPER_BASE_PATH_H
#define _DAGOR_OSAPIWRAPPER_BASE_PATH_H
#pragma once
#include <string.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_baseDef.h>
#include <stdio.h>

static constexpr int DF_MAX_BASE_PATH_NUM = 64;

//! unused entries are always at the end and are NULLs
extern const char *df_base_path[DF_MAX_BASE_PATH_NUM];
extern bool df_base_path_vrom_mounted[DF_MAX_BASE_PATH_NUM];

void rebuild_basepath_vrom_mounted();

static inline const char *resolve_named_mount_in_path(const char *fpath, const char *&mnt_path)
{
  if (fpath && *fpath == '%')
    return dd_resolve_named_mount_in_path(fpath, mnt_path);
  mnt_path = "";
  return fpath;
}
static inline void resolve_named_mount_s(char *dest_buf, size_t dest_sz, const char *fpath)
{
  const char *mnt_path = "";
  fpath = resolve_named_mount_in_path(fpath, mnt_path);
  snprintf(dest_buf, dest_sz, "%s%s", mnt_path, fpath);
}


template <typename Cb>
static inline bool iterate_base_paths_l(const char *fname, Cb cb)
{
  const char *mnt_path = "";
  fname = resolve_named_mount_in_path(fname, mnt_path);
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM && df_base_path[i]; i++)
  {
    int base_len = (int)strlen(df_base_path[i]);
    char fn[DAGOR_MAX_PATH];
    snprintf(fn, sizeof(fn), "%s%s%s", df_base_path[i], mnt_path, fname);
    dd_simplify_fname_c(fn + base_len);
    if (cb(fn, base_len))
      return true;
  }
  return false;
}
template <typename Cb>
static inline const char *iterate_base_paths_fast_s(const char *fname, char *fnbuf, int fnbuf_sz, bool force_root_iter,
  bool force_simplify, Cb cb)
{
  bool root_tested = false;
  const char *mnt_path = "";
  fname = resolve_named_mount_in_path(fname, mnt_path);
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM && df_base_path[i]; i++)
  {
    bool root_bpath = (*df_base_path[i] == '\0');
    if (root_bpath)
      root_tested = true;
    if (!root_bpath || *mnt_path || PATH_DELIM != '/')
    {
      snprintf(fnbuf, fnbuf_sz, "%s%s%s", df_base_path[i], mnt_path, fname);
      if (PATH_DELIM != '/' || force_simplify)
        dd_simplify_fname_c(fnbuf);
      if (cb(fnbuf))
        return fnbuf;
    }
    else if (cb(fname))
      return fname;
  }

  if (force_root_iter && !root_tested)
  {
    if (*mnt_path || PATH_DELIM != '/')
    {
      snprintf(fnbuf, fnbuf_sz, "%s%s", mnt_path, fname);
      if (PATH_DELIM != '/' || force_simplify)
        dd_simplify_fname_c(fnbuf);
      fname = fnbuf;
    }
    if (cb(fname))
      return fname;
  }
  return nullptr;
}
template <typename Cb>
static inline bool iterate_base_paths_fast(const char *fname, Cb cb)
{
  char fn[DAGOR_MAX_PATH];
  return iterate_base_paths_fast_s(fname, fn, DAGOR_MAX_PATH, false, false, cb) != nullptr;
}
template <typename Cb>
static inline bool iterate_base_paths_simplify(const char *fname, Cb cb)
{
  char fn[DAGOR_MAX_PATH];
  return iterate_base_paths_fast_s(fname, fn, DAGOR_MAX_PATH, false, true, cb) != nullptr;
}
template <typename Cb>
static inline bool iterate_base_paths2(const char *fname1, const char *fname2, Cb cb)
{
  char fn1[DAGOR_MAX_PATH], fn2[DAGOR_MAX_PATH];
  const char *mnt_path1 = "", *mnt_path2 = "";
  fname1 = resolve_named_mount_in_path(fname1, mnt_path1);
  fname2 = resolve_named_mount_in_path(fname2, mnt_path2);
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM && df_base_path[i]; i++)
  {
    snprintf(fn1, sizeof(fn1), "%s%s%s", df_base_path[i], mnt_path1, fname1);
    dd_simplify_fname_c(fn1);
    snprintf(fn2, sizeof(fn1), "%s%s%s", df_base_path[i], mnt_path2, fname2);
    dd_simplify_fname_c(fn2);
    if (cb(fn1, fn2))
      return true;
  }
  return false;
}

static inline const char *get_rom_asset_fpath(const char *fn)
{
  if (strncmp(fn, "asset://", 8) == 0)
    return fn + 8;
#if _TARGET_ANDROID
  if (strncmp(fn, "file:///android_asset/", 22) == 0)
    return fn + 22;
#endif
  return nullptr;
}

static inline bool is_path_unc_win(const char *fn) { return (strncmp(fn, "\\\\", 2) == 0); }

static inline bool is_path_unc_unix(const char *fn) { return (strncmp(fn, "//", 2) == 0); }

static inline bool is_path_unc(const char *fn) { return is_path_unc_win(fn) || is_path_unc_unix(fn); }

static inline bool is_path_abs(const char *fn)
{
  if (fn[0] == '%')
    return false;
  if (is_path_unc(fn))
    return true;
#if _TARGET_C1 | _TARGET_C2 | _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID
  if (fn[0] == '/')
    return true;
  if (get_rom_asset_fpath(fn))
    return true;
#elif _TARGET_C3






#elif _TARGET_PC_WIN | _TARGET_XBOX
  if (strchr(fn, ':'))
    return true;
#endif

  return false;
}

#endif
