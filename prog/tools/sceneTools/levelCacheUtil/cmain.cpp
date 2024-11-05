// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>
#include "scnCacheUtil.h"
#include <libTools/util/progressInd.h>
#include <signal.h>
#include <stdlib.h>

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_header()
{
  printf("DBLD cache update/check utility v1.1\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

class ConsoleProgressBar : public IGenProgressInd
{
public:
  IGenericProgressIndicator *pbar;

  ConsoleProgressBar() { pbar = create_con_progressbar(true); }
  ~ConsoleProgressBar()
  {
    pbar->destroy();
    pbar = NULL;
  }

  virtual void setActionDescFmt(const char *desc_fmt, const DagorSafeArg *arg, int anum)
  {
    String desc;
    desc.vprintf(128, desc_fmt, arg, anum);
    pbar->setActionDesc(desc);
    ;
  }
  virtual void setTotal(int total_cnt) { pbar->setTotal(total_cnt); }
  virtual void setDone(int done_cnt) { pbar->setDone(done_cnt); }
  virtual void incDone(int inc = 1) { pbar->incDone(inc); }
  virtual void destroy() { delete this; }
};

int DagorWinMain(bool debugmode)
{
  if (__argc < 2)
  {
    print_header();
    printf("usage: levelCacheUtil-dev.exe <ROOT_DIR> [-cache:<DIR>] [-levels:<DIR>]... [-clean:<on:off>]] [-validate] [-q|-v]\n\n"
           "defaults options are are:\n"
           "  -cache:levels.cache -levels:levels -levels:content -clean:on -v\n"
           "examples:\n"
           "  levelCacheUtil-dev.exe .\n"
           "  levelCacheUtil-dev.exe . -clean:off -validate\n"
           "  levelCacheUtil-dev.exe . -levels:levels -levels:DLC\n"
           "\n");
    return -1;
  }

  signal(SIGINT, ctrl_break_handler);

  const char *root_dir = __argv[1];
  String cache_dir;
  FastNameMapEx level_dirs;
  bool clean_unused = true;
  bool validate_cache_contents = false;

  dgs_execute_quiet = true;
  for (int i = 2; i < __argc; i++)
    if (stricmp(__argv[i], "-noeh") == 0)
      ; // skip
    else if (stricmp(__argv[i], "-quiet") == 0 || stricmp(__argv[i], "-q") == 0)
      dgs_execute_quiet = true;
    else if (stricmp(__argv[i], "-verbose") == 0 || stricmp(__argv[i], "-v") == 0)
      dgs_execute_quiet = false;
    else if (strnicmp(__argv[i], "-cache:", 7) == 0)
      cache_dir.printf(260, "%s/%s", root_dir, __argv[i] + 7);
    else if (strnicmp(__argv[i], "-levels:", 8) == 0)
      level_dirs.addNameId(String(260, "%s/%s", root_dir, __argv[i] + 8));
    else if (stricmp(__argv[i], "-clean:on") == 0)
      clean_unused = true;
    else if (stricmp(__argv[i], "-clean:off") == 0)
      clean_unused = false;
    else if (stricmp(__argv[i], "-validate") == 0)
      validate_cache_contents = true;
    else
    {
      print_header();
      printf("ERR: unknown parameter %s\n", __argv[i]);
      return -2;
    }

  if (cache_dir.empty())
    cache_dir.printf(260, "%s/%s", root_dir, "levels.cache");
  if (!level_dirs.nameCount())
  {
    level_dirs.addNameId(String(260, "%s/%s", root_dir, "levels"));
    level_dirs.addNameId(String(260, "%s/%s", root_dir, "content"));
  }

  print_header();
  printf("Cache: %s\n", cache_dir.str());
  printf("Clean unused caches:     %s\n", clean_unused ? "ON" : "OFF");
  printf("Validate cache contents: %s\n", validate_cache_contents ? "ON" : "OFF");
  printf("Level locations:\n");
  for (int i = 0; i < level_dirs.nameCount(); i++)
    printf("  %s\n", level_dirs.getName(i));
  printf("\n");

  ConsoleProgressBar pbar;
  if (!update_level_caches(cache_dir, level_dirs, clean_unused, &pbar, dgs_execute_quiet))
  {
    printf("ERR: update_level_caches failed\n");
    return -3;
  }
  if (validate_cache_contents)
    if (!verify_level_caches_files(cache_dir, &pbar, dgs_execute_quiet))
    {
      printf("\nsome cache files were inconsistent and been removed\n\n");
      if (!update_level_caches(cache_dir, level_dirs, clean_unused, &pbar, dgs_execute_quiet))
      {
        printf("ERR: update_level_caches failed\n");
        return -3;
      }
    }

  return 0;
}
