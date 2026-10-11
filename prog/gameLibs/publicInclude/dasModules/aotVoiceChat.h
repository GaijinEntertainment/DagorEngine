//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotBitStream.h>

namespace bind_dascript
{
bool join_proximity_chat_room();
void leave_proximity_chat_room();

bool set_max_tamvoice_binary_channel_queue_size(size_t size);
bool read_from_tamvoice_binary_channel(const das::TBlock<void, const danet::BitStream &, uint64_t> &blk, das::Context *context,
  das::LineInfoArg *at);
bool write_to_tamvoice_binary_channel(const danet::BitStream &bs);

} // namespace bind_dascript
