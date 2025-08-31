// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/basePath.h>
#include <stddef.h>
#include <string.h>
#include <debug/dag_debug.h>


static void rebuild_strings();

static char fn_storage[4096];
static char *fn_next_ptr = fn_storage;
static int fn_remove_cnt = 0;

static inline const char *fn_dup(const char *fn)
{
  int len = i_strlen(fn) + 1;
  if (fn_next_ptr + len - fn_storage > sizeof(fn_storage))
  {
    if (!fn_remove_cnt)
      return NULL;

    rebuild_strings();
    if (fn_next_ptr + len - fn_storage > sizeof(fn_storage))
      return NULL;
  }
  memcpy(fn_next_ptr, fn, len);
  fn_next_ptr += len;
  return fn_next_ptr - len;
}


bool dd_add_base_path(const char *base_path, bool insert_first)
{
  if (!base_path)
    return false;

  if (insert_first)
  {
    if (df_base_path[DF_MAX_BASE_PATH_NUM - 1])
      return false; // not enough entries
    for (int i = DF_MAX_BASE_PATH_NUM - 1; i >= 0; i--)
      if (df_base_path[i] && !dd_stricmp(base_path, df_base_path[i]))
        return false; // duplicate path
    for (int i = DF_MAX_BASE_PATH_NUM - 1; i > 0; i--)
      df_base_path[i] = df_base_path[i - 1];

    df_base_path[0] = fn_dup(base_path);
    rebuild_basepath_vrom_mounted();
    return df_base_path[0] != NULL;
  }

  for (int i = 0; i < DF_MAX_BASE_PATH_NUM; i++)
    if (df_base_path[i])
    {
      // duplicate path
      if (dd_stricmp(base_path, df_base_path[i]) == 0)
        return false;
    }
    else
    {
      df_base_path[i] = fn_dup(base_path);
      rebuild_basepath_vrom_mounted();
      return df_base_path[i] != NULL;
    }
  // not enough entries
  return false;
}

//! removes base_path from list
const char *dd_remove_base_path(const char *base_path)
{
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM; i++)
    if (df_base_path[i] && dd_stricmp(base_path, df_base_path[i]) == 0)
    {
      const char *ptr = df_base_path[i];
      for (; i < DF_MAX_BASE_PATH_NUM - 1; i++)
        df_base_path[i] = df_base_path[i + 1];
      df_base_path[i] = NULL;
      fn_remove_cnt++;
      rebuild_basepath_vrom_mounted();
      return ptr;
    }
  return NULL;
}

//! clears list of base pathes
void dd_clear_base_paths()
{
  memset(df_base_path, 0, sizeof(df_base_path));
  memset(df_base_path_vrom_mounted, 0, sizeof(df_base_path_vrom_mounted));
  fn_next_ptr = fn_storage;
  fn_remove_cnt = 0;
}

//! dumps list of base pathes to output
void dd_dump_base_paths()
{
  debug("Base path list:");
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM; i++)
  {
    if (!df_base_path[i])
      break;

    debug("%i: '%s'", i, df_base_path[i]);
  }
  debug("End of base path list");
}


const char *df_next_base_path(int *index_ptr)
{
  if (*index_ptr < 0 || *index_ptr >= DF_MAX_BASE_PATH_NUM)
    return NULL;

  for (int i = *index_ptr; i < DF_MAX_BASE_PATH_NUM; i++)
    if (df_base_path[i])
    {
      *index_ptr = i + 1;
      return df_base_path[i];
    }

  *index_ptr = DF_MAX_BASE_PATH_NUM;
  return NULL;
}


static void rebuild_strings()
{
  char stor2[4096];
  ptrdiff_t rebase = stor2 - fn_storage;

  memcpy(stor2, fn_storage, sizeof(stor2));
  fn_next_ptr = fn_storage;
  fn_remove_cnt = 0;

  for (int i = 0; i < DF_MAX_BASE_PATH_NUM; i++)
    if (df_base_path[i])
      df_base_path[i] = fn_dup(df_base_path[i] + rebase);
}

#define EXPORT_PULL dll_pull_osapiwrappers_basePathMgr
#include <supp/exportPull.h>
