// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <compressionUtils/compression.h>
#include "compressionImpl.h"
#include <arc/bzip2-1.0.3/bzlib.h>

class Bzip2Compression final : public Compression
{
public:
  int getRequiredCompressionBufferLength(int plainDataLen, int level) const override
  {
    G_UNREFERENCED(level);
    // the worst case is uncompressible data which will be actually stored as is using the high bit set in the size field
    // so it is just plainDataLen + sizeof(uint32_t)
    return plainDataLen + sizeof(uint32_t);
  }
  int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const override
  {
    G_UNUSED(dataLen);
    const char *data = (const char *)data_;
    G_ASSERT(dataLen >= sizeof(uint32_t));
    G_ASSERT(data != NULL);
    uint32_t field32 = local_ntohl(*((uint32_t *)data));
    uint32_t size = field32 & 0x7FFFFFFF;
    if (size > MAX_PLAIN_DATA_SIZE)
      return -1;
    if (field32 & 0x80000000)
    {
      // the data is not compressed (uncompressible), decompress will return a pointer into the input buffer
      return 0;
    }
    return size;
  }
  char getId() const override { return 'b'; }
  bool isValid() const override { return true; }
  const char *getName() const override { return "bzip2"; }
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
    bz_stream bzip2Stream;
    bzip2Stream.bzalloc = NULL;
    bzip2Stream.bzfree = NULL;
    bzip2Stream.opaque = NULL;
    // best compression, zero verbosity, default work factor
    BZ2_bzCompressInit(&bzip2Stream, 9, 0, 0);
    bzip2Stream.avail_in = inLen;
    bzip2Stream.next_in = (char *)in;
    bzip2Stream.avail_out = outLen - sizeof(uint32_t);
    bzip2Stream.next_out = outData;
    int r = BZ2_bzCompress(&bzip2Stream, BZ_FINISH);
    const char *retVal = NULL;
    if (r == BZ_STREAM_END)
    {
      // compressed OK
      *((uint32_t *)out) = local_htonl(inLen);
      outLen -= bzip2Stream.avail_out;
      retVal = out;
    }
    else if (r == BZ_FINISH_OK)
    {
      // not enough space in the out buffer (uncompressible data), just store the source data
      G_ASSERT(outLen >= (inLen + sizeof(uint32_t)));
      *((uint32_t *)out) = local_htonl(inLen | 0x80000000);
      outLen = inLen + sizeof(uint32_t);
      memcpy(&out[sizeof(uint32_t)], in, inLen);
      retVal = out;
    }
    BZ2_bzCompressEnd(&bzip2Stream);
    return retVal;
  }
  const char *decompress(const void *in_, int inLen, void *out_, int &outLen) const override
  {
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(inLen >= sizeof(uint32_t));
    uint32_t uncompressedLen = getRequiredDecompressionBufferLength(in, inLen);
    if (uncompressedLen > MAX_PLAIN_DATA_SIZE) // -1 is the case too
      return NULL;
    G_ASSERT(outLen >= uncompressedLen);

    if (local_ntohl(*((uint32_t *)in)) & 0x80000000) // just plain data stored
    {
      int declaredLen = local_ntohl(*((uint32_t *)in)) & 0x7FFFFFFF;
      if (inLen < declaredLen + sizeof(uint32_t))
        return NULL;
      outLen = declaredLen;
      return &in[sizeof(uint32_t)]; // no copying here
    }

    G_ASSERT(out != NULL);

    bz_stream bzip2Stream;
    bzip2Stream.bzalloc = NULL;
    bzip2Stream.bzfree = NULL;
    bzip2Stream.opaque = NULL;
    // verbosity 0, small 0
    BZ2_bzDecompressInit(&bzip2Stream, 0, 0);
    bzip2Stream.avail_in = inLen - sizeof(uint32_t);
    bzip2Stream.next_in = (char *)&in[sizeof(uint32_t)];
    bzip2Stream.avail_out = outLen;
    bzip2Stream.next_out = out;
    int r = BZ2_bzDecompress(&bzip2Stream);
    const char *retVal = NULL;
    if (r == BZ_STREAM_END)
    {
      outLen -= bzip2Stream.avail_out;
      retVal = out;
    }
    BZ2_bzDecompressEnd(&bzip2Stream);
    return retVal;
  }
};
