//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/net/message.h>
#include <EASTL/functional.h>
#include <EASTL/fixed_function.h>

namespace net
{

typedef void (*msg_handler_t)(const IMessage *msg);

ecs::EntityId get_msg_sink(); // if INVALID_ENTITY_ID then not created yet
                              //
using msg_sink_created_cb_t = eastl::fixed_function<eastl::max(sizeof(void *) * 3, sizeof(int) * 4), void(ecs::EntityId)>;
void set_msg_sink_created_cb(msg_sink_created_cb_t cb);

}; // namespace net
