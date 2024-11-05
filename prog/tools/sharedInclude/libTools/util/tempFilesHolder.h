//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_direct.h>


class TempFilesHolder
{
public:
  TempFilesHolder() : fnames(tmpmem) {}
  TempFilesHolder(const Tab<String> &files) : fnames(files) {}
  ~TempFilesHolder() { eraseFiles(); }

  void push_back(const char *fname)
  {
    if (find(fname) == -1)
      fnames.push_back(fname);
  }

  void push_back(const Tab<String> &files)
  {
    for (int i = 0; i < files.size(); ++i)
      if (find(files[i]) == -1)
        fnames.push_back(files[i]);
  }

  void removeFromList(const char *fname)
  {
    const int idx = find(fname);
    if (idx > -1)
      fnames.erase(idx, 1);
  }

  void clearFilesList(bool free_mem = false) { !free_mem ? fnames.clear() : clear_and_shrink(fnames); }

  void eraseFiles()
  {
    for (int i = 0; i < fnames.size(); ++i)
      ::dd_erase(fnames[i]);
  }

private:
  Tab<String> fnames;

  int find(const char *fname) const
  {
    for (int i = 0; i < fnames.size(); ++i)
      if (!stricmp(fname, fnames[i]))
        return i;

    return -1;
  }
};
