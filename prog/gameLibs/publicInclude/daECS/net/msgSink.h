//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/net/message.h>

namespace net
{

typedef void (*msg_handler_t)(const IMessage *msg);

ecs::EntityId get_msg_sink(); // if INVALID_ENTITY_ID then not created yet
void set_msg_sink_created_cb(void (*on_msg_sink_created_cb)(ecs::EntityId eid));

}; // namespace net
