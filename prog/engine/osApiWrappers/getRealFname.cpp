#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/basePath.h>
#include <osApiWrappers/dag_vromfs.h>
#include <stdio.h>
#include <string.h>
#include "fs_hlp.h"

extern VromReadHandle vromfs_get_file_data_one(const char *fname, VirtualRomFsData **out_vrom);

static thread_local char frn_tls[512];

static inline const char *get_abs_vrom_name(const char *fname)
{
  return iterate_base_paths_fast_s(fname, frn_tls, sizeof(frn_tls), true, true, [](const char *fn) {
    VirtualRomFsData *out_vrom = nullptr;
    return vromfs_get_file_data_one(fn, &out_vrom).data() != nullptr;
  });
}
static inline const char *get_real_name(const char *fname, bool folder, bool allow_vrom = false)
{
  if (is_path_abs(fname))
  {
    char *full_real_name = frn_tls;
    strcpy(full_real_name, fname);
    dd_simplify_fname_c(full_real_name);
    if (!folder)
    {
      if (allow_vrom && vromfs_get_file_data_one(full_real_name, nullptr).data())
        return full_real_name;
      return check_file_exists(full_real_name) ? full_real_name : NULL;
    }
    return check_dir_exists(full_real_name) ? full_real_name : NULL;
  }

  if (!folder && allow_vrom && vromfs_first_priority)
    if (const char *abs_fn = get_abs_vrom_name(fname))
      return abs_fn;
  const char *full_real_name = iterate_base_paths_fast_s(fname, frn_tls, sizeof(frn_tls), false, true, [folder](const char *fn) {
    // this call will check if file exist AND is ready to use (with dag_on_file_pre_open)
    // if we want to check the filename regardless of our ability to read it (like it used to be), replace with check_file_exists_raw
    return (!folder && check_file_exists(fn)) || (folder && check_dir_exists(fn));
  });
  if (full_real_name)
    return full_real_name;

  if (!folder && allow_vrom && !vromfs_first_priority)
    if (const char *abs_fn = get_abs_vrom_name(fname))
      return abs_fn;

  return NULL;
}
static const char *prefer_src(const char *src, const char *result)
{
  return (result && (src == result || strcmp(src, result) == 0)) ? src : result;
}

const char *df_get_real_name(const char *fname) { return prefer_src(fname, get_real_name(fname, false)); }
const char *df_get_real_folder_name(const char *fname) { return prefer_src(fname, get_real_name(fname, true)); }
const char *df_get_abs_fname(const char *fname) { return prefer_src(fname, get_real_name(fname, false, true)); }

#define EXPORT_PULL dll_pull_osapiwrappers_getRealFname
#include <supp/exportPull.h>
