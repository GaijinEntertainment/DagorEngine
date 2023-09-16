#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/strUtil.h>

void showUsage()
{
  printf("\nUsage: blk2robin.exe <input.BLK> [<output.roblk>]\n");
  printf("  when output name is ommitted, it is constructed as input.roblk\n");
}

int DagorWinMain(bool debugmode)
{
  printf("BLK -> Read-only BLK bindump  Conversion Tool v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n");

  // get options
  if (dgs_argc < 2)
  {
    showUsage();
    return 0;
  }

  if (stricmp(dgs_argv[1], "-h") == 0 || stricmp(dgs_argv[1], "-H") == 0 || stricmp(dgs_argv[1], "/?") == 0)
  {
    showUsage();
    return 0;
  }

  String out_fn;
  if (dgs_argc > 2)
    out_fn = dgs_argv[2];
  else
  {
    out_fn = dgs_argv[1];
    remove_trailing_string(out_fn, ".blk");
    out_fn += ".roblk";
  }

  DataBlock blk;
  if (!blk.load(dgs_argv[1]))
  {
    printf("can't read input BLK: %s", dgs_argv[1]);
    return 1;
  }

  {
    FullFileSaveCB fcwr(out_fn);
    if (!fcwr.fileHandle)
    {
      printf("can't create output roBLK: %s", (char *)out_fn);
      return 2;
    }

    mkbindump::BinDumpSaveCB cwr(256 << 10, _MAKE4C('PC'), false);
    mkbindump::write_ro_datablock(cwr, blk);

    cwr.copyDataTo(fcwr);
  }

  if (dgs_argc > 3)
  {
    DataBlock blk;
    FullFileLoadCB fcrd(out_fn);
    RoDataBlock *rb = RoDataBlock::load(fcrd);
    blk = *rb;
    blk.saveToTextFile(dgs_argv[3]);
  }

  return 0;
}
