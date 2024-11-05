// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>

#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <3d/dag_materialData.h>
#include <3d/dag_texMgr.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>


static void init_dagor(const char *base_path);
static void shutdown_dagor();
static void show_usage();
static void get_node_tex(const Node *node, Tab<String> &tex_list);
static void analyze_mat(MaterialData *m, Tab<String> &tex_list);


//==============================================================================
int _cdecl main(int argc, char **argv)
{
  printf("DAG texture rename\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");

  if (argc < 3)
  {
    ::show_usage();
    return 1;
  }

  ::init_dagor(argv[0]);

  printf("Loading file...");

  AScene scn;
  if (!::load_ascene(argv[1], scn, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    printf("\nFATAL: error loading file \"%s\"", argv[1]);
    ::shutdown_dagor();
    return 1;
  }

  printf(" OK\n");
  printf("Obtaining textures...");

  Tab<String> texList(tmpmem);
  ::get_node_tex(scn.root, texList);

  printf(" OK\n");

  printf("Creating rename_tex.cmd...");

  file_ptr_t f = ::df_open("rename_tex.cmd", DF_WRITE);

  if (!f)
  {
    printf("\nFATAL: unable to create file \"rename_tex.cmd\"");
    ::shutdown_dagor();
    return 1;
  }

  for (int i = 0; i < texList.size(); ++i)
  {
    for (int j = 0; j < texList[i].length(); ++j)
      if (texList[i][j] == '/')
        texList[i][j] = '\\';

    String newFname = ::get_file_name_wo_ext(texList[i]);
    newFname += String(8, ".%s", argv[2]);

    ::df_printf(f, "ren %s %s\n", (const char *)texList[i], (const char *)newFname);
  }

  ::df_close(f);

  printf(" OK\n");

  printf("done\n");

  ::shutdown_dagor();
  return 0;
}


//==============================================================================
static void init_dagor(const char *base_path)
{
  dd_get_fname(""); //== pull in directoryService.obj
  ::dd_init_local_conv();
  ::dd_add_base_path("");

  //  String base(base_path);
  //  ::location_from_path(base);

  //  if (base[1] == ':')
  //    strncpy(_ctl_StartDirectory, (const char*)base, 200);
  //  else
  //    _snprintf(_ctl_StartDirectory, 200, "./%s", (const char*)base);
}


//==============================================================================
static void shutdown_dagor() {}


//==============================================================================
static void show_usage()
{
  printf("usage: texren <file name> <extension>\n");
  printf("where\n");
  printf("<filename>\tpath to DAG file\n");
  printf("<extension>\tnew textures extension\n");
}


//==============================================================================
static void get_node_tex(const Node *node, Tab<String> &tex_list)
{
  if (!node)
    return;

  int matCnt = 0;
  int i;

  if (node->mat)
  {
    matCnt = node->mat->subMatCount();
    if (!matCnt)
      matCnt = 1;
  }

  if (matCnt)
  {
    for (i = 0; i < matCnt; ++i)
    {
      MaterialData *sm = node->mat->getSubMat(i);
      if (!sm)
        continue;

      analyze_mat((MaterialData *)sm, tex_list);
    }
  }

  for (int i = 0; i < node->child.size(); ++i)
    ::get_node_tex(node->child[i], tex_list);
}


//==============================================================================
static void analyze_mat(MaterialData *m, Tab<String> &tex_list)
{
  for (int i = 0; i < MAXMATTEXNUM; ++i)
  {
    const char *texName = ::get_managed_texture_name(m->mtex[i]);

    if (texName)
    {
      bool known = false;
      for (int j = 0; j < tex_list.size(); ++j)
        if (!stricmp(texName, tex_list[j]))
        {
          known = true;
          break;
        }

      if (!known)
        tex_list.push_back(::make_good_path(texName));
    }
  }
}
