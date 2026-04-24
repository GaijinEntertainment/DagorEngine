//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>

namespace bind_dascript
{
bool join_proximity_chat_room();
void leave_proximity_chat_room();

bool set_max_tamvoice_binary_channel_queue_size(size_t size);
bool read_from_tamvoice_binary_channel(das::TArray<char> &dst, das::Context *context, das::LineInfoArg *at);
bool write_to_tamvoice_binary_channel(const das::TArray<char> &src, das::Context *context, das::LineInfoArg *at);

} // namespace bind_dascript
