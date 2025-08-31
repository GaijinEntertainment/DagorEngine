// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fileutil.h"
#include <string.h>
#include <stdio.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>

int find_files_recursive(const char *dir_path, Tab<alefind_t> &out_list, char tmpPath[DAGOR_MAX_PATH])
{
  if (strlen(dir_path) + 2 >= DAGOR_MAX_PATH)
    return 0;

  Tab<SimpleString> foldersFound(framemem_ptr());

  SNPRINTF(tmpPath, DAGOR_MAX_PATH, "%s/*", dir_path);

  for (const alefind_t &ff : dd_find_iterator(tmpPath, DA_SUBDIR))
  {
    if ((*dir_path ? strlen(dir_path) : 1) + 1 + strlen(ff.name) >= DAGOR_MAX_PATH)
      continue;
    if (ff.attr & DA_SUBDIR)
    {
      SNPRINTF(tmpPath, DAGOR_MAX_PATH, "%s/%s", *dir_path ? dir_path : ".", ff.name);
      foldersFound.push_back() = tmpPath;
    }
    else
      out_list.push_back(ff);
  }

  for (int i = 0; i < foldersFound.size(); i++)
    find_files_recursive(foldersFound[i].str(), out_list, tmpPath);

  return out_list.size();
}
