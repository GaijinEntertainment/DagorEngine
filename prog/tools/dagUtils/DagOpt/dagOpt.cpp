// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_direct.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>

#include <libTools/staticGeom/staticGeometryContainer.h>

#include <debug/dag_debug.h>
#include <memory/dag_mem.h>

#include <stdio.h>


#define MIN_ARGUMENTS_COUNT 2


static void initDagor(const char *base_path);
static int getFaceCount(const StaticGeometryContainer &cont);
static int getMatCount(const StaticGeometryContainer &cont);
static bool backup(const char *path);


int _cdecl main(int argc, char **argv)
{
  dd_get_fname(""); //== pull in directoryService.obj
  if (argc < MIN_ARGUMENTS_COUNT)
  {
    printf("Usage: dagopt <DAG file> [options]\n");
    printf("where\n");
    printf("DAG file\tfile to optimize\n");
    printf("options\t\tadditional settings\n");
    printf("  -h\thide copyright message\n");
    printf("  -b\tbackup modified file\n");
    printf("  -nf\tdo not optimize faces\n");
    printf("  -nm\tdo not optimize materials\n");
    return -1;
  }

  initDagor(argv[0]);

  String dagPath;
  bool doBackup = false;
  bool doFaces = true;
  bool doMaterials = true;
  bool showCopyright = true;

  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
        case 'b':
        case 'B': doBackup = true; break;

        case 'h':
        case 'H': showCopyright = false; break;

        case 'n':
        case 'N':
          switch (argv[i][2])
          {
            case 'f':
            case 'F': doFaces = false; break;
            case 'm':
            case 'M': doMaterials = false; break;
          }
          break;

        default: printf("unknown option %s\n", argv[i]);
      }
    }
    else
      dagPath = argv[i];
  }

  if (showCopyright)
  {
    printf("Dag optimization tool\n");
    printf("Copyright (c) Gaijin Games KFT, 2023\n");
  }

  StaticGeometryContainer cont;
  if (!cont.loadFromDag(dagPath, NULL, true))
  {
    printf("error: couldn't load DAG file \"%s\"\n", (const char *)dagPath);
    return -1;
  }

  const int oldFaceCnt = getFaceCount(cont);
  const int oldMatCnt = getMatCount(cont);

  bool wasOpt = cont.optimize(doFaces, doMaterials);

  const int newFaceCnt = getFaceCount(cont);
  const int deltaFace = oldFaceCnt - newFaceCnt;

  const int newMatCnt = getMatCount(cont);
  const int deltaMat = oldMatCnt - newMatCnt;

  if (deltaFace > 0 || deltaMat > 0 || wasOpt)
    printf("optimize %s...\n", ::get_file_name(dagPath));

  if (deltaFace > 0)
  {
    printf("%i faces was, %i faces remain, %i faces erased\n", oldFaceCnt, newFaceCnt, deltaFace);
    wasOpt = true;
  }

  if (deltaMat > 0)
  {
    printf("%i materials was, %i materials remain, %i materials erased\n", oldMatCnt, newMatCnt, deltaMat);
    wasOpt = true;
  }

  if (wasOpt)
  {
    if (doBackup)
      backup(dagPath);

    cont.exportDag(dagPath);
  }

  return 0;
}


void initDagor(const char *base_path)
{
  ::dd_init_local_conv();

  String dir(base_path);
  ::location_from_path(dir);
  ::append_slash(dir);

  ::dd_add_base_path("");
  ::dd_add_base_path(dir);
}


int getFaceCount(const StaticGeometryContainer &cont)
{
  int ret = 0;
  for (int ni = 0; ni < cont.nodes.size(); ++ni)
  {
    const StaticGeometryNode *node = cont.nodes[ni];
    if (node && node->mesh)
      ret += node->mesh->mesh.face.size();
  }

  return ret;
}


int getMatCount(const StaticGeometryContainer &cont)
{
  int ret = 0;
  Tab<StaticGeometryMaterial *> mats(tmpmem);

  for (int ni = 0; ni < cont.nodes.size(); ++ni)
  {
    const StaticGeometryNode *node = cont.nodes[ni];
    if (node && node->mesh)
      for (int mi = 0; mi < node->mesh->mats.size(); ++mi)
      {
        bool known = false;
        for (int i = 0; i < mats.size(); ++i)
          if (node->mesh->mats[mi] == mats[i])
          {
            known = true;
            break;
          }

        if (!known)
          mats.push_back(node->mesh->mats[mi]);
      }
  }

  return mats.size();
}


bool backup(const char *path)
{
  String bakPath(path);
  ::location_from_path(bakPath);
  bakPath = ::make_full_path(bakPath, String(64, "%s.bak", (const char *)::get_file_name_wo_ext(path)));

  return dag_copy_file(path, bakPath);
}
