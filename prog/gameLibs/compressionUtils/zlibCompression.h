// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <compression.h>
#include "compressionImpl.h"
#include <zlib.h>


class ZlibCompression final : public Compression
{
public:
  ZlibCompression() { defaultLevel = Z_BEST_COMPRESSION; }
  int getRequiredCompressionBufferLength(int plainDataLen, int level) const override
  {
    int compressionLevel = clampLevel(level);
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    deflateInit(&zlibStream, compressionLevel);
    int reqSize = (int)(deflateBound(&zlibStream, plainDataLen) + 4);
    deflateEnd(&zlibStream);
    return reqSize;
  }
  int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const override
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
  int getMinLevel() const override { return Z_BEST_SPEED; }
  int getMaxLevel() const override { return Z_BEST_COMPRESSION; }
  char getId() const override { return 'z'; }
  bool isValid() const override { return true; }
  const char *getName() const override { return "zlib"; }
  const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const override
  {
    int compressionLevel = clampLevel(level);
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen <= MAX_PLAIN_DATA_SIZE);
    G_ASSERT(outLen >= getRequiredCompressionBufferLength(inLen, compressionLevel));

    char *outData = &out[sizeof(uint32_t)];
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    deflateInit(&zlibStream, compressionLevel);
    zlibStream.avail_in = inLen;
    zlibStream.next_in = (Bytef *)in;
    zlibStream.avail_out = outLen - sizeof(uint32_t);
    zlibStream.next_out = (Bytef *)outData;
    int r = deflate(&zlibStream, Z_FINISH);
    const char *retVal = NULL;
    if (r == Z_STREAM_END)
    {
      *((uint32_t *)out) = local_htonl(inLen);
      outLen -= zlibStream.avail_out;
      retVal = out;
    }
    deflateEnd(&zlibStream);
    return retVal;
  }
  const char *decompress(const void *in_, int inLen, void *out_, int &outLen) const override
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

    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    zlibStream.avail_in = 0;
    zlibStream.next_in = Z_NULL;
    inflateInit(&zlibStream);
    zlibStream.avail_in = inLen - sizeof(uint32_t);
    zlibStream.next_in = (Bytef *)&in[sizeof(uint32_t)];
    zlibStream.avail_out = outLen;
    zlibStream.next_out = (Bytef *)out;
    int r = inflate(&zlibStream, Z_FINISH);
    const char *retVal = NULL;
    if (r == Z_STREAM_END)
    {
      outLen -= zlibStream.avail_out;
      retVal = out;
    }
    inflateEnd(&zlibStream);
    return retVal;
  }
};
