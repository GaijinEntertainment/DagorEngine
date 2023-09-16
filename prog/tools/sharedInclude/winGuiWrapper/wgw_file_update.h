//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_simpleString.h>
#include <winGuiWrapper/wgw_timer.h>

struct FileTime
{
  unsigned long dwLowDateTime;
  unsigned long dwHighDateTime;
};

class FileUpdateCallback : public ITimerCallBack
{
public:
  FileUpdateCallback() : mTimer(this, 1000, false) {}

  virtual void setFile(const char *fn);
  virtual void update();

protected:
  virtual void UpdateFile() = 0;

private:
  bool getWriteTime(FileTime &ft);

  WinTimer mTimer;
  SimpleString mFileName;
  FileTime mFileTime;
};
