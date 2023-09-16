#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <math/dag_math3d.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/util/strUtil.h>

#include <stdio.h>


static void init_dagor(const char *base_path);
static void shutdown_dagor();

static void show_usage();

static int get_tex_list(const char *fname, Tab<String> &list);
static bool write_dag(const char *src, const char *dest, int src_pos, const Tab<String> &list);


//==============================================================================
int _cdecl main(int argc, char **argv)
{
  printf("DAG texture remap tool\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");

  if (argc < 3)
  {
    ::show_usage();
    return 1;
  }

  const char *inFile = argv[1];

  if (!inFile || !*inFile)
  {
    printf("FATAL: Empty input file name\n");
    return 1;
  }

  char *testChar = &argv[1][strlen(argv[1]) - 1];
  if (*testChar == '"')
    *testChar = 0;

  const char *outFile = inFile;
  bool useTempFile = true;

  if (argc == 4)
  {
    outFile = argv[3];
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

  printf("Search for textures... ");

  Tab<String> texList(tmpmem);

  const int dagPos = ::get_tex_list(inFile, texList);

  if (!dagPos)
  {
    printf("\nFATAL: unable to find textures in file \"%s\"\n", inFile);
    return 1;
  }

  printf("OK\n");
  printf("Remap textures... ");

  testChar = &argv[2][strlen(argv[2]) - 1];

  if (*testChar == '"')
    *testChar = 0;

  for (int i = 0; i < texList.size(); ++i)
    texList[i] = ::make_full_path(argv[2], ::get_file_name(texList[i]));

  printf("OK\n");
  printf("Save to \"%s\"... ", outFile);

  String tempFile(inFile);
  tempFile += "_temp";

  if (!::write_dag(inFile, useTempFile ? tempFile : outFile, dagPos, texList))
  {
    printf("\nFATAL: unable to write file \"%s\"\n", outFile);
    return 1;
  }

  if (useTempFile)
    ::dd_rename(tempFile, outFile);

  printf("OK\nTextures remapped successfully\n");

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
  printf("usage: tex_remap <file name> <new texture path> [<output file>]\n");
  printf("where\n");
  printf("<file name>\t\tpath to DAG file\n");
  printf("<new texture path>\tnew path to texture files\n");
  printf("<output file>\t\tpath to output DAG file with remapped textures "
         "(by default <output file> is same as <file name>)\n");
}


//==============================================================================
static int get_tex_list(const char *fname, Tab<String> &list)
{
  file_ptr_t dag = ::df_open(fname, DF_READ);

  if (!dag)
    return 0;

  int id = 0;
  ::df_read(dag, &id, 4);

  int result = 0;

  if (id == DAG_ID)
  {
    int dagSize = 0;

    ::df_read(dag, &dagSize, 4);
    ::df_read(dag, &id, 4);

    if (id == DAG_ID)
    {
      int texSize = 0;

      ::df_read(dag, &texSize, 4);
      ::df_read(dag, &id, 4);

      if (id == DAG_TEXTURES)
      {
        int count = 0;

        clear_and_shrink(list);
        ::df_read(dag, &count, 2);

        for (int i = 0; i < count; ++i)
        {
          int sz = 0;
          char name[256 + 1] = "";

          ::df_read(dag, &sz, 1);

          if (sz)
          {
            ::df_read(dag, name, sz);
            name[sz] = 0;

            list.push_back(String(name));
          }

          result = df_tell(dag);
        }
      }
    }
  }

  ::df_close(dag);
  return result;
}


//==============================================================================
static bool write_dag(const char *src, const char *dest, int src_pos, const Tab<String> &list)
{
  file_ptr_t from = df_open(src, DF_READ);

  if (!from)
    return false;

  file_ptr_t to = df_open(dest, DF_WRITE);

  if (!to)
  {
    df_close(from);
    return false;
  }

  const int dagId = DAG_ID;
  const int dagTex = DAG_TEXTURES;
  int id = 0;
  bool success = false;

  df_read(from, &id, 4);

  if (id == dagId)
  {
    int srcSize = 0;

    df_read(from, &srcSize, 4);
    df_read(from, &id, 4);

    if (id == dagId)
    {
      int dagSize = 0;
      int texSize = 0;

      df_write(to, &dagId, 4);
      df_write(to, &dagSize, 4);
      df_write(to, &dagId, 4);
      df_write(to, &texSize, 4);
      df_write(to, &dagTex, 4);

      int texCount = list.size();
      df_write(to, &texCount, 2);

      for (int i = 0; i < texCount; ++i)
      {
        const unsigned char len = list[i].length();

        df_write(to, &len, 1);
        df_write(to, (const char *)list[i], len);

        texSize += len + 1;
      }

      dagSize = srcSize + df_tell(to) - src_pos;
      texSize += 6;

      df_seek_to(from, src_pos);

      char buf[1024 * 64];

      for (int read = df_read(from, buf, 1024 * 64); read > 0; read = df_read(from, buf, 1024 * 64))
        df_write(to, buf, read);

      df_seek_to(to, 4);
      df_write(to, &dagSize, 4);

      df_seek_to(to, 12);
      df_write(to, &texSize, 4);

      success = true;
    }
  }

  df_close(from);
  df_close(to);

  return success;
}
