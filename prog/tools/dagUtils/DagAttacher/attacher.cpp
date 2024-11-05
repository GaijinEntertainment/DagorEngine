// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>

#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <3d/dag_materialData.h>
#include <3d/dag_texMgr.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>


static void init_dagor(const char *base_path);
static void shutdown_dagor();
static void show_usage();
static void attache_nodes(Node *attach_to, Node *attach_from, const char *prefix);


//==============================================================================
int _cdecl main(int argc, char **argv)
{
  dd_get_fname(""); //== pull in directoryService.obj
  printf("DAG texture rename\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");

  if (argc < 4)
  {
    ::show_usage();
    return 1;
  }

  ::init_dagor(argv[0]);

  printf("Loading file...");

  AScene scnTo, scnFrom;
  if (!::load_ascene(argv[1], scnTo, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    printf("\nFATAL: error loading file \"%s\"", argv[1]);
    ::shutdown_dagor();
    return 1;
  }

  if (!::load_ascene(argv[2], scnFrom, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    printf("\nFATAL: error loading file \"%s\"", argv[2]);
    ::shutdown_dagor();
    return 1;
  }

  printf(" attaching...\n");
  attache_nodes(scnTo.root, scnFrom.root, argv[2]);

  printf(" OK\n");
  printf(" exporting...\n");
  DagExporter::exportGeometry(argv[3], scnTo.root);
  printf(" OK\n");

  printf("done\n");

  ::shutdown_dagor();
  return 0;
}


//==============================================================================
static void init_dagor(const char *base_path)
{
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
  printf("usage: texren <A.dag> <B.dag> <Out.dag>\n");
  printf("where\n");
  printf("<A> & <B> \tpath to DAG file operands\n");
  printf("<Out.dat>\tresult of combining (A+B)\n");
}


//==============================================================================
static void rename_node(Node *node, const char *prefix)
{
  if (!node)
    return;
  node->name = String(128, "%s%s", prefix, (const char *)node->name).data();
  for (int i = 0; i < node->child.size(); ++i)
    rename_node(node->child[i], prefix);
}

static void move_nodes(Node *node, const TMatrix &tm)
{
  if (!node)
    return;
  node->tm = tm * node->tm;
}

static void attache_nodes(Node *attach_to, Node *attach_from, const char *prefix)
{
  if (!attach_to || !attach_from)
    return;
  rename_node(attach_from, prefix);
  //  move_nodes();
  /*for (int i = 0; i < attach_from->child.size(); ++i)
  {
    if (!attach_from->child[i])
      continue;
    attach_to->child.push_back(attach_from->child[i]);
    attach_from->child[i]->tm = attach_from->tm*attach_from->child[i]->tm;
    attach_from->child[i]->parent = attach_to;
  }*/
  attach_to->child.push_back(attach_from);
  attach_from->parent = attach_to;
  attach_to->invalidate_wtm();
  attach_to->calc_wtm();
}
