//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>

class RapidJsonWriter;

namespace bind_dascript
{
void message_queue_put_raw(const char *queueName, const das::TBlock<void, RapidJsonWriter> &block, das::Context *context,
  das::LineInfoArg *at);
}
