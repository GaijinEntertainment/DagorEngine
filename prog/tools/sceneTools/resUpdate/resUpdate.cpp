#include <gameResPatcher/resUpdUtil.h>
#include <util/dag_globDef.h>
#include <debug/dag_except.h>
#include <libTools/util/progressInd.h>
#include <util/dag_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static void print_header()
{
  printf("GRP/DxP data updater v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}
static bool ctrl_c_pressed = false;
static void __cdecl ctrl_break_handler(int)
{
  ctrl_c_pressed = true;
  ;
}

int DagorWinMain(bool debugmode)
{
  bool dryRun = false;
  const char *vfs_fn = "grp_hdr.vromfs.bin";
  const char *reslist_fn = "resPacks.blk";

  for (int i = 1; i < __argc; i++)
    if (__argv[i][0] == '-')
    {
      if (stricmp(__argv[i], "-dryRun") == 0)
        dryRun = true;
      else if (stricmp(__argv[i], "-verbose") == 0)
        patch_update_game_resources_verbose = 1;
      else if (strnicmp(__argv[i], "-vfs:", 5) == 0)
        vfs_fn = __argv[i] + 5;
      else if (strnicmp(__argv[i], "-resBlk:", 8) == 0)
        reslist_fn = __argv[i] + 8;
      else
        printf("ERR: unknown option %s, skipping\n", __argv[i]);

      memmove(__argv + i, __argv + i + 1, (__argc - i - 1) * sizeof(__argv[0]));
      __argc--;
      i--;
    }

  if (__argc < 4 || (__argc & 1))
  {
    print_header();
    printf("usage: resUpdate-dev.exe [switches] <game_root> [<new_vrom_file> <vromdest>]...\n\n"
           "switches are:\n"
           "  -dryRun\n"
           "  -verbose\n"
           "  -vfs:<grp_hdr.vromfs.bin>\n"
           "  -resBlk:<resPacks.blk>\n"
           "\n\n");
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  class Pbar : public IGameResPatchProgressBar
  {
    IGenericProgressIndicator *pb;

  public:
    Pbar() : pb(create_con_progressbar()) { pb->setTotal(1000); }
    ~Pbar() { pb->destroy(); }

    virtual void setStage(int stage)
    {
      switch (stage)
      {
        case 0: pb->setActionDesc("Preparing to patch"); break;
        case 1: pb->setActionDesc("Patching gameres"); break;
        case 2: pb->setActionDesc("Patching texpack"); break;
        case 3: pb->setActionDesc("Finishing patch"); break;
      }
    }
    virtual void setPromilleDone(int promille) { pb->setDone(promille); }
    virtual bool isCancelled() { return ctrl_c_pressed; }
  } pbar;

  for (int i = 2; i + 1 < __argc; i += 2)
  {
    DAGOR_TRY
    {
      int ret = patch_update_game_resources(__argv[1], __argv[i], __argv[i + 1], &pbar, dryRun, vfs_fn, reslist_fn);
      if (ret < 0)
      {
        printf("\nERR: failed to update %s/%s with %s\n", __argv[1], __argv[i + 1], __argv[i]);
        return 1;
      }
      printf("updated %s/%s with %s,  %d%% data reusage\n\n", __argv[1], __argv[i + 1], __argv[i], ret);
    }
    DAGOR_CATCH(DagorException e)
    {
      printf("\nERR: failed to update %s/%s with %s\n\nEXCEPTION %s\n%s\n", __argv[1], __argv[i + 1], __argv[i], e.excDesc,
        DAGOR_EXC_STACK_STR(e).str());
      return 1;
    }
  }
  return 0;
}
