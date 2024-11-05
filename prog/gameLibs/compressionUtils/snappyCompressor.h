// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <compressionUtils/compression.h>
#include "compressionImpl.h"
#include <arc/snappy-1.1.9/snappy.h>

class SnappyCompression final : public Compression
{
public:
  int getRequiredCompressionBufferLength(int plainDataLen, int level) const override
  {
    G_UNREFERENCED(level);
    return (int)snappy::MaxCompressedLength(plainDataLen);
  }
  int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const override
  {
    size_t len = 0;
    if (snappy::GetUncompressedLength((const char *)data_, dataLen, &len))
      if (len <= MAX_PLAIN_DATA_SIZE)
        return (int)len;
    return -1;
  }
  char getId() const override { return 's'; }
  bool isValid() const override { return true; }
  const char *getName() const override { return "snappy"; }
  const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const override
  {
    G_UNUSED(level);
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen <= MAX_PLAIN_DATA_SIZE);
    G_ASSERT(outLen >= getRequiredCompressionBufferLength(inLen, level));

    size_t outLenSizeT = outLen;
    snappy::RawCompress(in, inLen, out, &outLenSizeT);
    outLen = (int)outLenSizeT;
    return out;
  }
  const char *decompress(const void *in_, int inLen, void *out_, int &outLen) const override
  {
    const char *in = (const char *)in_;
    char *out = (char *)out_;
    G_ASSERT(in != NULL);
    G_ASSERT(out != NULL);
    G_ASSERT(inLen > 0);
    int reqLen = getRequiredDecompressionBufferLength(in, inLen);
    if (reqLen < 0)
      return NULL;
    G_ASSERT(outLen >= reqLen);
    if (!snappy::RawUncompress(in, inLen, out))
      return NULL;
    outLen = reqLen;
    return out;
  }

protected:
  static const int uncompressedLenSize = 4;
};
