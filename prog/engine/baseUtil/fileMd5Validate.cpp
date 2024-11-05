// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_fileMd5Validate.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_strUtil.h>
#include <hash/md5.h>

static constexpr int HASH_SZ = 16;
static bool getFileHash(const char *fname, unsigned char out_hash[HASH_SZ])
{
  FullFileLoadCB crd(fname);
  memset(out_hash, 0, HASH_SZ);

  if (!crd.fileHandle)
    return false;

  md5_state_t md5s;
  ::md5_init(&md5s);

  unsigned char buf[32768];
  int len, size = df_length(crd.fileHandle);
  while (size > 0)
  {
    len = size > 32768 ? 32768 : size;
    size -= len;
    crd.read(buf, len);
    ::md5_append(&md5s, buf, len);
  }

  ::md5_finish(&md5s, out_hash);
  return true;
}

bool validate_file_md5_hash(const char *fname, const char *md5, const char *logerr_prefix)
{
  unsigned char md5_hash[HASH_SZ];
  if (!getFileHash(fname, md5_hash))
    memset(md5_hash, 0, sizeof(md5_hash));

  String md5_str;
  if (strcmp(md5, data_to_str_hex(md5_str, md5_hash, sizeof(md5_hash))) != 0)
  {
    if (logerr_prefix)
    {
#if DAGOR_DBGLEVEL > 0
      logerr("%s%s, %s != %s", logerr_prefix, fname, md5, md5_str);
#else
      logerr("%s%s", logerr_prefix, fname);
#endif
    }
    return false;
  }

  return true;
}

#define EXPORT_PULL dll_pull_baseutil_fileMd5Validate
#include <supp/exportPull.h>
