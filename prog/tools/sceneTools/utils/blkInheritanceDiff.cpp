#include <stdio.h>
#include <stdlib.h>

#include <debug/dag_logSys.h>

#include "blkInhDiffMaker.h"

#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_mainCon.inc.cpp>

int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);
  printf("Dagor datablock inheritance diff creator\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  const char *parent_fn = NULL, *source_fn = NULL, *result_fn = NULL;

  if (__argc > 2)
  {
    DataBlockInhDiffMaker diffMaker;
    diffMaker.makeDiff(__argv[1], __argv[2], (__argc > 3) ? __argv[3] : NULL);
    return 0;
  }

  printf("\n"
         "Program make child datablock from source with inheritance from parent\n"
         "Command parameters:\n"
         "parent_blk_file source_blk_file result_blk_file(optional)\n");

  return 0;
}
