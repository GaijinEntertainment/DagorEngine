// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <winGuiWrapper/wgw_file_update.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN
#include <windows.h>

#pragma comment(lib, "user32.lib")
#else
#include <sys/stat.h>
#endif


bool FileUpdateCallback::getWriteTime(FileTime &ft)
{
#if _TARGET_PC_WIN
  FILETIME create_tm, access_tm, write_tm;

  void *fh = CreateFile(mFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fh == INVALID_HANDLE_VALUE)
    return false;

  if (GetFileTime(fh, &create_tm, &access_tm, &write_tm))
  {
    ft.dwLowDateTime = write_tm.dwLowDateTime;
    ft.dwHighDateTime = write_tm.dwHighDateTime;
    CloseHandle(fh);
    return true;
  }

  CloseHandle(fh);
  return false;
#else
  struct stat statBuffer;
  if (stat(mFileName, &statBuffer) != 0)
    return false;

  // The value is only used for comparison so no conversion is needed here.
  G_STATIC_ASSERT(sizeof(ft) == sizeof(statBuffer.st_mtim));
  ft = *reinterpret_cast<FileTime *>(&statBuffer.st_mtim);
  return true;
#endif
}


void FileUpdateCallback::setFile(const char *fn)
{
  if (!fn || !*fn || stricmp(mFileName, fn) != 0)
  {
    if (!mFileName.empty() && mTimer.isActive())
      mTimer.stop();
    mFileName = fn;

    if (!mFileName.empty())
    {
      FileTime ft;
      if (getWriteTime(ft))
        mFileTime = ft;
      mTimer.start();
      return;
    }

    mFileName = "";
  }
}


void FileUpdateCallback::update()
{
  FileTime ft;
  if (getWriteTime(ft) && (ft.dwLowDateTime != mFileTime.dwLowDateTime || ft.dwHighDateTime != mFileTime.dwHighDateTime))
  {
    mFileTime = ft;
    UpdateFile();
  }
}
