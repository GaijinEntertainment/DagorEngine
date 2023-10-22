#pragma once

#if _TARGET_C1 | _TARGET_C2

#elif defined(__GNUC__)
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/stat.h>
#elif _TARGET_PC_WIN | _TARGET_XBOX
#include <io.h>
#include <direct.h>
#include <string.h>
#include <sys/stat.h>
#endif

#if _TARGET_C3

#endif

#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/basePath.h>
#include "romFileReader.h"


#if _TARGET_XBOX
// On xbox stat fails when path has mixed slashes
// Fixup UNC path to have \ slashes everywhere
#define FIXUP_UNC_SLASHES(fname)               \
  char fileName[DAGOR_MAX_PATH];               \
  if (is_path_unc_win(fname))                  \
  {                                            \
    strcpy(fileName, fname);                   \
    for (size_t i = 2; i < strlen(fname); ++i) \
    {                                          \
      if (fileName[i] == '/')                  \
        fileName[i] = '\\';                    \
    }                                          \
    fname = fileName;                          \
  }
#else
#define FIXUP_UNC_SLASHES(fname)
#endif


#if !(_TARGET_C1 | _TARGET_C2)
static inline bool check_file_exists_raw(const char *fname)
{
  if (const char *asset_fn = get_rom_asset_fpath(fname))
    return RomFileReader::getAssetSize(asset_fn) >= 0;
#if _TARGET_C3

#else
  FIXUP_UNC_SLASHES(fname);
  struct stat st = {0};
  if (::stat(fname, &st))
    return false;
  return (st.st_mode & S_IFMT) != S_IFDIR && (st.st_mode & 0444);
#endif
}

static inline bool check_dir_exists(const char *dname)
{
#if _TARGET_C3

#else
  FIXUP_UNC_SLASHES(dname);
  struct stat st = {0};
  if (::stat(dname, &st))
    return false;
  return (st.st_mode & S_IFMT) == S_IFDIR;
#endif
}
#endif

static inline bool check_file_exists(const char *fname)
{
  return (dag_on_file_pre_open && !dag_on_file_pre_open(fname)) ? false : check_file_exists_raw(fname);
}

#if defined(__GNUC__)
static int mkdir(const char *path) { return mkdir(path, 0777); }
#endif


// Implementation of the UN*X wildcards
// Supported wild-characters: '*', '?'
int wildcardfit(const char *wildcard, const char *test);

template <typename T>
inline bool is_special_dir(const T *name)
{
  return name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0));
}
