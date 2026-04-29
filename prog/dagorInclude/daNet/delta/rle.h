//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <daNet/bitStream.h>

namespace net
{

namespace delta
{

void compress(const danet::BitStream &in, danet::BitStream &out);
inline danet::BitStream compress(const danet::BitStream &in)
{
  danet::BitStream out;
  compress(in, out);
  return out;
}

void decompress(const danet::BitStream &in, danet::BitStream &out);
inline danet::BitStream decompress(const danet::BitStream &in)
{
  danet::BitStream out;
  decompress(in, out);
  return out;
}

// raw interface
int rle0ki_compress(dag::Span<uint8_t> dstSlice, dag::ConstSpan<uint8_t> srcSlice);
int rle0ki_decompress(dag::Span<uint8_t> dstSlice, dag::ConstSpan<uint8_t> srcSlice);

} // namespace delta

} // namespace net
