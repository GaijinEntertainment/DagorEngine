//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <time.h>
#include <sys/stat.h>


class FileTimeChecker
{
public:
  time_t lastTime;

  FileTimeChecker(const char *fn = NULL) { setFileName(fn); }

  inline const char *getFileName() { return filename; }

  void setFileName(const char *fn)
  {
    filename = fn;
    updateFileTime();
  }

  void updateFileTime() { lastTime = getFileTime(); }

  time_t getFileTime()
  {
#if !_TARGET_PC_WIN
    return 0;
#else
    struct _stat st;
    if (_stat(filename, &st) != 0)
      return 0;
    return st.st_mtime;
#endif
  }

  bool fileChanged() { return getFileTime() != lastTime; }

protected:
  SimpleString filename;
};


class FileTimeCheckerArray : public Tab<FileTimeChecker>
{
public:
  FileTimeCheckerArray(const allocator_type &allocator = tmpmem_ptr()) : Tab<FileTimeChecker>(allocator) {}

  void updateFileTimes()
  {
    for (int i = 0; i < size(); ++i)
      operator[](i).updateFileTime();
  }

  bool filesChanged()
  {
    for (int i = 0; i < size(); ++i)
      if (operator[](i).fileChanged())
        return true;

    return false;
  }

  void addFile(const char *fn)
  {
    G_ASSERT(fn);

    for (int j = 0; j < size(); j++)
      if (!stricmp(operator[](j).getFileName(), fn))
        return;

    int i = append_items(*this, 1);
    operator[](i).setFileName(fn);
  }
};
