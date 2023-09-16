#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <sys/stat.h>
#include <osApiWrappers/dag_files.h>
#include <memory/dag_mem.h>

#include <stdio.h>
#include <windows.h>

bool load_scene(const char *fname, DataBlock &blk, bool load_tm);
bool save_scene(const char *in_fname, DataBlock &blk, const char *out_fname);

static void getFileTime(const char *fn, FILETIME &t)
{
  HANDLE hFile = CreateFile(::df_get_real_name(fn), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
  GetFileTime(hFile, NULL, NULL, &t);
  CloseHandle(hFile);
}

static void setFileTime(const char *fn, FILETIME &t)
{
  HANDLE hFile = CreateFile(::df_get_real_name(fn), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
  SetFileTime(hFile, &t, &t, &t);
  CloseHandle(hFile);
}

static void init_dagor(const char *base_path)
{
  ::dd_init_local_conv();
  ::dd_add_base_path("");
}

//==============================================================================
static void shutdown_dagor() {}

void importScript(const char *in_dag, const char *out_blk, bool load_tm)
{
  if (!::dd_file_exist(in_dag))
  {
    printf("[ERROR]: file not exists %s\n\n", in_dag);
    return;
  }

  DataBlock blk;
  if (!load_scene(in_dag, blk, load_tm))
  {
    printf("[ERROR]\n");
    return;
  }

  blk.saveToTextFile(out_blk);
  printf("Export script to %s ... OK\n", out_blk);

  FILETIME t;
  getFileTime(in_dag, t);
  setFileTime(out_blk, t);
}


bool exportScript(const char *in_dag, const char *in_blk, const char *out_dag)
{
  if (!::dd_file_exist(in_blk) || !::dd_file_exist(in_dag))
    return false;

  FILETIME ft1, ft2;
  getFileTime(in_dag, ft1);
  getFileTime(in_blk, ft2);

  if (CompareFileTime(&ft1, &ft2) == 0)
  {
    printf("[OK] data wasn't changed. shutting down.\n");
    return false;
  }

  DataBlock blk;
  if (!blk.load(in_blk))
  {
    printf("invalid blk: %s", in_blk);
    return false;
  }

  if (!save_scene(in_dag, blk, out_dag))
    return false;

  printf("Create new DAG: %s ... OK\n", out_dag);
  return true;
}

static void show_usage()
{
  printf("DAG scripter tool, v1.01\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n\n");
  printf("usage #1 (export script): dag_script -e [/tm] <source DAG> <dest BLK>\n");
  printf("usage #2 (import script): dag_script -i <old DAG> <source BLK> [new DAG]\n\n");
}

int _cdecl main(int argc, char **argv)
{
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  if (argc < 2)
  {
    ::show_usage();
    return 1;
  }

  const char *typeStr = argv[1];

  if (stricmp(typeStr, "-e") == 0)
  {
    if (argc < 4)
    {
      ::show_usage();
      return 1;
    }

    ::init_dagor(argv[0]);

    if (stricmp(argv[2], "/tm") == 0)
      importScript(argv[3], argv[4], true);
    else
      importScript(argv[2], argv[3], false);
  }
  else if (stricmp(typeStr, "-i") == 0)
  {
    if (argc < 4)
    {
      ::show_usage();
      return 1;
    }

    ::init_dagor(argv[0]);

    const char *out = (argc == 5 ? argv[4] : argv[2]);
    char tmpfile[1024];
    sprintf(tmpfile, "%s.$tmp$.dag", out);
    if (exportScript(argv[2], argv[3], tmpfile))
    {
      ::dd_rename(tmpfile, out);
      printf("rename %s to %s\n", tmpfile, out);
    }
  }
  else
  {
    ::show_usage();
    return -1;
  }

  ::shutdown_dagor();
  return 0;
}
