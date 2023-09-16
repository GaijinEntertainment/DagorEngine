#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>

#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <math/dag_math3d.h>
#include <math/dag_SHmath.h>
#include <3d/dag_materialData.h>
#include <math/dag_SHlight.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <EASTL/vector_set.h>
#include <libTools/dagFileRW/dagMatRemapUtil.h>

#include <3d/dag_resMgr.h>

#include <libTools/staticGeom/staticGeometryContainer.h>

#include <stdio.h>

static void init_dagor(const char *base_path);
static void shutdown_dagor();
static void show_usage();

//==============================================================================

static bool printf_assert(bool verify, const char *file, int line, const char *func, const char *cond, const char *fmt,
  const DagorSafeArg *arg, int anum)
{
  printf("file %s line %d, func %s, cond %s\n", file, line, func, cond);
  return false;
}


extern void default_crt_init_kernel_lib();
int load_ascene2(const char *fn, AScene &sc, int flg, bool fatal_on_error, PtrTab<MaterialData> &list);
void processNode(Node *node, PtrTab<MaterialData> &list, eastl::vector_set<int> &mat_used)
{
  if (node->mat && node && node->obj && node->obj->isSubOf(OCID_MESHHOLDER) && ((MeshHolderObj *)node->obj)->mesh)
  {
    eastl::vector_set<int> local;
    Mesh &m = *((MeshHolderObj *)node->obj)->mesh;
    for (int fi = 0; fi < m.face.size(); ++fi)
      local.insert(min<int>(m.face[fi].mat, node->mat->list.size() - 1));
    for (auto l : local)
    {
      for (int i = 0; i < list.size(); ++i)
        if (((MaterialData *)node->mat->list[l]) == (MaterialData *)list[i])
          mat_used.insert(i);
    }
  }
  for (auto c : node->child)
    processNode(c, list, mat_used);
}

int _cdecl main(int argc, char **argv)
{
  default_crt_init_kernel_lib();

  if (argc < 2)
  {
    ::show_usage();
    return 1;
  }

  const char *inFile = argv[1];
  printf("DAG material optimizer tool\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");
  start_classic_debug_system(".debug", false);

  if (!inFile || !*inFile)
  {
    printf("FATAL: Empty input file name\n");
    return 1;
  }

  const char *outFile = inFile;
  bool useTempFile = true;

  ::init_dagor(argv[0]);

  if (!::dd_file_exist(inFile))
  {
    printf("FATAL: file \"%s\" not exists", inFile);
    return 1;
  }

  DagData data;
  printf("load scene");
  // enable_res_mgr_mt(true, 4096);
  dgs_assertion_handler = &printf_assert;

  if (!load_scene(inFile, data, true))
  {
    printf("\nFATAL: unable to find materials in file \"%s\"\n", inFile);
    return 1;
  }
  printf("loaded scene\n");

  eastl::vector_set<int> mats;
  {
    AScene sc;
    PtrTab<MaterialData> matlist;
    if (!::load_ascene2(inFile, sc, LASF_MATNAMES | LASF_NULLMATS, false, matlist))
    {
      printf("Couldn't load ascene from \"%s\" file\n", inFile);
      return 1;
    }
    processNode(sc.root, matlist, mats);
  }
  printf("loaded scene\n");
  if (mats.size() == data.matlist.size())
  {
    printf("all materials are used\n");
    return 0;
  }
  printf("%d materials was, %d needed\n", data.matlist.size(), mats.size());

  DagData otherData;
  otherData.matlist.reserve(mats.size());


  DataBlock *mat;
  for (int i = 0; i < data.matlist.size(); ++i)
  {
    auto &m = data.matlist[i];
    if (mats.find(i) == mats.end())
    {
      memset(&m.mater, 0, sizeof(m.mater));
      DagColor c;
      c.r = c.g = c.b = 255;
      m.mater.diff = m.mater.amb = m.mater.spec = c;
      for (int ti = 0; ti < DAGTEXNUM; ++ti)
        m.mater.texid[ti] = DAGBADMATID;
      m.script = "";
      m.classname = "";
      m.name = "unused";
      m.mater.power = 8;
    }
  }

  printf("Materials were loaded successfully.\n");

  int unusedTextures = remove_unused_textures(data);

  printf("Removing unused textures from total of %d...\n", data.texlist.size());
  printf("%d texture(s) removed\n", unusedTextures);
  String tempFile(inFile);
  tempFile += "_temp";

  if (write_dag((useTempFile ? tempFile : outFile), data))
  {
    printf("OK\n");
    if (useTempFile)
    {
      printf("Renaming dag <%s> to <%s>\n", (const char *)tempFile, (const char *)outFile);
      ::dd_rename(tempFile, outFile);
    }
  }
  else
    printf("FATAL ERROR writing file.\n");
  ::shutdown_dagor();
  return 0;
}


//==============================================================================
static void init_dagor(const char *base_path)
{
  ::dd_init_local_conv();
  ::dd_add_base_path("");
}


//==============================================================================
static void shutdown_dagor() {}


//==============================================================================
static void show_usage() { printf("usage: mat_opt [-q] <file name> \n"); }
