// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <osApiWrappers/dag_direct.h>
#include <util/dag_simpleString.h>

#include "checker.h"


static void showUsage() { printf("\nUsage:\n  findDup-dev <start_dir> [-all]\n\n"); }


int DagorWinMain(bool debugmode)
{
  printf("Dagor File Duplicate Finder\n"
         "Copyright (c) Gaijin Games KFT, 2023\n\n");
  // get options
  if (__argc < 2)
  {
    showUsage();
    return 1;
  }
  bool all_files = __argc > 2 && stricmp(__argv[2], "-all") == 0;
  const char *dir_path = __argv[1];
  if (!::dd_dir_exist(dir_path))
  {
    printf("\n[FATAL ERROR] '%s' is not a valid dir path\n", dir_path);
    showUsage();
    return 1;
  }

  SimpleString dir(dir_path);
  ::dd_simplify_fname_c(dir);

  FileChecker checker(all_files);
  checker.gatherFileNames(dir);
  checker.checkForDuplicates();

  printf("\n%d file[s] checked, %d duplicate[s] found.\n", checker.files.size(), checker.fileDups.size());
  printf("\nduplicates:\n");

  for (int i = 0; i < checker.fileDups.size(); ++i)
  {
    const char *file1 = checker.files[checker.fileDups[i].f1]->filePath;
    const char *file2 = checker.files[checker.fileDups[i].f2]->filePath;
    printf("%d.\n-> %s\n-> %s\n", i + 1, file1, file2);
  }

  return 0;
}
