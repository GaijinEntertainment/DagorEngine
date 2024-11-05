// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <util/dag_hash.h>
#include <util/dag_finally.h>
#include <ioSys/dag_dataBlock.h>

namespace sndsys::banks
{
using prohibited_hash_t = ProhibitedBankDesc::file_hash_t;
G_STATIC_ASSERT((eastl::is_same<prohibited_hash_t, uint64_t>::value));
__forceinline prohibited_hash_t mod_file_hash_fun(const char *data, size_t data_len, prohibited_hash_t hash)
{
  return mem_hash_fnv1<64>(data, data_len, hash);
}

static bool read_file_hash(const char *path, intptr_t &filesize, prohibited_hash_t &filehash, eastl::vector<char> &tmp_buffer)
{
  filesize = {};
  filehash = {};

  file_ptr_t fp = df_open(path, DF_READ);
  if (!fp)
    return true;

  FINALLY([fp]() { df_close(fp); });

  filesize = df_length(fp);

  constexpr intptr_t bufferSize = 4 << 20;
  tmp_buffer.resize(bufferSize);
  G_ASSERT_RETURN(tmp_buffer.size() == bufferSize, false);

  for (intptr_t pos = 0;; pos += bufferSize)
  {
    const intptr_t sizeToRead = min(bufferSize, filesize - pos);
    if (sizeToRead <= 0)
      break;
    const intptr_t sizeRead = df_read(fp, tmp_buffer.begin(), sizeToRead);
    if (sizeRead != sizeToRead)
    {
      logerr("Read file '%s' error: sizeRead %d != sizeToRead %d", path, sizeRead, sizeToRead);
      return false;
    }
    filehash = mod_file_hash_fun(tmp_buffer.begin(), sizeToRead, filehash);
  }
  return true;
}

static bool debug_prohibited_mods(const DataBlock &blk)
{
#if DAGOR_DBGLEVEL > 0
  return blk.getBool("debugProhibitedMods", false);
#else
  G_UNUSED(blk);
  return false;
#endif
}

static bool verify_mods(const char *banks_folder, const char *mod_path, const char *extension, const DataBlock &blk,
  const ProhibitedBankDescs &descs)
{
  bool ok = true;
  FrameStr path;
  eastl::vector<char> tmpBuffer;

  ProhibitedBankDesc cur = {};
  for (const ProhibitedBankDesc &desc : descs)
  {
    G_ASSERT_CONTINUE(desc.filename && *desc.filename);
    if (!cur.filename || strcmp(cur.filename, desc.filename) != 0)
    {
      cur = {};
      cur.filename = desc.filename;
      path.sprintf("%s/%s%s%s", banks_folder, mod_path, cur.filename, extension);
      if (!read_file_hash(path.c_str(), cur.filesize, cur.filehash, tmpBuffer))
        return false;
      const int id = &desc - descs.begin();
      debug("[SNDSYS] modded #%d '%s' size=%d hash=%llu", id, path.c_str(), cur.filesize, cur.filehash);
    }
    if (desc.filesize == cur.filesize && desc.filehash == cur.filehash)
    {
      ok = false;
      logwarn("[SNDSYS] Bank mod '%s' is prohibited, size=%d, hash=%llu, label='%s'", desc.filename, desc.filesize, desc.filehash,
        desc.label);
    }
  }

  return ok || debug_prohibited_mods(blk);
}

} // namespace sndsys::banks
