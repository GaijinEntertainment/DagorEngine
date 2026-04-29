// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameResPatcher/resUpdUtil.h>
#include <util/dag_globDef.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_except.h>
#include <libTools/util/progressInd.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>
#include <util/dag_finally.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <memory/dag_framemem.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/optional.h>

static void print_header()
{
  printf("Sound bank updater v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2026\n\n");
}
static bool ctrl_c_pressed = false;
static void __cdecl ctrl_break_handler(int) { ctrl_c_pressed = true; }

int DagorWinMain(bool debugmode)
{
  if (dgs_argc < 4)
  {
    print_header();
    printf("usage(mkidx): soundUpd-dev.exe mkidx <idx> <sound1.assets.bank> <sound2.assets.bank> ...\n"
           "usage(patch): soundUpd-dev.exe patch <idx> <sound_dir_to_patch>\n"
           "\n\n");
    return -1;
  }

  if (stricmp(dgs_argv[1], "mkidx") == 0)
  {
    FullFileSaveCB indexCwr;
    if (!indexCwr.open(dgs_argv[2], DF_WRITE | DF_CREATE))
    {
      print_header();
      printf("ERR: failed to open index file %s for writing\n", dgs_argv[2]);
      return -1;
    }

    for (int i = 3; i < dgs_argc; i++)
    {
      if (write_index_for_sound_bank_to_stream(dgs_argv[i], indexCwr))
        printf("Bank %s has been written to index file\n", dgs_argv[i]);
      else
        printf("ERR: failed to write index file for bank %s. skipping...\n", dgs_argv[i]);
    }

    printf("Index file %s created successfully\n", dgs_argv[2]);
  }
  else if (stricmp(dgs_argv[1], "patch") == 0)
    patch_update_sound_bank(dgs_argv[2], dgs_argv[3]);
  else
  {
    print_header();
    printf("ERR: unknown mode %s\n", dgs_argv[1]);
    return -1;
  }

  return 0;
}
