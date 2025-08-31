//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

  void setFile(const char *fn);
  void update() override;

protected:
  virtual void UpdateFile() = 0;

private:
  bool getWriteTime(FileTime &ft);

  WinTimer mTimer;
  SimpleString mFileName;
  FileTime mFileTime;
};
