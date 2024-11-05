// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <compressionUtils/compression.h>
#include "compressionImpl.h"
#include <arc/lz4/lz4.h>
#include <arc/lz4/lz4hc.h>

class Lz4Compression : public Compression
{
public:
  int getRequiredCompressionBufferLength(int plainDataLen, int level) const final
  {
    G_UNREFERENCED(level);
    // we're placing a 32-bit integer containing uncompressed data length at the beginning of the compressed data
    return LZ4_compressBound(plainDataLen) + sizeof(uint32_t);
  }
  int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const final
  {
    G_UNUSED(dataLen);
    const char *data = (const char *)data_;
    G_ASSERT(dataLen >= sizeof(uint32_t));
    G_ASSERT(data != NULL);
    uint32_t size = local_ntohl(*((uint32_t *)data));
    if (size > MAX_PLAIN_DATA_SIZE)
      return -1;
    return size;
  }
  bool isValid() const final { return true; }
  char getId() const override { return 'l'; }
  const char *getName() const override { return "lz4"; }
  const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const override
  {
    G_UNUSED(level);
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen <= MAX_PLAIN_DATA_SIZE);
    G_ASSERT(outLen >= getRequiredCompressionBufferLength(inLen, level));
    char *outData = &out[sizeof(uint32_t)];
    int r = LZ4_compress_default(in, outData, inLen, outLen - sizeof(uint32_t));
    if (!r)
      return NULL;
    *((uint32_t *)out) = local_htonl(inLen);
    outLen = r + sizeof(uint32_t);
    return out;
  }
  const char *decompress(const void *in_, int inLen, void *out_, int &outLen) const final
  {
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen >= sizeof(uint32_t));
    uint32_t uncompressedLen = local_ntohl(*((uint32_t *)in));
    if (uncompressedLen > MAX_PLAIN_DATA_SIZE)
      return NULL;
    G_ASSERT(outLen >= uncompressedLen);
    int r = LZ4_decompress_safe(&in[sizeof(uint32_t)], out, inLen - sizeof(uint32_t), outLen);
    if (r != (int)uncompressedLen)
      return NULL;
    outLen = r;
    return out;
  }
};


class Lz4hcCompression final : public Lz4Compression
{
public:
  char getId() const override { return 'L'; }
  const char *getName() const override { return "lz4hc"; }
  const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const override
  {
    G_UNUSED(level);
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen <= MAX_PLAIN_DATA_SIZE);
    G_ASSERT(outLen >= getRequiredCompressionBufferLength(inLen, level));
    char *outData = &out[sizeof(uint32_t)];
    int r = LZ4_compress_HC(in, outData, inLen, LZ4_compressBound(inLen), LZ4HC_CLEVEL_OPT_MIN);
    *((uint32_t *)out) = local_htonl(inLen);
    outLen = r + sizeof(uint32_t);
    return out;
  }
};
