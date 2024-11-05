// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daNet/bitStream.h>
#include <lz4/lz4.h>
#include <memory/dag_framemem.h>

#define COMPRESSION_ENABLED   1
#define MAX_COMPRESSION_RATIO 8 // bound to avoid overcommit memory on decompression

inline const danet::BitStream &bitstream_compress(const danet::BitStream &bs, int headerSize, uint8_t ptype, danet::BitStream &outbs,
  const uint32_t compression_threshold)
{
  int payloadSize = (int)BITS_TO_BYTES(bs.GetNumberOfBitsUsed()) - headerSize;
  G_ASSERT(payloadSize >= 0);
  if (!COMPRESSION_ENABLED || (payloadSize + headerSize) < compression_threshold)
    return bs;

  BitSize_t payloadCompressedBound = LZ4_compressBound(payloadSize);
  outbs.ResetWritePointer();
  outbs.reserveBits(BYTES_TO_BITS(headerSize + payloadCompressedBound));
  outbs.Write((const char *)bs.GetData(), headerSize);
  outbs.WriteAt(ptype, 0);
  int compressedSize = LZ4_compress_default((const char *)bs.GetData() + headerSize, (char *)outbs.GetData() + headerSize, payloadSize,
    payloadCompressedBound);
  G_ASSERTF_RETURN(compressedSize > 0, bs, "lz4 compression of %d bytes failed (%d)!", payloadSize, compressedSize);
  if (compressedSize >= payloadSize || compressedSize * MAX_COMPRESSION_RATIO < payloadSize)
    return bs;

  bs.IgnoreBytes(compressedSize);
  outbs.SetWriteOffset(outbs.GetWriteOffset() + BYTES_TO_BITS(compressedSize));

  return outbs;
}

inline bool bitstream_decompress(const danet::BitStream &bs, danet::BitStream &outbs)
{
  if (bs.GetReadOffset() >= bs.GetNumberOfBitsUsed())
    return false;
  G_ASSERT((bs.GetReadOffset() & 7) == 0);
  G_ASSERT((bs.GetNumberOfBitsUsed() & 7) == 0);
  BitSize_t compressedSize = BITS_TO_BYTES(bs.GetNumberOfUnreadBits()), maxOutputSize = compressedSize * MAX_COMPRESSION_RATIO;
  outbs.ResetWritePointer();
  outbs.reserveBits(BYTES_TO_BITS(maxOutputSize));
  int readedSize = LZ4_decompress_safe((const char *)bs.GetData() + BITS_TO_BYTES(bs.GetReadOffset()), (char *)outbs.GetData(),
    compressedSize, maxOutputSize);
  if (readedSize <= 0)
    return false;

  outbs.SetWriteOffset(BYTES_TO_BITS(readedSize));

  return true;
}
