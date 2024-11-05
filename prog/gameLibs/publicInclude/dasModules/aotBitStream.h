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

inline danet::BitStream *bitstream_addr(danet::BitStream &stream) { return &stream; }
} // namespace bind_dascript
