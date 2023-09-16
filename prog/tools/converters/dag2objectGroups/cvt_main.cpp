#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <libTools/util/fileUtils.h>
#include <startup/dag_startupTex.h>

extern bool generate_res_db(const char *src_dag_file, const char *land_class_file);

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("--------------------------------------------\n"
         "Dag to ObjectsGroup converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n"
         "--------------------------------------------\n"
         "compilation time: %s %s\n\n\n",
    __DATE__, __TIME__);
}

static bool strPars(char *strToPars, const char *prefix, String *str = NULL)
{
  int paramLen = strlen(prefix);
  if (strncmp(strToPars, prefix, paramLen) == 0)
  {
    if (str)
      *str = &strToPars[paramLen];
    return true;
  }
  return false;
}

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  String srcFile;
  String dstObjectGroupsFile;

  if (__argc >= 2)
    srcFile = __argv[1];

  print_title();

  if (strlen(srcFile) < 1)
  {
    printf("usage: <source dag file>\n\n");

    return -1;
  }

  register_jpeg_tex_load_factory();
  register_tga_tex_load_factory();
  register_psd_tex_load_factory();
  register_any_tex_load_factory();

  if (dstObjectGroupsFile.empty())
    dstObjectGroupsFile = srcFile + ".blk";

  generate_res_db(srcFile, dstObjectGroupsFile);

  printf("\n...complete\n\n");

  return true;
}
