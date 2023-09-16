#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#define __DEBUG_FILEPATH          "*"
#include <startup/dag_mainCon.inc.cpp>
#include <signal.h>

extern int64_t make_game_resources_diff(const char *base_root_dir, const char *new_root_dir, const char *patch_dest_dir, bool dryRun,
  const char *vrom_name, const char *res_blk_name, const char *rel_mount_dir, int &out_diff_files);
extern int game_resources_diff_verbose;
extern bool game_resources_diff_strict_tex;

static void print_header()
{
  printf("GRP/DxP data diff v1.2\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}
static bool ctrl_c_pressed = false;
static void __cdecl ctrl_break_handler(int) { ctrl_c_pressed = true; }

int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);

  bool dryRun = false;
  const char *vfs_fn = "grp_hdr.vromfs.bin";
  const char *reslist_fn = "resPacks.blk";

  for (int i = 1; i < __argc; i++)
    if (__argv[i][0] == '-')
    {
      if (stricmp(__argv[i], "-dryRun") == 0)
        dryRun = true;
      else if (stricmp(__argv[i], "-verbose") == 0)
        game_resources_diff_verbose = 1;
      else if (stricmp(__argv[i], "-strictTex") == 0)
        game_resources_diff_strict_tex = 1;
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

  if (__argc < 5)
  {
    print_header();
    printf("usage: resDiffUtil-dev.exe [switches] <base_game_root> <new_game_root> <mount_dir> <patch_dest_dir>\n\n"
           "switches are:\n"
           "  -dryRun\n"
           "  -verbose\n"
           "  -strictTex\n"
           "  -vfs:<grp_hdr.vromfs.bin>\n"
           "  -resBlk:<resPacks.blk>\n"
           "\n\n");
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  int diff_files = 0;
  int64_t ret = make_game_resources_diff(__argv[1], __argv[2], __argv[4], dryRun, vfs_fn, reslist_fn, __argv[3], diff_files);
  if (ret < 0)
  {
    printf("\nERR: failed to diff changes from %s to %s, mount dir: %s\n", __argv[1], __argv[2], __argv[2]);
    return 1;
  }
  return 0;
}
size_t dagormem_max_crt_pool_sz = 256 << 20;
