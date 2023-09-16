#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID | _TARGET_C3

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/basePath.h>
#include "fs_hlp.h"
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#if _TARGET_C3

#else
#define HAVE_STAT 1

DIR *opendir_impl(const char *path) { return opendir(path); }
void closedir_impl(DIR *handle) { closedir(handle); }
dirent *readdir_impl(DIR *handle, dirent *) { return readdir(handle); }

#endif

struct RealFind
{
  dirent direntry;
  struct stat st;
  const char *wildcard; // ptr to alefind_t.fmask
  char path[DAGOR_MAX_PATH];
  char *pathEnd; // points to path's terminator
  DIR *handle;
  bool subdir;
  bool statSuccess;

  RealFind() { memset(this, 0, sizeof(*this)); }

  inline bool isDir() const { return (direntry.d_type == DT_DIR); }

  inline void callStat()
  {
#if HAVE_STAT
    statSuccess = (stat(path, &st) == 0);
#elif HAVE_FSTAT
    int filehandle = 0;
    if ((filehandle = ::open(path, O_RDONLY)) < -1)
      statSuccess = false;
    else
    {
      statSuccess = (fstat(filehandle, &st) == 0);
      ::close(filehandle);
    }
#else
#error RealFind::callStat() requires stat()/fstat() functions
#endif

#if defined(S_ISDIR)
    // Some filesystems (XFS for example) always return DT_UNKNOWN in direntry
    // and stat/fstat should be used to determine the actual type.
    // Since we're only interested in directories, don't check other types
    if (statSuccess && direntry.d_type == DT_UNKNOWN)
    {
      if (S_ISDIR(st.st_mode))
        direntry.d_type = DT_DIR;
    }
#else
#error RealFind::callStat() requires S_ISDIR macro
#endif
  }

  inline void prepareResult()
  {
    direntry.d_name[sizeof(direntry.d_name) - 1] = 0;

    *pathEnd = '\0';
    strncat(path, direntry.d_name, sizeof(path) - (pathEnd - path) - 1);

    callStat();
  }

  inline void copyTo(alefind_t &fs)
  {
    strncpy(fs.name, direntry.d_name, sizeof(fs.name));
    fs.name[sizeof(fs.name) - 1] = '\0';
    fs.attr = isDir() ? (DA_SUBDIR | DA_READONLY) : (0 | DA_READONLY);

    if (statSuccess)
    {
      fs.size = st.st_size;
      fs.atime = st.st_atime;
      fs.mtime = st.st_mtime;
      fs.ctime = st.st_ctime;
    }
    else
    {
      fs.size = 0;
      fs.atime = fs.mtime = fs.ctime = -1;
    }
  }

public:
  static constexpr int MAX_RF = 32;
  static RealFind rfPool[MAX_RF];
  static WinCritSec critSec;

  static RealFind *open(DIR *fd)
  {
    WinAutoLock lock(critSec);
    for (int i = 0; i < MAX_RF; i++)
      if (!rfPool[i].handle)
      {
        memset(&rfPool[i], 0, sizeof(rfPool[i]));
        rfPool[i].handle = fd;
        return &rfPool[i];
      }
    return NULL;
  }
  static bool close(RealFind *rd)
  {
    WinAutoLock lock(critSec);
    if (rd < &rfPool[0] || rd >= &rfPool[MAX_RF])
      return false;

    int i = rd - &rfPool[0];
    if (rfPool[i].handle == nullptr)
      return false;

    closedir_impl(rfPool[i].handle);

    memset(&rfPool[i], 0, sizeof(rfPool[i]));
    rfPool[i].handle = NULL;
    return true;
  }
};

RealFind RealFind::rfPool[MAX_RF];
WinCritSec RealFind::critSec;

static bool find_next(RealFind *rf, bool dir)
{
  for (;;)
  {
    dirent *de = nullptr;
#if _TARGET_C3




#endif
    de = readdir_impl(rf->handle, de);
    if (!de)
      return false;
    memcpy(&rf->direntry, de, de->d_reclen);
    rf->direntry.d_name[sizeof(rf->direntry.d_name) - 1] = 0;

    if (is_special_dir(rf->direntry.d_name))
      continue;

    if (!wildcardfit(rf->wildcard, rf->direntry.d_name))
      continue;

    rf->prepareResult();

    if (dir || !rf->isDir())
      return true;
  }
  return false;
}

static bool bp_find_first(bool dir, alefind_t *fs, const char *basepath)
{
  char msk[DAGOR_MAX_PATH];
  char path[DAGOR_MAX_PATH];

  // printf("find first basepath=%s\n", basepath);
  if (fs == NULL)
    return false;

  strncpy(msk, basepath, sizeof(msk));
  msk[sizeof(msk) - 1] = '\0';
  strncat(msk, fs->fmask, sizeof(msk) - strlen(msk) - 1);
  dd_simplify_fname_c(msk);

  // findfirst
  dd_get_fname_location(path, msk);
  const char *wildcard = dd_get_fname(fs->fmask);
  // printf("dir=%d mask:%s basepath:%s full:%s path:%s name:%s\n", dir, fs->fmask, basepath, msk, path, wildcard);

  DIR *fd = opendir_impl(path);
  if (!fd)
    return false;

  if (fs->data == NULL) // struct could be already allocated
  {
    fs->data = RealFind::open(fd);
    if (!fs->data)
    {
      closedir_impl(fd);
      return false;
    }
  }
  else
    closedir_impl(fd);

  RealFind *rf = (RealFind *)fs->data;
  rf->wildcard = wildcard;
  strncpy(rf->path, path, sizeof(rf->path) - 1);
  rf->pathEnd = rf->path + strlen(rf->path);
  rf->subdir = dir;

  if (find_next(rf, dir))
  {
    rf->copyTo(*fs);
    return true;
  }

  return false;
}


static int bp_find_next(alefind_t *fs)
{
  if (fs == NULL || fs->data == NULL)
    return 0;
  // printf("find next=%s\n", fs->fmask);

  RealFind *rf = (RealFind *)fs->data;
  if (find_next(rf, rf->subdir))
  {
    rf->copyTo(*fs);
    return 1;
  }
  return 0;
}

static int bp_find_close(alefind_t *fs)
{
  // printf("find close %08x\n", fs);
  if (fs == NULL || fs->data == NULL)
    return 0;

  RealFind::close((RealFind *)fs->data);
  fs->data = NULL;

  return 1;
}

static int find_in_basepath(alefind_t *fs, int grp)
{
  for (int i = grp; i < DF_MAX_BASE_PATH_NUM; i++)
  {
    if (df_base_path[i] == NULL)
      break;

    if (bp_find_first((fs->fattr & DA_SUBDIR) ? true : false, fs, df_base_path[i]))
    {
      fs->grp = i;
      return 1;
    }
  }
  bp_find_close(fs);
  return 0;
}

extern "C" int dd_find_first(const char *mask, char attr, alefind_t *fs)
{
  if (fs == NULL)
    return 0;
  fs->grp = -1; // group directory index
  fs->fattr = attr;
  fs->data = NULL;

  resolve_named_mount_s(fs->fmask, sizeof(fs->fmask), mask);
  mask = fs->fmask;

  if (is_path_abs(mask))
  {
    if (bp_find_first((fs->fattr & DA_SUBDIR) ? true : false, fs, ""))
    {
      fs->grp = -2;
      return 1;
    }
    bp_find_close(fs);
    return 0;
  }

  return find_in_basepath(fs, 0);
}

extern "C" int dd_find_next(alefind_t *fs)
{
  static alefind_t f;

  if (bp_find_next(fs))
    return 1;

  int grp = fs->grp;
  if (grp == -2)
  {
    bp_find_close(fs);
    fs->grp = -1;
    return 0;
  }
  if (grp >= DF_MAX_BASE_PATH_NUM)
    return 0;

  // printf("continue with next base path %d\n", fs->grp);

  bp_find_close(fs);
  return find_in_basepath(fs, grp + 1);
}

extern "C" int dd_find_close(alefind_t *fs)
{
  if (fs->grp == -2)
    return bp_find_close(fs);

  if (fs->grp < 0 || fs->grp >= DF_MAX_BASE_PATH_NUM)
    return 0;

  return bp_find_close(fs);
}

#define EXPORT_PULL dll_pull_osapiwrappers_findFile
#include <supp/exportPull.h>

#endif
