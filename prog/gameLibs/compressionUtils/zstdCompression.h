// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <compression.h>
#include "compressionImpl.h"
#include <zstd.h>


class ZstdCompression final : public Compression
{
public:
  ZstdCompression() { defaultLevel = 15; }

  int getRequiredCompressionBufferLength(int dlen, int) const override { return (int)ZSTD_compressBound((size_t)dlen); }
  int getRequiredDecompressionBufferLength(const void *data, int dataLen) const override
  {
    long long plainSize = (long long)ZSTD_getFrameContentSize(data, (size_t)dataLen);
    if (plainSize < 0 || plainSize > MAX_PLAIN_DATA_SIZE)
      return -1;
    return (int)plainSize;
  }
  int getMinLevel() const override { return 1; }
  int getMaxLevel() const override { return ZSTD_maxCLevel(); }
  char getId() const override { return 'Z'; }
  bool isValid() const override { return true; }
  const char *getName() const override { return "zstd"; }
  const char *compress(const void *in, int inLen, void *out, int &outLen, int level) const override
  {
    int compressionLevel = clampLevel(level);

    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen >= 0);
    G_ASSERT(inLen <= MAX_PLAIN_DATA_SIZE);
    G_ASSERT(outLen >= getRequiredCompressionBufferLength(inLen, level));

    size_t comprSize = ZSTD_compress(out, (size_t)outLen, in, (size_t)inLen, compressionLevel);
    if (ZSTD_isError(comprSize))
      return NULL;
    outLen = (int)comprSize;
    return (const char *)out;
  }
  const char *decompress(const void *in, int inLen, void *out, int &outLen) const override
  {
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    int plainSize = getRequiredDecompressionBufferLength(in, inLen);
    G_ASSERT(plainSize >= 0);
    G_ASSERT(outLen >= plainSize);

    size_t decomprSize = ZSTD_decompress(out, (size_t)outLen, in, (size_t)inLen);
    if (ZSTD_isError(decomprSize))
      return NULL;
    outLen = (int)decomprSize;
    return (const char *)out;
  }
};
