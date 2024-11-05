//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>

class RapidJsonWriter;

namespace bind_dascript
{
void message_queue_put_raw(const char *queueName, const das::TBlock<void, RapidJsonWriter> &block, das::Context *context,
  das::LineInfoArg *at);
int64_t generate_transaction_id();
} // namespace bind_dascript
