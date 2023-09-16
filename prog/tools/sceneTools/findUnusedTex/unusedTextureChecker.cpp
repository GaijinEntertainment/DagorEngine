#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <osApiWrappers/dag_direct.h>
#include <debug/dag_logSys.h>

#include "tex_checker.h"


static void showUsage(bool verbose)
{
  printf("\nUsage:\n  findUnusedTex-dev <levels-list.blk> [direct-tex-list.blk]\n");
  if (!verbose)
    return;

  printf("\n"
         "levels-list.blk:\n"
         "  app_dir:t=\".\"\n"
         "  printAssets:b=yes\n"
         "  printFiles:b=no\n"
         "  dumpWorkLists:b=no\n"
         "  level_bins {\n"
         "    path:t=\"game/levels/poly_chud/poly_chud.bin\"\n"
         "  }\n\n"
         "direct-tex-list.blk:\n"
         "  textures {\n"
         "    tex:t=\"tex-asset1\"\n"
         "    tex:t=\"tex-asset2\"\n"
         "    tex:t=\"tex-asset3\"\n"
         "  }\n");
}

static int checkBlkFile(const char *filename)
{
  const char *ext = ::dd_get_fname_ext(filename);
  if (!ext || strcmp(ext, ".blk") != 0)
  {
    printf("\n[FATAL ERROR] '%s' is not BLK\n", filename);
    showUsage(false);
    return 0;
  }

  return 1;
}


int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);
  printf("Dagor Unused Texture Checker\n"
         "Copyright (C) Gaijin Games KFT, 2023\n");

  if (__argc < 2)
  {
    showUsage(true);
    return 1;
  }

  const char *filename1 = __argv[1];
  const char *filename2 = __argc > 2 ? __argv[2] : NULL;

  if (!checkBlkFile(filename1))
    return 1;
  if (filename2 && !checkBlkFile(filename2))
    return 1;

  TexChecker checker(filename1, filename2);

  Tab<SimpleString> out_assets(tmpmem), out_fnames(tmpmem);
  checker.CheckTextures(out_assets, out_fnames);

  printf("\nUnused textures list(%d):\n", out_assets.size());

  DataBlock blk(filename1);
  if (blk.getBool("printAssets", true) && blk.getBool("printFiles", false))
    for (int i = 0; i < out_assets.size(); ++i)
      printf("%-48s %s\n", out_assets[i].str(), out_fnames[i].str());
  else if (blk.getBool("printAssets", true) && !blk.getBool("printFiles", false))
    for (int i = 0; i < out_assets.size(); ++i)
      printf("%s\n", out_assets[i].str());
  else if (!blk.getBool("printAssets", true) && blk.getBool("printFiles", false))
    for (int i = 0; i < out_assets.size(); ++i)
      printf("%s\n", out_fnames[i].str());

  return 0;
}
