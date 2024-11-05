// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <yup_parse/yup.h>
#include <osApiWrappers/dag_files.h>
#include <bencode.h>
#include <string.h>

#ifdef _MSC_VER
#define STRTOK_R strtok_s
#else
#define STRTOK_R strtok_r
#endif

be_node *yup_load(const char *yup_path)
{
  file_ptr_t fp = df_open(yup_path, DF_READ | DF_IGNORE_MISSING);
  if (!fp)
    return NULL;
  int dataLen = -1;
  const void *fdata = df_mmap(fp, &dataLen);
  if (!fdata)
  {
    df_close(fp);
    return NULL;
  }
  be_node *ret = be_decoden((const char *)fdata, (long long)dataLen);
  df_unmap(fdata, dataLen);
  df_close(fp);
  return ret;
}

be_node *yup_get(be_node *yup, const char *keypath)
{
  if (!yup || yup->type != BE_DICT)
    return NULL;
  char tmppath[512];
  strncpy(tmppath, keypath, sizeof(tmppath));
  be_node *lastNode = yup;
  char *savedptr = NULL;
  char *token = STRTOK_R(tmppath, "/", &savedptr);
  for (; token; token = STRTOK_R(NULL, "/", &savedptr))
  {
    bool found = false;
    if (lastNode->type == BE_DICT)
    {
      for (int i = 0; lastNode->val.d[i].val; ++i)
        if (strcmp(lastNode->val.d[i].key, token) == 0)
        {
          lastNode = lastNode->val.d[i].val;
          found = true;
          break;
        }
    }
    else
      break;
    if (!found)
    {
      lastNode = NULL;
      break;
    }
  }
  return token ? NULL : lastNode;
}

const char *yup_get_str(be_node *yup, const char *keypath)
{
  be_node *node = yup_get(yup, keypath);
  return node && node->type == BE_STR ? node->val.s : NULL;
}

const long long *yup_get_int(be_node *yup, const char *keypath)
{
  be_node *node = yup_get(yup, keypath);
  return node && node->type == BE_INT ? &node->val.i : NULL;
}

void yup_free(be_node *node)
{
  if (node)
    be_free(node);
}
