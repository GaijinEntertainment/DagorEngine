//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daECS/net/netbase.h>
#include <daNet/bitStream.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotDagorMath.h>
#include "bitStreamWalker.h"


MAKE_TYPE_FACTORY(BitStream, danet::BitStream)

namespace bind_dascript
{

template <typename T>
inline bool bitstream_read(const danet::BitStream &stream, T &value)
{
  return stream.Read(value);
}
template <typename T>
inline void bitstream_write(danet::BitStream &stream, T value)
{
  stream.Write(value);
}
template <typename T>
inline void bitstream_write_ref(danet::BitStream &stream, const T &value)
{
  stream.Write(value);
}
template <typename T>
inline void bitstream_write_at(danet::BitStream &stream, T value, BitSize_t pos)
{
  stream.WriteAt(value, pos);
}
template <typename T>
inline void bitstream_write_at_ref(danet::BitStream &stream, const T &value, BitSize_t pos)
{
  stream.WriteAt(value, pos);
}

inline bool bitstream_read_eid(const danet::BitStream &stream, ecs::EntityId &eid) { return net::read_eid(stream, eid); }
inline void bitstream_write_eid(danet::BitStream &stream, ecs::EntityId value) { net::write_eid(stream, value); }

template <typename T>
inline bool bitstream_read_compressed(const danet::BitStream &stream, T &value)
{
  return stream.ReadCompressed(value);
}
template <typename T>
inline void bitstream_write_compressed(danet::BitStream &stream, T value)
{
  stream.WriteCompressed(value);
}
template <BitStreamWalkerMode mode>
inline vec4f bitstream_read_write_any(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  auto *stream = das::cast<typename BitStreamWalker<mode>::BitStreamType *>::to(args[0]);
  BitStreamWalker<mode> walker(stream, &context, &call->debugInfo);

  auto *data = reinterpret_cast<char *>(das::cast<void *>::to(args[1]));
  das::TypeInfo *ti = call->types[1];
  walker.walk(data, ti);
  return das::cast<bool>::from(!walker.cancel());
}

inline danet::BitStream *bitstream_addr(danet::BitStream &stream) { return &stream; }

inline void bitstream_data(danet::BitStream &stream, const das::TBlock<void, const das::TTemporary<const das::TArray<uint8_t>>> &blk,
  das::Context *ctx, das::LineInfoArg *at)
{
  das::Array arr;
  arr.data = (char *)stream.GetData();
  arr.size = stream.GetNumberOfBytesUsed();
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  ctx->invoke(blk, &arg, nullptr, at);
}

inline void bitstream_data_ctor(const das::TArray<uint8_t> &data, const das::TBlock<void, const danet::BitStream &> &blk,
  das::Context *ctx, das::LineInfoArg *at)
{
  danet::BitStream stream((const uint8_t *)data.data, data.size, /*copy*/ false);
  vec4f arg = das::cast<danet::BitStream &>::from(stream);
  ctx->invoke(blk, &arg, nullptr, at);
}

} // namespace bind_dascript
