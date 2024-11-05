// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/makeBindump.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <gameRes/grpData.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void __cdecl ctrl_break_handler(int) { quit_game(0); }


static void print_header()
{
  printf("Make GRP cache v1.1\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

int DagorWinMain(bool debugmode)
{
  if (__argc < 3)
  {
    print_header();
    printf("usage: mkGrpCache-dev.exe <grp_binary> <dest_cache_fname> [{PC|PS3|X360}]\n");
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  FullFileLoadCB crd(__argv[1]);
  if (!crd.fileHandle)
  {
    print_header();
    printf("ERR: can't open %s", __argv[1]);
    return 13;
  }
  unsigned targetCode = _MAKE4C('PC');
  if (__argc >= 4)
  {
    const char *targetStr = __argv[3];
    targetCode = 0;
    while (*targetStr)
    {
      targetCode = (targetCode >> 8) | (*targetStr << 24);
      targetStr++;
    }
  }
  unlink(__argv[2]);

  bool read_be = dagor_target_code_be(targetCode);

  gamerespackbin::GrpHeader ghdr;
  crd.read(&ghdr, sizeof(ghdr));
  if (ghdr.label != _MAKE4C('GRP1') && ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
  {
    print_header();
    printf("%s: no GRP1/GRP2/GRP3 lable\n", __argv[1]);
    return 13;
  }
  int restSz = mkbindump::le2be32_cond(ghdr.restFileSize, read_be);
  int fullDataSize = mkbindump::le2be32_cond(ghdr.fullDataSize, read_be);
  if (restSz + sizeof(ghdr) != df_length(crd.fileHandle))
  {
    print_header();
    printf("%s: Corrupt file: restFileSize=%d!=%d\n", __argv[1], restSz, df_length(crd.fileHandle) + sizeof(ghdr));
    return 13;
  }

  FullFileSaveCB cwr(__argv[2]);
  if (!cwr.fileHandle)
  {
    print_header();
    printf("ERR: can't open output %s", __argv[2]);
    return 13;
  }
  cwr.write(&ghdr, sizeof(ghdr));
  copy_stream_to_stream(crd, cwr, fullDataSize);

  return 0;
}
