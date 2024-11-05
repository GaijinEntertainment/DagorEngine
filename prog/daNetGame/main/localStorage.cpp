// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "localStorage.h"
#include <ioSys/dag_cryptIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>
#include <EASTL/string.h>
#include <folders/folders.h>
#include <json/reader.h>
#include <json/writer.h>
#include <dataBlockUtils/blk2json.h>

namespace local_storage
{

class RobustFileLoadCB : public IBaseLoad
{
  file_ptr_t fileHandle = NULL;
  eastl::string targetName;

public:
  RobustFileLoadCB(const char *fname)
  {
    fileHandle = df_open(fname, DF_READ);
    targetName = fname;
  }

  ~RobustFileLoadCB() { close(); }

  void close()
  {
    if (fileHandle)
    {
      df_close(fileHandle);
      fileHandle = NULL;
    }
  }

  void read(void *ptr, int size) override { tryRead(ptr, size); }

  int tryRead(void *ptr, int size) override
  {
    if (!fileHandle)
      return 0;
    return df_read(fileHandle, ptr, size);
  }

  int tell() override
  {
    if (!fileHandle)
      return 0;
    return df_tell(fileHandle);
  }

  void seekto(int pos) override
  {
    if (fileHandle)
      df_seek_to(fileHandle, pos);
  }

  void seekrel(int pos) override
  {
    if (fileHandle)
      df_seek_rel(fileHandle, pos);
  }

  const char *getTargetName() override { return targetName.c_str(); }
};

class RobustFileSaveCB : public IBaseSave
{
  file_ptr_t fileHandle = NULL;
  eastl::string targetName;

public:
  RobustFileSaveCB(const char *fname)
  {
    fileHandle = df_open(fname, DF_WRITE | DF_CREATE);
    targetName = fname;

    if (!fileHandle)
      logerr("failed to open file for write '%s'", fname);
  }

  ~RobustFileSaveCB() { close(); }

  void close()
  {
    if (fileHandle)
    {
      df_close(fileHandle);
      fileHandle = NULL;
    }
  }

  void write(const void *ptr, int size) override
  {
    if (!fileHandle)
      return;
    if (df_write(fileHandle, (void *)ptr, size) != size)
      logerr("failed to write %d bytes into file %s", size, targetName.c_str());
  }

  int tell() override
  {
    if (!fileHandle)
      return 0;
    return df_tell(fileHandle);
  }

  void seekto(int pos) override
  {
    if (fileHandle)
      df_seek_to(fileHandle, pos);
  }

  void seektoend(int ofs = 0) override
  {
    if (fileHandle)
      df_seek_end(fileHandle, ofs);
  }

  const char *getTargetName() override { return targetName.c_str(); }

  void flush() override
  {
    if (fileHandle)
      df_flush(fileHandle);
  }
};

using CryptedRobustFileSaveIO = CryptedFileSaveIO<RobustFileSaveCB>;
using CryptedRobustFileLoadIO = CryptedFileLoadIO<RobustFileLoadCB>;

namespace hidden
{
static const unsigned crypto_key_seed = 0xE50A84E7;
static eastl::string storage_file;

static const char *get_storage_name() { return storage_file.c_str(); }

static Json::Value read_enc_json(const char *fname)
{
  CryptedRobustFileLoadIO stream(fname, crypto_key_seed);
  char buf[512];
  eastl::vector<char> document;
  while (true)
  {
    int nread = stream.tryRead(&buf, sizeof(buf));
    if (!nread)
      break;
    document.insert(document.end(), &buf[0], &buf[nread]);
  }
  Json::Reader reader;
  Json::Value root;
  if (!document.empty())
    reader.parse(document.begin(), document.end(), root, false);
  return root;
}

static void write_enc_json(const char *fname, Json::Value const &value)
{
  Json::FastWriter writer;
  auto jsonDoc = writer.write(value);
  CryptedRobustFileSaveIO stream(fname, crypto_key_seed);
  stream.write(jsonDoc.data(), jsonDoc.size());
}

void store_json(const char *key, Json::Value const &val)
{
  Json::Value storage = read_enc_json(get_storage_name());
  storage[key] = val;
  write_enc_json(get_storage_name(), storage);
}

Json::Value load_json(const char *key)
{
  Json::Value storage = read_enc_json(get_storage_name());
  if (storage.isObject() || storage.isNull())
    return storage[key];
  else
  {
    logerr("can't read key = %s from json of type = %d", key, (int)storage.type());
    return Json::Value();
  }
}
} // namespace hidden

void init()
{
  hidden::storage_file = folders::get_local_appdata_dir();
  hidden::storage_file += PATH_DELIM;
  hidden::storage_file += "enlconf.bin";
}

} // namespace local_storage
