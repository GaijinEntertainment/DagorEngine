// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <compressionUtils/compression.h>
#include "zstdCompression.h"
#include "zlibCompression.h"
#include "vromfsCompressionImpl.h"
#ifndef DAGOR_MINIMUM_COMPR_UTILS
#include "bzip2Compressor.h"
#include "gzipCompression.h"
#include "lz4Compressor.h"
#include "snappyCompressor.h"
#endif
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <string.h>


static Compression invalidCompression;
static ZstdCompression zstdCompression;
static ZlibCompression zlibCompression;
static VromfsCompression vromfsCompression;
#ifndef DAGOR_MINIMUM_COMPR_UTILS
static Lz4Compression lz4Compression;
static Lz4hcCompression lz4hcCompression;
static SnappyCompression snappyCompression;
static Bzip2Compression bzip2Compression;
static GzipCompression gzipCompression;
#endif

// list order determines priority - first entries are chosen if not configured otherwise
static Compression::CompressionEntry availableCompressions[] = {
#ifndef DAGOR_MINIMUM_COMPR_UTILS
  {lz4hcCompression.getName(), &lz4hcCompression, lz4hcCompression.getId()},
  {lz4Compression.getName(), &lz4Compression, lz4Compression.getId()},
  {snappyCompression.getName(), &snappyCompression, snappyCompression.getId()},
  {bzip2Compression.getName(), &bzip2Compression, bzip2Compression.getId()},
  {gzipCompression.getName(), &gzipCompression, gzipCompression.getId()},
#endif
  {vromfsCompression.getName(), &vromfsCompression, vromfsCompression.getId()},
  {zstdCompression.getName(), &zstdCompression, zstdCompression.getId()},
  {zlibCompression.getName(), &zlibCompression, zlibCompression.getId()},
  {invalidCompression.getName(), &invalidCompression, invalidCompression.getId()}};

const int availableCompressionsCount = countof(availableCompressions);


int Compression::clampLevel(int level) const { return clamp(level, getMinLevel(), getMaxLevel()); }


static const Compression::CompressionEntry *find_entry_by_name(const char *name)
{
  if (!name || !name[0])
    return nullptr;
  for (int i = 0; i < availableCompressionsCount; i++)
  {
    if (strcmp(name, availableCompressions[i].name) == 0)
      return &availableCompressions[i];
  }
  return nullptr;
}


static const Compression::CompressionEntry *find_entry_by_id(int id)
{
  for (int i = 0; i < availableCompressionsCount; i++)
  {
    if (availableCompressions[i].id == id)
      return &availableCompressions[i];
  }
  return nullptr;
}


const Compression &Compression::getInstanceByName(const char *name)
{
  const CompressionEntry *entry = find_entry_by_name(name);
  if (!entry)
    return invalidCompression;
  G_ASSERT(entry->inst);
  return *entry->inst;
}


const Compression &Compression::getInstanceById(int id)
{
  const CompressionEntry *entry = find_entry_by_id(id);
  if (!entry)
    return invalidCompression;
  G_ASSERT(entry->inst);
  return *entry->inst;
}


#ifndef DAGOR_MINIMUM_COMPR_UTILS
const Compression &Compression::getBestCompression() { return bzip2Compression; }
#else
const Compression &Compression::getBestCompression() { return zstdCompression; }
#endif


const Compression::CompressionEntry *Compression::getCompressionTypes() { return availableCompressions; }


int Compression::getCompressionTypesCount() { return availableCompressionsCount; }


const char *Compression::getName() const
{
  static const char *name = "";
  return name;
}


int Compression::getRequiredCompressionBufferLength(int plainDataLen, int level) const
{
  G_UNREFERENCED(plainDataLen);
  G_UNREFERENCED(level);
  return 0;
}


int Compression::getRequiredDecompressionBufferLength(const void *data_, int dataLen) const
{
  G_UNREFERENCED(data_);
  G_UNREFERENCED(dataLen);
  return 0;
}


const char *Compression::compress(const void *in_, int inLen, void *out_, int &outLen, int level) const
{
  G_UNREFERENCED(out_);
  G_UNREFERENCED(level);
  outLen = inLen;
  return (const char *)in_;
}


const char *Compression::decompress(const void *in_, int inLen, void *out_, int &outLen) const
{
  G_UNREFERENCED(out_);
  outLen = inLen;
  return (const char *)in_;
}


bool Compression::compressWithId(const void *in_, int inLen, void *out_, int &outLen, int level) const
{
  if (!isValid())
    return false;
  if (!outLen)
    return false;
  G_ASSERT(out_ != NULL);
  char *out = (char *)out_;
  *out = getId();
  outLen--;
  const char *retVal = compress(in_, inLen, &out[1], outLen, level);
  if (!retVal || retVal != &out[1])
    return false;
  outLen++;
  return true;
}


const Compression *Compression::selectCompressionByList(const char *list)
{
  G_ASSERT(list != NULL);
  int listLen = (int)strlen(list);
  Tab<char> buffer(tmpmem);
  buffer.resize(listLen + 1);
  const char *curName = &buffer[0];
  const Compression *chosen = NULL;
  for (int i = 0; i <= listLen; i++) // terminating '\0' is iterated too
  {
    bool finished = (list[i] == '\0');
    buffer[i] = (list[i] == ';') ? '\0' : list[i];
    if (!buffer[i])
    {
      const Compression &compr = getInstanceByName(curName);
      if (compr.isValid())
      {
        chosen = &compr;
        break;
      }
      if (!finished)
        curName = &buffer[i + 1];
    }
  }

  return chosen;
}
