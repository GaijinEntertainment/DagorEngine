// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_simpleString.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>

#include <debug/dag_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <sys/utime.h>
#include <io.h>
#include <direct.h>
#elif _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID | _TARGET_C3
#include <unistd.h>
#include <utime.h>
#define __stat64    stat
#define _stat64     stat
#define __utimbuf64 utimbuf
#define _utime64    utime
static int mkdir(const char *path) { return mkdir(path, 0777); }
#elif _TARGET_C1 | _TARGET_C2





#endif
#if _TARGET_PC_WIN
#include <windows.h>
#elif _TARGET_PC_MACOSX
#include <libproc.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>


bool dag_get_file_time(const char *fname, int64_t &time_date)
{
  const char *fpath = df_get_real_name(fname);
  if (fpath)
  {
    struct __stat64 buf;
    if (_stat64(fpath, &buf) == 0)
    {
      time_date = buf.st_mtime;
      return true;
    }
  }
  time_date = 0;
  return false;
}

bool dag_set_file_time(const char *fname, int64_t time_date)
{
#if !(_TARGET_C1 | _TARGET_C2)
  const char *fpath = df_get_real_name(fname);
  if (fpath)
  {
    struct __utimbuf64 buf;

    buf.actime = time_date;
    buf.modtime = time_date;
    return _utime64(fpath, &buf) == 0;
  }
#endif
  return false;
}

int dag_get_file_size(const char *fname)
{
  const char *fpath = df_get_real_name(fname);
  if (!fpath)
    return -1;

  struct __stat64 buf;
  if (_stat64(fpath, &buf) == 0)
    return buf.st_size;
  else
    return -1;
}


//==================================================================================================
bool dag_copy_file(const char *src, const char *dest, bool overwrite)
{
#if _TARGET_C1 | _TARGET_C2


#else
  if (!::access(dest, 4) && !overwrite)
    return false;
#endif

  const char *srcPath = ::df_get_real_name(src);
  if (!srcPath)
    return false;

  if (!dest)
    return false;

  if (!::stricmp(srcPath, dest))
    return true;
  if (make_good_path(srcPath) == make_good_path(dest))
    return true;

  FILE *srcF = ::fopen(srcPath, "rb");
  if (!srcF)
    return false;

  FILE *destF = ::fopen(dest, "wb");
  if (!destF)
  {
    ::fclose(srcF);
    return false;
  }

  char buff[64 * 1024];

  while (size_t count = ::fread(buff, 1, 64 * 1024, srcF))
    if (::fwrite(buff, 1, count, destF) != count)
    {
      logerr("copy(%s) to (%s): write %d bytes failed at %d", src, dest, count, ftell(srcF) - count);
      ::fclose(srcF);
      ::fclose(destF);
      ::unlink(dest);
      return false;
    }

  ::fclose(srcF);
  ::fclose(destF);

  return true;
}


inline static bool ignoreFolder(const char *folder, const Tab<String> &ignore)
{
  if (folder[0] == '.')
  {
    if (!folder[1])
      return true;

    if (folder[1] == '.' && !folder[2])
      return true;
  }

  for (int i = 0; i < ignore.size(); ++i)
    if (!::stricmp(ignore[i], folder))
      return true;

  return false;
}


static bool copyFile(const char *src, const char *dest, const Tab<String> &ignore, bool folder)
{
  if (folder)
  {
    if (mkdir(dest))
      return false;

    if (!dag_copy_folder_content(src, dest, ignore, true))
      return false;
  }
  else
  {
    if (!dag_copy_file(src, dest))
      return false;
  }

  return true;
}


//==================================================================================================
bool dag_copy_folder_content(const char *src, const char *dest, const Tab<String> &ignore, bool copy_subfolders)
{
#if _TARGET_PC_WIN
  const char *srcPath = ::is_full_path(src) ? src : ::df_get_real_folder_name(src);
  if (!srcPath || !*srcPath)
    return false;

  const char *destPath = ::is_full_path(dest) ? dest : ::df_get_real_folder_name(dest);
  if (!destPath || !*destPath)
    return false;

  const String mask = ::make_full_path(srcPath, "*.*");

  String srcStr;
  String destStr;
  srcStr.reserve(512);
  destStr.reserve(512);
  bool isFolder;

  __finddata64_t findData;
  intptr_t findResult = _findfirst64(mask, &findData);
  bool ret = true;

  if (findResult != -1)
  {
    if (!ignoreFolder(findData.name, ignore))
    {
      srcStr = ::make_full_path(srcPath, findData.name);
      destStr = ::make_full_path(destPath, findData.name);
      isFolder = findData.attrib & _A_SUBDIR;

      if (copy_subfolders || !isFolder)
      {
        if (!copyFile(srcStr, destStr, ignore, isFolder))
        {
          _findclose(findResult);
          return false;
        }
      }
    }

    while (!_findnext64(findResult, &findData))
    {
      if (!ignoreFolder(findData.name, ignore))
      {
        srcStr = ::make_full_path(srcPath, findData.name);
        destStr = ::make_full_path(destPath, findData.name);
        isFolder = findData.attrib & _A_SUBDIR;

        if (copy_subfolders || !isFolder)
        {
          if (!copyFile(srcStr, destStr, ignore, isFolder))
          {
            _findclose(findResult);
            return false;
          }
        }
      }
    }
  }
  else
    ret = false;

  _findclose(findResult);
  return ret;
#else
  G_UNUSED(src);
  G_UNUSED(dest);
  G_UNUSED(ignore);
  G_UNUSED(copy_subfolders);
  return false;
#endif
}


//==================================================================================================
bool dag_file_compare(const char *path1, const char *path2)
{
  if (!path1 || !*path1 || !path2 || !*path2)
    return false;

  SimpleString realPath1(::df_get_real_name(path1));
  if (realPath1.empty())
    return false;

  SimpleString realPath2(::df_get_real_name(path2));
  if (realPath2.empty())
    return false;

  if (!::stricmp(realPath1, realPath2))
    return true;

  file_ptr_t f1 = ::df_open(realPath1, DF_READ);
  if (!f1)
    return false;

  file_ptr_t f2 = ::df_open(realPath2, DF_READ);
  if (!f2)
  {
    ::df_close(f1);
    return false;
  }

  if (df_length(f1) != df_length(f2))
  {
    ::df_close(f1);
    ::df_close(f2);

    return false;
  }

  char buff1[64 * 1024];
  char buff2[64 * 1024];
  int count1 = 0;
  int count2 = 0;

  bool proceed = true;
  bool equal = true;

  while (proceed)
  {
    count1 = ::df_read(f1, buff1, 64 * 1024);
    count2 = ::df_read(f2, buff2, 64 * 1024);

    if (count1 != count2)
    {
      equal = false;
      proceed = false;
    }

    if (!count1)
    {
      equal = true;
      proceed = false;
    }

    if (memcmp(buff1, buff2, count1) != 0)
    {
      equal = false;
      proceed = false;
    }
  }

  ::df_close(f1);
  ::df_close(f2);

  return equal;
}


bool dag_read_file_to_mem(const char *path, Tab<char> &buffer)
{
  if (!path || !*path)
    return false;

  const char *realPath = ::is_full_path(path) ? path : ::df_get_real_name(path);
  if (!realPath || !*realPath)
    return false;

  file_ptr_t f = ::df_open(realPath, DF_READ);
  if (!f)
    return false;

  const int len = ::df_length(f);

  if (len <= 0)
  {
    ::df_close(f);
    return false;
  }

  buffer.resize(len);
  const int count = ::df_read(f, buffer.data(), len);
  ::df_close(f);

  return count == len;
}


bool dag_read_file_to_mem_str(const char *path, String &buffer)
{
  Tab<char> fileContent(tmpmem);

  if (::dag_read_file_to_mem(path, fileContent))
  {
    buffer.printf(0, "%.*s", fileContent.size(), fileContent.data());
    return true;
  }

  return false;
}


int dag_path_compare(const char *path1, const char *path2)
{
  unsigned char ac;
  unsigned char bc;

  if (!path1 && !path2)
    return 0;

  if (!path1)
    return -1;

  if (!path2)
    return 1;

  if (!*path1 && !*path2)
    return 0;

  if (!*path1)
    return -1;

  if (!*path2)
    return 1;

  const int l1 = i_strlen(path1);
  const int l2 = i_strlen(path2);

  for (int i = 0; i < l1 && i < l2; ++i)
  {
    ac = path1[i];
    bc = path2[i];

    if ((ac == '\\' || ac == '/') && (bc == '\\' || bc == '/'))
      continue;

    if (isupper(ac))
      ac = _tolower(ac);

    if (isupper(bc))
      bc = _tolower(bc);

    if (ac > bc)
      return 1;
    else if (ac < bc)
      return -1;
  }

  if (l1 > l2)
  {
    if (l1 - l2 == 1)
    {
      ac = path1[l1 - 1];
      if (ac == '\\' || ac == '/')
        return 0;
    }

    return 1;
  }
  else if (l1 < l2)
  {
    if (l2 - l1 == 1)
    {
      bc = path2[l2 - 1];
      if (bc == '\\' || bc == '/')
        return 0;
    }

    return -1;
  }

  return 0;
}


#if _TARGET_PC
char *dag_get_appmodule_dir(char *dirbuf, size_t dirbufsz)
{
#if _TARGET_PC_WIN
  GetModuleFileNameA(NULL, dirbuf, dirbufsz);
#elif _TARGET_PC_MACOSX
  char buf[PROC_PIDPATHINFO_MAXSIZE];
  if (proc_pidpath(getpid(), buf, sizeof(buf)) < 0)
    dirbuf[0] = '\0';
  else
    snprintf(dirbuf, dirbufsz, "%s", buf);
#else
  if (readlink("/proc/self/exe", dirbuf, dirbufsz) < 0)
    dirbuf[0] = '\0';
#endif
  dd_simplify_fname_c(dirbuf);
  if (char *p = strrchr(dirbuf, '/'))
    *p = '\0';
  return dirbuf;
}
#endif
