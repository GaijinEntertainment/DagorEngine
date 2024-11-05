// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/basePath.h>
#include "romFileReader.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "fs_hlp.h"
#include <debug/dag_debug.h>
#include <fcntl.h>
#include <math/random/dag_random.h>
#include <errno.h>
#include <osApiWrappers/dag_critSec.h>

#define CHECK_FILE_CASE 0

#define SIMULATE_READ_ERRORS 0
#if SIMULATE_READ_ERRORS
#include <math/random/dag_random.h>
#endif

#if DAGOR_DBGLEVEL > 0
#define VROMFS_FIRST_PRIORITY vromfs_first_priority
#else
#define VROMFS_FIRST_PRIORITY (1)
#endif

#if _TARGET_PC_WIN
#define STAT  _stat64
#define FSTAT _fstat64
#else
#define STAT  stat
#define FSTAT fstat
#endif

#if !defined(_TARGET_PC_WIN) && !defined(_TARGET_XBOX)
#define _fileno(f)     fileno(f)
#define _filelength(f) filelength(f)
#endif // !_TARGET_PC_WIN && !_TARGET_XBOX

static inline int sys_fstat(void *fp, struct STAT &st)
{
#if _TARGET_C1

#else
  int fd = _fileno((FILE *)fp);
#endif
  return FSTAT(fd, &st);
}

RomFileReader RomFileReader::rdPool[MAX_FILE_RD];
WinCritSec RomFileReader::critSec;

#if CHECK_FILE_CASE && _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
#include <windows.h>
static void check_file_case(FILE *fp, const char *fn)
{
  char realname[DAGOR_MAX_PATH + 1];
  HANDLE h = (HANDLE)_get_osfhandle(_fileno(fp));
  if (h != INVALID_HANDLE_VALUE)
    if (GetFinalPathNameByHandleA(h, realname, DAGOR_MAX_PATH, 0))
    {
      dd_simplify_fname_c(realname);

      if (!strstr(realname, fn))
        DAG_FATAL("trying to open %s as %s", realname, fn);
    }
}
#else
static void check_file_case(FILE * /*fp*/, const char * /*fn*/) {}
#endif

#if _TARGET_C3

#endif

file_ptr_t df_open(const char *filename, int flags)
{
  if (!filename || !*filename)
    return NULL;

#if _TARGET_C3

#endif
  if ((flags & (DF_READ | DF_WRITE | DF_CREATE)) == DF_READ && strncmp(filename, "b64://", 6) == 0)
    return to_file_ptr_t(RomFileReader::openB64(filename + 6));
  if ((flags & (DF_READ | DF_WRITE | DF_CREATE)) == DF_READ)
    if (const char *asset_fn = get_rom_asset_fpath(filename))
      return to_file_ptr_t(RomFileReader::openAsset(asset_fn));

  bool vromFirstPriority = VROMFS_FIRST_PRIORITY;
  if ((flags & (DF_READ | DF_WRITE | DF_CREATE | DF_REALFILE_ONLY)) == DF_READ && (vromFirstPriority || (flags & DF_VROM_ONLY)))
  {
    VirtualRomFsData *vrom = NULL;
    VromReadHandle data = vromfs_get_file_data(filename, &vrom);
    if (data.data())
    {
      vromFirstPriority = vrom->firstPriority;
      if (vromFirstPriority || (flags & DF_VROM_ONLY))
      {
        VirtualRomFsPack *pack = static_cast<VirtualRomFsPack *>(vrom);
        return to_file_ptr_t(RomFileReader::open(data, pack->isValid() ? pack : NULL, vrom));
      }
    }

    if (flags & DF_VROM_ONLY)
    {
      if (dag_on_file_not_found && !(DF_IGNORE_MISSING & flags))
        dag_on_file_not_found(filename);
      return NULL;
    }
  }

  if (dag_on_file_pre_open)
    if (!dag_on_file_pre_open(filename))
    {
      if (dag_on_file_not_found && !(DF_IGNORE_MISSING & flags))
        dag_on_file_not_found(filename);
      return NULL;
    }

  char accmode[8], *p = accmode;

  // convert flags to access string
  // following access modes are defined:
  // r, w, a, r+, w+, a+
  if (flags & DF_APPEND)
  {
    *p = 'a';
    p++;
    if (flags & DF_READ)
    {
      *p = '+';
      p++;
    }
  }
  else
  {
    switch (flags & DF_RDWR)
    {
      case DF_READ:
        *p = 'r';
        p++;
        break;
      case DF_WRITE:
        // it should possibly be "r+" without DF_CREATE but this could break existing apps
        *p = 'w';
        p++;
        break;
      case DF_RDWR:
        *p = (flags & DF_CREATE) ? 'w' : 'r';
        p++;
        *p = '+';
        p++;
        break;

      default:
      {
        logerr("cannot open '%s', expected DF_READ or DF_WRITE or DF_RDWR or DF_APPEND "
               "in flags",
          filename);
        return NULL;
      }
    }
  }

  *p = 'b';
  p++;
  *p = '\0';
  accmode[sizeof(accmode) - 1] = 0; // for /analize

  // check for absolute path
  if (is_path_abs(filename))
  {
    char fn[DAGOR_MAX_PATH];
    strncpy(fn, filename, sizeof(fn) - 1);
    fn[sizeof(fn) - 1] = 0;
    dd_simplify_fname_c(fn);

#if _TARGET_C3










#endif
    FILE *fp;
    {
      fp = fopen(fn, accmode);
      if (fp)
      {
        check_file_case(fp, fn);
        if (dag_on_file_open)
          dag_on_file_open(fn, fp, flags);
#if _TARGET_C3 // we reduce WriteFile() calls with file buffering


#endif
        return (file_ptr_t)fp;
      }
    }
  }
  else // try opening with all base paths
  {
    file_ptr_t file_ptr = nullptr;
    iterate_base_paths_simplify(filename, [&file_ptr, accmode, flags](const char *fn_ptr) {
#if _TARGET_C3








#endif
      if (FILE *fp = fopen(fn_ptr, accmode))
      {
#ifdef S_ISDIR
        struct STAT st;
        if (sys_fstat(fp, st) == 0 && S_ISDIR(st.st_mode))
        {
          fclose(fp);
          return false;
        }
#endif
        check_file_case(fp, fn_ptr);
        if (dag_on_file_open)
          dag_on_file_open(fn_ptr, fp, flags);
#if _TARGET_C3 // we reduce WriteFile() calls with file buffering


#endif

        file_ptr = (file_ptr_t)fp;
        return true;
      }
      return false;
    });
    if (file_ptr)
      return file_ptr;
  }

  if ((flags & (DF_READ | DF_WRITE | DF_CREATE | DF_REALFILE_ONLY)) == DF_READ && !vromFirstPriority)
  {
    VirtualRomFsData *vrom = NULL;
    VromReadHandle data = vromfs_get_file_data(filename, &vrom);
    if (data.data())
    {
      VirtualRomFsPack *pack = static_cast<VirtualRomFsPack *>(vrom);
      return to_file_ptr_t(RomFileReader::open(data, pack->isValid() ? pack : NULL, vrom));
    }
  }

  if (dag_on_file_not_found && !(DF_IGNORE_MISSING & flags))
    dag_on_file_not_found(filename);
  return NULL;
}

int df_close(file_ptr_t fp)
{
  if (!fp)
    return -1;

#if _TARGET_C3

#endif

  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
  {
    RomFileReader::close(rr);
    return 0;
  }

  void *_fp = fp;
  int result = fclose((FILE *)fp);
  if (result != 0)
  {
    logerr("fclose(%p) returns error %d. Probably invalid pointer, IO error (disk full?) or file is already closed", _fp, errno);
    G_UNUSED(_fp);
    return result;
  }
  if (dag_on_file_close)
    dag_on_file_close(fp);
  return 0;
}

void df_flush(file_ptr_t fp)
{
  if (!fp)
    return;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return;

  fflush((FILE *)fp);
}

int df_read(file_ptr_t fp, void *ptr, int len)
{
  if (!fp)
    return -1;

  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->read(ptr, len);

  int ofs = 0;

retry_read:

  int ret;
  bool eof = false;
  ret = (int)fread(ptr, 1, len, (FILE *)fp);
  eof = ret == len ? false : feof((FILE *)fp);

  if (ret != len && ((!eof && dag_on_read_error_cb) || (eof && dag_on_read_beyond_eof_cb)))
    ofs = ftell((FILE *)fp) - (ret > 0 ? ret : 0);
#if SIMULATE_READ_ERRORS
  if (grnd() < 2048)
  {
    ofs = ftell((FILE *)fp) - (ret > 0 ? ret : 0);
    ret = len / 2;
  }
#endif

  if (ret != len && ((!eof && dag_on_read_error_cb && dag_on_read_error_cb(fp, ofs, len)) ||
                      (eof && dag_on_read_beyond_eof_cb && dag_on_read_beyond_eof_cb(fp, ofs, len, ret))))
  {
    clearerr((FILE *)fp);
    fseek((FILE *)fp, ofs, SEEK_SET);
    goto retry_read;
  }

  return ret;
}

int df_write(file_ptr_t fp, const void *ptr, int len)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return 0;
  return (int)fwrite(ptr, 1, len, (FILE *)fp);
}


int df_vprintf(file_ptr_t fp, const char *fmt, va_list lst)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return 0;
  return vfprintf((FILE *)fp, fmt, lst);
}
int df_cprintf(file_ptr_t fp, const char *fmt, ...)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return 0;

  va_list ap;
  va_start(ap, fmt);
  int ret = df_vprintf(fp, fmt, ap);
  va_end(ap);
  return ret;
}
int df_printf(file_ptr_t fp, const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return 0;

  return DagorSafeArg::fprint_fmt(fp, fmt, arg, anum);
}

char *df_gets(char *buf, int n, file_ptr_t fp)
{
  if (!fp)
    return NULL;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->gets(buf, n);
  return fgets(buf, n, (FILE *)fp);
}

int df_seek_to(file_ptr_t fp, int offset_abs)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->seekTo(offset_abs);
  return fseek((FILE *)fp, offset_abs, SEEK_SET);
}
int df_seek_rel(file_ptr_t fp, int offset_rel)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->seekRel(offset_rel);
  return fseek((FILE *)fp, offset_rel, SEEK_CUR);
}
int df_seek_end(file_ptr_t fp, int offset_from_end)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->seekEnd(offset_from_end);
  return fseek((FILE *)fp, offset_from_end, SEEK_END);
}

int df_tell(file_ptr_t fp)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->tell();
  return ftell((FILE *)fp);
}

int df_length(file_ptr_t fp)
{
  if (!fp)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
    return rr->getLength();
#if defined(__GNUC__)
  int old_pos = ftell((FILE *)fp);
  fseek((FILE *)fp, 0, SEEK_END);
  int sz = ftell((FILE *)fp);
  fseek((FILE *)fp, old_pos, SEEK_SET);
  return sz;
#else
  return ::_filelength(_fileno((FILE *)fp));
#endif
}

const VirtualRomFsData *df_get_vromfs_for_file_ptr(file_ptr_t fp)
{
  if (RomFileReader *rr = to_rom_file_reader(fp))
    return rr->getSrcVrom();
  return nullptr;
}
const char *df_get_vromfs_file_data_for_file_ptr(file_ptr_t fp, int &out_data_sz)
{
  if (RomFileReader *rr = to_rom_file_reader(fp))
    return rr->getFileData(out_data_sz);
  return nullptr;
}

static bool vromfs_stat_internal(const char *path, DagorStat *buf)
{
  VirtualRomFsData *vrom = NULL;
  VromReadHandle data = vromfs_get_file_data(path, &vrom);
  if (data.data())
  {
    buf->atime = buf->ctime = -1;
    buf->mtime = vrom->mtime;
    buf->size = data.size();
  }
  return data.data() != NULL;
}

static int sys_stat2dagor_stat(const struct STAT *st, DagorStat *dst)
{
  dst->size = st->st_size;
  dst->atime = st->st_atime;
  dst->mtime = st->st_mtime;
  dst->ctime = st->st_ctime;
  return 0;
}

int df_stat(const char *path, DagorStat *buf)
{
  if (!path || !buf)
    return -1;

  if (strncmp(path, "b64://", 6) == 0)
  {
    const char *data_b64 = path + 6;
    const char *data_b64_end = strchr(data_b64, '.');
    if (!data_b64_end)
      data_b64_end = data_b64 + strlen(data_b64);

    buf->atime = buf->ctime = buf->mtime = -1;
    buf->size = (data_b64_end - data_b64) / 4 * 3;

    if (data_b64_end - 1 > data_b64 && *(data_b64_end - 1) == '=')
    {
      buf->size--;
      if (data_b64_end - 2 > data_b64 && *(data_b64_end - 2) == '=')
        buf->size--;
    }
    return 0;
  }
  if (const char *asset_fn = get_rom_asset_fpath(path))
  {
    buf->atime = buf->ctime = buf->mtime = -1;
    buf->size = RomFileReader::getAssetSize(asset_fn);
    return buf->size < 0 ? ENOENT : 0;
  }

  if (VROMFS_FIRST_PRIORITY && vromfs_stat_internal(path, buf))
    return 0;

  struct STAT st;
  char fn[DAGOR_MAX_PATH];

  if (is_path_abs(path))
  {
    strncpy(fn, path, sizeof(fn) - 1);
    fn[sizeof(fn) - 1] = '\0';
    dd_simplify_fname_c(fn);

    if (STAT(fn, &st) == 0)
      return sys_stat2dagor_stat(&st, buf);
#if _TARGET_C3



#endif
  }
  else if (iterate_base_paths_simplify(path, [&st](const char *fn) { return STAT(fn, &st) == 0; }))
    return sys_stat2dagor_stat(&st, buf);

  if (!VROMFS_FIRST_PRIORITY && vromfs_stat_internal(path, buf))
    return 0;

  return -1;
}

int df_fstat(file_ptr_t fp, DagorStat *buf)
{
  if (!fp || !buf)
    return -1;
  RomFileReader *rr = to_rom_file_reader(fp);
  if (rr)
  {
    buf->atime = buf->mtime = buf->ctime = -1;
    buf->size = rr->getLength();
    return 0;
  }
  struct STAT st;
  return sys_fstat(fp, st) == 0 ? sys_stat2dagor_stat(&st, buf) : -1;
}

#if _TARGET_C1 | _TARGET_C2 | _TARGET_C3

#else
#define TMP_FILES_SUPPORTED 1
#endif

file_ptr_t df_mkstemp(char *templ)
{
#if TMP_FILES_SUPPORTED
#if _TARGET_PC_WIN | _TARGET_XBOX
  int fd = _mktemp(templ) ? open(templ, O_RDWR | O_CREAT | O_BINARY, 0600) : -1;
#else
  int fd = mkstemp(templ);
#endif
  file_ptr_t fp = fd >= 0 ? fdopen(fd, "w+b") : NULL;
#else
  static const char rndchars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  char *beg = templ + strlen(templ) - 1;
  for (; *beg == 'X' && beg >= templ; --beg) // lookup start of Xs regions
    ;
  beg++;
  int fd = -1;
  for (int i = 0; i < 16; ++i)
  {
    for (char *c = beg; *c; ++c)
      *c = rndchars[grnd() % sizeof(rndchars)];
    fd = open(templ, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0 || errno != EEXIST)
      break;
  }
  file_ptr_t fp = NULL;
  if (fd >= 0) // both linux & freebsd have "x" mode extension which is alternative to O_EXCL, but is not clear whether it's works on
               // ps4 ps5 or not (at least it's not documented)
  {
    close(fd);
    fp = fopen(templ, "w+b"); // fdopen() isn't compatible with handles opened with fopen() on PS4
  }
#endif
  if (fp && dag_on_file_open)
    dag_on_file_open(templ, fp, DF_WRITE);
  return fp;
}
