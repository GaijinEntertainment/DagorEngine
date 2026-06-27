//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_basePath.h>
#include <util/dag_string.h>

#include <ctype.h>

// Prepend %appDir/ to project-relative paths; leave named-mount (leading %) and absolute paths untouched
static inline void make_eff_app_relative_path(String &dest, const char *path, bool make_dir_path = false)
{
  if (!path || !*path)
  {
    dest = "";
    return;
  }

  if (*path == '%')
    dest = path;
    // The function is supposed to return absolute paths as is, but we cannot do the same for linux yet
    // linux absolute path should start with leading '/', but some relative paths in blk start with leading '/'
    // so they all will be falsely returned as absolute
#if _TARGET_PC_WIN
  else if ((isalpha(*path) && path[1] == ':') || (*path == '\\' && path[1] == '\\'))
    dest = path;
#endif
  else
    dest.setStrCat("%appDir/", path);
  dd_resolve_named_mount_inplace(dest);
  if (make_dir_path)
    dest += "/";
  simplify_fname(dest);
}

static inline String make_eff_app_relative_path(const char *path, bool make_dir_path = false)
{
  String final_path;
  make_eff_app_relative_path(final_path, path, make_dir_path);
  return final_path;
}
