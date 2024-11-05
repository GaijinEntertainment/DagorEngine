// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include <winGuiWrapper/wgw_file_update.h>
#include <debug/dag_debug.h>

#pragma comment(lib, "user32.lib")


bool FileUpdateCallback::getWriteTime(FileTime &ft)
{
  FILETIME create_tm, access_tm, write_tm;

  void *fh = CreateFile(mFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fh != INVALID_HANDLE_VALUE && GetFileTime(fh, &create_tm, &access_tm, &write_tm))
  {
    ft.dwLowDateTime = write_tm.dwLowDateTime;
    ft.dwHighDateTime = write_tm.dwHighDateTime;
    return true;
  }

  return false;
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
