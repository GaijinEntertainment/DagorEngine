//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/net/message.h>

namespace net
{

// Sink-routed and untargeted messages dispatch to the same handler table; the path is chosen
// by the sender (packet ID) and the receiver looks up by classHash in one map.
bool dispatch_net_msg_handler(const IMessage *msg);

void register_net_msg_handler(const MessageClass &klass, msg_handler_t handler);

void clear_net_msg_handlers();

} // namespace net
