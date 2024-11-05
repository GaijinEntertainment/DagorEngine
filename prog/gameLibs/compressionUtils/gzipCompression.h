// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <compression.h>
#include "compressionImpl.h"
#include <zlib.h>


class GzipCompression final : public Compression
{
  enum
  {
    GZIP_HEADER_SIZE = 10,
    GZIP_TRAILER_SIZE = 8,
    GZIP_TRIMMED_HEADER_SIZE = GZIP_HEADER_SIZE - 1,
    ZLIB_DEFAULT_MEM_LEVEL = 8, // DEF_MEM_LEVEL is declared in zutil.h which cannot be used by apps
    // this ugly thing means wrapping output with gzip header and trailer,
    // see http://www.zlib.net/manual.html#Advanced about it
    GZIP_MAGIC_WINDOW_SIZE = 16 | MAX_WBITS,
  };

public:
  GzipCompression() { defaultLevel = Z_BEST_COMPRESSION; }

  int getRequiredCompressionBufferLength(int plainDataLen, int level) const override
  {
    int compressionLevel = clampLevel(level);
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    deflateInit2(&zlibStream, compressionLevel, Z_DEFLATED, GZIP_MAGIC_WINDOW_SIZE, ZLIB_DEFAULT_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    int reqSize = (int)deflateBound(&zlibStream, plainDataLen);
    deflateEnd(&zlibStream);
    // deflateBound does not account for gzip wrapping
    return GZIP_TRIMMED_HEADER_SIZE + reqSize + GZIP_TRAILER_SIZE;
  }
  int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const override
  {
    const char *data = (const char *)data_;
    G_ASSERT(data != NULL);

    if (dataLen > 0 && data[0] == getId())
    {
      data++;
      dataLen--;
    }

    if (dataLen < GZIP_TRIMMED_HEADER_SIZE + GZIP_TRAILER_SIZE)
      return -1;
    char outDummy;
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    zlibStream.avail_in = 0;
    zlibStream.next_in = Z_NULL;
    inflateInit2(&zlibStream, GZIP_MAGIC_WINDOW_SIZE);
    zlibStream.avail_out = 1;
    zlibStream.next_out = (Bytef *)&outDummy;

    char idHolder = getId();
    zlibStream.avail_in = 1;
    zlibStream.next_in = (Bytef *)&idHolder;
    G_VERIFY(inflate(&zlibStream, Z_BLOCK) == Z_OK);
    G_ASSERT(zlibStream.avail_in == 0);

    zlibStream.avail_in = dataLen;
    zlibStream.next_in = (Bytef *)data;

    bool success = true;
    if (inflate(&zlibStream, Z_BLOCK) != Z_OK)
      success = false;
    else if (zlibStream.avail_in < GZIP_TRAILER_SIZE)
      success = false;

    inflateEnd(&zlibStream);

    return success ? le2be32(local_ntohl(*(uint32_t *)&data[dataLen - sizeof(uint32_t)])) : -1;
  }
  int getMinLevel() const override { return Z_BEST_SPEED; }
  int getMaxLevel() const override { return Z_BEST_COMPRESSION; }
  char getId() const override { return 0x1F; } // ID1 value according to gzip rfc
  bool isValid() const override { return true; }
  const char *getName() const override { return "gzip"; }
  const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const override
  {
    int compressionLevel = clampLevel(level);
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen <= MAX_PLAIN_DATA_SIZE);
    G_ASSERT(outLen >= getRequiredCompressionBufferLength(inLen, level));
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    deflateInit2(&zlibStream, compressionLevel, Z_DEFLATED, GZIP_MAGIC_WINDOW_SIZE, ZLIB_DEFAULT_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    zlibStream.avail_in = inLen;
    zlibStream.next_in = (Bytef *)in;

    char firstByte = 0;
    zlibStream.avail_out = 1;
    zlibStream.next_out = (Bytef *)&firstByte;

    G_ASSERT(deflate(&zlibStream, Z_FINISH) == Z_OK);
    G_ASSERT(firstByte == getId());

    zlibStream.avail_out = outLen;
    zlibStream.next_out = (Bytef *)out;

    int r = deflate(&zlibStream, Z_FINISH);
    const char *retVal = NULL;
    if (r == Z_STREAM_END)
    {
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

    if (inLen > 0 && in[0] == getId())
    {
      in++;
      inLen--;
    }

    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;
    zlibStream.avail_in = 0;
    zlibStream.next_in = Z_NULL;
    inflateInit2(&zlibStream, GZIP_MAGIC_WINDOW_SIZE);
    zlibStream.avail_out = outLen;
    zlibStream.next_out = (Bytef *)out;

    char idHolder = getId();
    zlibStream.avail_in = 1;
    zlibStream.next_in = (Bytef *)&idHolder;
    G_VERIFY(inflate(&zlibStream, Z_BLOCK) == Z_OK);
    G_ASSERT(zlibStream.avail_in == 0);

    zlibStream.avail_in = inLen;
    zlibStream.next_in = (Bytef *)in;
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
