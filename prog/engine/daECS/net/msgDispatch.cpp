// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/msgDispatch.h>
#include <daECS/net/message.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <debug/dag_assert.h>

namespace net
{

using HandlerMap = ska::flat_hash_map<uint32_t, msg_handler_t, ska::power_of_two_std_hash<uint32_t>>;

static HandlerMap handlers;

void register_net_msg_handler(const MessageClass &klass, msg_handler_t handler)
{
  G_ASSERT(handler);
  auto ins = handlers.emplace(typename HandlerMap::value_type(klass.classHash, handler));
  G_ASSERTF(ins.second || (ins.first->second == handler), "Duplicate net msg handler for message class %s/%x", klass.debugClassName,
    klass.classHash);
  ins.first->second = handler;
}

void clear_net_msg_handlers() { HandlerMap().swap(handlers); }

bool dispatch_net_msg_handler(const IMessage *msg)
{
  if (!msg)
    return false;
  auto it = handlers.find(msg->getMsgClass().classHash);
  if (it == handlers.end())
    return false;
  it->second(msg);
  return true;
}

} // namespace net
