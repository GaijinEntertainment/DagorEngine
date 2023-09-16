//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

#ifdef __cplusplus
extern "C"
{
#endif

  //! adds base_path to list (path is copied to local storage, so no strdup() needed)
  KRNLIMP bool dd_add_base_path(const char *base_path, bool insert_first = false);

  //! removes base_path from list, returns pointer removed from list (pointer to local storage must not be free'd)
  KRNLIMP const char *dd_remove_base_path(const char *base_path);

  //! clears list of base pathes
  KRNLIMP void dd_clear_base_paths();

  //! dumps list of base pathes to output
  KRNLIMP void dd_dump_base_paths();

  //! iterates over base pathes, returns NULL when finished
  KRNLIMP const char *df_next_base_path(int *index_ptr);


  static constexpr int MAX_MOUNT_NAME_LEN = 64;

  //! sets path for named mount (or removed named point if path_to==nullptr)
  //! mount_name must consist of C-ident chars only (letters, digits, underscore) and be no longer than MAX_MOUNT_NAME_LEN
  //! mount_name may start with '%', named mounts doesn't store this implied prefix
  //! named mounts may be used later in paths using scheme: %mount_name/path/to/file
  KRNLIMP void dd_set_named_mount_path(const char *mount_name, const char *path_to);

  //! returns path assigned for named mount (when mount exists) or nullptr (when mount_name not known)
  KRNLIMP const char *dd_get_named_mount_path(const char *mount_name, int mount_name_len = -1);

  //! dumps list of named mounts (with assigned pathes) to output
  KRNLIMP void dd_dump_named_mounts();

  //! resolves fpath that (optionaly) contains named mount into mnt_path and the rest of path
  //! returns substring of fpath to appended to mnt_path to get resolved filepath
  KRNLIMP const char *dd_resolve_named_mount_in_path(const char *fpath, const char *&mnt_path);

  //! checks whether named mount is valid
  //! returns true when no named mount used or it is correct, returns false when missing named mount is specified in path
  KRNLIMP bool dd_check_named_mount_in_path_valid(const char *fpath);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
//! resolves fpath that contains named mount; returns true when named mount detected and result path copied to dest
template <class STR>
inline bool dd_resolve_named_mount(STR &dest, const char *fpath)
{
  const char *mnt_path = "";
  const char *fpath_suffix = dd_resolve_named_mount_in_path(fpath, mnt_path);
  if (fpath == fpath_suffix)
    return false;
  dest = STR(mnt_path) + fpath_suffix;
  return true;
}
//! resolves fpath that contains named mount inplace
template <class STR>
inline void dd_resolve_named_mount_inplace(STR &dest)
{
  dd_resolve_named_mount(dest, dest.c_str());
}
#endif

#include <supp/dag_undef_COREIMP.h>
