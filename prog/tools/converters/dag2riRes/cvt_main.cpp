#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <libTools/util/fileUtils.h>
#include <startup/dag_startupTex.h>

extern bool generate_res_db(const char *src_dag_file, const char *dst_dir, const char *composit_file, const char *land_class_file,
  bool fast_tex_cvt);

void __cdecl ctrl_break_handler(int) { quit_game(0); }
bool skip_tex_convert = false;
bool ignore_origin = false;
bool clear_nodeprops = false;
bool export_all = false, export_node_001 = false;

static void print_title()
{
  printf("--------------------------------------------\n"
         "Dag to RendInst converter\n"
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

  String srcFile, destDir;
  String dstCompFile, dstLandClFile;
  bool fastCvt = false;

  for (int i = 0; i < __argc; i++)
  {
    if (__argv[i][0] == '-')
    {
      if (strPars(&__argv[i][1], "s:", &srcFile))
        continue;
      if (strPars(&__argv[i][1], "d:", &destDir))
        continue;
      if (strPars(&__argv[i][1], "c:", &dstCompFile))
        continue;
      if (strPars(&__argv[i][1], "l:", &dstLandClFile))
        continue;
      if (strPars(&__argv[i][1], "f"))
      {
        fastCvt = true;
        continue;
      }
      if (strcmp(__argv[i], "-no_tex") == 0)
        skip_tex_convert = true;
      if (strcmp(__argv[i], "-ignore_origin") == 0)
        ignore_origin = true;
      if (strcmp(__argv[i], "-clr_nodeprops") == 0)
        clear_nodeprops = true;
      if (strcmp(__argv[i], "-export_all_nodes") == 0)
        export_all = true;
      if (strcmp(__argv[i], "-export_node_001") == 0)
        export_node_001 = true;
    }
  }

  print_title();

  if (((strlen(srcFile) < 1) || (strlen(destDir) < 1)))
  {
    printf("usage: -s:<source dag file> -d:<destination folder> [-f] [-no_tex] [-ignore_origin] [-clr_nodeprops] [-export_all_nodes]\n"
           "       [-c:<destination composit  file>]\n"
           "       [-l:<destination landClass file>]\n\n");
    return -1;
  }

  register_jpeg_tex_load_factory();
  register_tga_tex_load_factory();
  register_psd_tex_load_factory();
  register_any_tex_load_factory();

  if (dstCompFile.empty())
    dstCompFile = "composit.res.blk";
  if (dstLandClFile.empty())
    dstLandClFile = "landClass.res.blk";

  generate_res_db(srcFile, destDir, dstCompFile, dstLandClFile, fastCvt);

  printf("\n...complete\n\n");

  return 0;
}
