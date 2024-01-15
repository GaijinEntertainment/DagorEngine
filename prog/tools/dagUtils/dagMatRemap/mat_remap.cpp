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
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <libTools/dagFileRW/dagMatRemapUtil.h>

#include <stdio.h>

static void init_dagor(const char *base_path);
static void shutdown_dagor();
static void show_usage();

static DagColor dagcolor(const IPoint3 &i) { return DagColor(i[0], i[1], i[2]); }

int _cdecl main(int argc, char **argv)
{
  if (argc < 3)
  {
    ::show_usage();
    return 1;
  }
  bool isQuiet = false;
  bool onlyRead = false;
  int optionsCount = 0;
  if (strcmp(argv[1], "-q") == 0)
  {
    isQuiet = true;
    optionsCount = 1;
  }
  else if (strcmp(argv[1], "-r") == 0)
  {
    isQuiet = true;
    optionsCount = 1;
    onlyRead = true;
  }

  const char *inFile = argv[optionsCount + 1];
  if (argc < 3 + optionsCount)
  {
    ::show_usage();
    return 1;
  }
  if (!isQuiet)
  {
    printf("DAG material remap tool\n");
    printf("Copyright (c) Gaijin Games KFT, 2023\n");
    start_classic_debug_system(".debug", false);
  }
  else
  {
    start_classic_debug_system(NULL, false);
  }
  if (!inFile || !*inFile)
  {
    printf("FATAL: Empty input file name\n");
    return 1;
  }

  char *testChar = &argv[optionsCount + 1][strlen(argv[optionsCount + 1]) - 1];
  if (*testChar == '"')
    *testChar = 0;

  const char *outFile = inFile;
  bool useTempFile = true;

  if (argc >= optionsCount + 4)
  {
    outFile = argv[optionsCount + 3];
    useTempFile = false;

    if (!outFile || !*outFile)
    {
      printf("FATAL: Empty output file name\n");
      return 1;
    }
  }

  ::init_dagor(argv[0]);

  if (!::dd_file_exist(inFile))
  {
    printf("FATAL: file \"%s\" not exists", inFile);
    return 1;
  }

  if (!isQuiet)
  {
    printf("Search for materials... ");
    debug_flush(true);
  }
  DagData data;
  if (!load_scene(inFile, data))
  {
    printf("\nFATAL: unable to find materials in file \"%s\"\n", inFile);
    return 1;
  }

  if (!isQuiet)
  {
    printf("OK\n");
    printf("Remap materials... ");
  }
  if (!::dd_file_exist(argv[optionsCount + 2]) || onlyRead)
  {
    if (::make_dag_mats_blk(data).saveToTextFile(argv[optionsCount + 2]))
    {
      if (!isQuiet)
        printf("OK\nMaterials written successfully to %s\n", argv[optionsCount + 2]);
    }
    else
    {
      printf("FATAL:\n Materials cannot be written to %s\n", argv[optionsCount + 2]);
    }

    ::shutdown_dagor();
    return 0;
  }

  DagData otherData;
  DataBlock matBlock;
  if (!matBlock.load(argv[optionsCount + 2]))
  {
    printf("%s is invalid!\n", argv[optionsCount + 2]);
    ::shutdown_dagor();
    return 0;
  }
  otherData.matlist.reserve(matBlock.blockCount());

  DataBlock *mat;
  for (int i = -1; mat = matBlock.getBlockByName("material", i); ++i)
  {
    append_items(otherData.matlist, 1);
    DagData::Material &m = otherData.matlist.back();
    m.name = mat->getStr("name", String(128, "Material%d", i));
    m.classname = mat->getStr("class", "simple");
    memset(&m.mater, 0, sizeof(DagMater));
    m.mater.flags = 0;
    if (mat->getBool("tex16support", false))
      m.mater.flags |= DAG_MF_16TEX;
    if (mat->getBool("twosided", false))
      m.mater.flags |= DAG_MF_2SIDED;
    m.mater.amb = dagcolor(mat->getIPoint3("amb", IPoint3(255, 255, 255)));
    m.mater.diff = dagcolor(mat->getIPoint3("diff", IPoint3(255, 255, 255)));
    m.mater.spec = dagcolor(mat->getIPoint3("spec", IPoint3(255, 255, 255)));
    m.mater.emis = dagcolor(mat->getIPoint3("emis", IPoint3(0, 0, 0)));
    m.mater.power = mat->getReal("power", 8);
    for (int ti = 0, te = !(m.mater.flags & DAG_MF_16TEX) ? 8 : DAGTEXNUM; ti < te; ++ti)
      if (mat->getStr(String(256, "tex%d", ti), NULL))
      {
        String texname(mat->getStr(String(256, "tex%d", ti), ""));
        //::dd_simplify_fname_c(texname.data());
        int tl;
        for (tl = 0; tl < otherData.texlist.size(); ++tl)
          if (stricmp(texname, otherData.texlist[tl]) == 0)
            break;
        if (tl == otherData.texlist.size())
        {
          tl = otherData.texlist.size();
          otherData.texlist.push_back(texname);
        }
        m.mater.texid[ti] = tl;
      }
      else
        m.mater.texid[ti] = DAGBADMATID;

    int scriptId = mat->getNameId("script");
    String script;
    for (int sci = mat->findParam(scriptId); sci >= 0; sci = mat->findParam(scriptId, sci))
      script += String(256, "%s\r\n", mat->getStr(sci));
    m.script = script;
  }

  if (!isQuiet)
  {
    printf("Materials were loaded successfully.\n");
    if (otherData.matlist.size() != data.matlist.size())
    {
      printf("Materials count are not the same.\n Materials will be resolved by name.\n");
    }
  }
  for (int i = 0; i < data.matlist.size(); ++i)
  {
    DagData::Material &m = data.matlist[i];
    DagData::Material *smp = NULL;
    if (otherData.matlist.size() != data.matlist.size())
    {
      for (int j = 0; j < otherData.matlist.size(); ++j)
      {
        if (strcmp(otherData.matlist[j].name, m.name) == 0)
          if (smp)
          {
            printf("Duplicate material name <%s>\n", &smp->name[0]);
            break;
          }
          else
            smp = &otherData.matlist[j];
      }
      if (!smp)
      {
        debug("Cant find material name <%s>", &m.name[0]);
        continue;
      }
    }
    else
    {
      smp = &otherData.matlist[i];
      if (strcmp(smp->name, m.name))
        printf("Changing material <%s> to <%s>\n", &m.name[0], &smp->name[0]);
    }
    // debug("Changing material <%s> to <%s> <%s>-><%s>\n", &m.name[0], &smp->name[0],
    //&m.script[0], &smp->script[0]);
    m = *smp;
    for (int ti = 0, te = !(smp->mater.flags & DAG_MF_16TEX) ? 8 : DAGTEXNUM; ti < te; ++ti)
    {
      if (smp->mater.texid[ti] >= otherData.texlist.size())
        smp->mater.texid[ti] = DAGBADMATID;
      if (smp->mater.texid[ti] != DAGBADMATID)
      {
        String texname(otherData.texlist[smp->mater.texid[ti]]);
        int tl;
        for (tl = 0; tl < data.texlist.size(); ++tl)
          if (stricmp(texname, data.texlist[tl]) == 0)
            break;
        if (tl == data.texlist.size())
        {
          tl = data.texlist.size();
          data.texlist.push_back(texname);
        }
        m.mater.texid[ti] = tl;
      }
    }
  }
  int unusedTextures = remove_unused_textures(data);
  if (!isQuiet)
  {
    printf("OK\nMaterials remapped successfully\n");

    printf("Removing unused textures from total of %d...\n", data.texlist.size());
    printf("%d texture(s) removed\n", unusedTextures);
  }
  String tempFile(inFile);
  tempFile += "_temp";

  if (!isQuiet)
    printf("Writing dag <%s>...", (const char *)(useTempFile ? tempFile : outFile));
  if (write_dag((useTempFile ? tempFile : outFile), data))
  {
    if (!isQuiet)
      printf("OK\n");
    if (useTempFile)
    {
      if (!isQuiet)
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
static void show_usage()
{
  printf("usage: mat_remap [-q] <file name> <new materials path> [<output file>]\n");
  printf("where\n");
  printf("<file name>\t\tpath to DAG file\n");
  printf("<new materials path>\tpath to material format files\n");
  printf("<output file>\t\tpath to output DAG file with remapped materials "
         "(by default <output file> is same as <file name>)\n");
}
