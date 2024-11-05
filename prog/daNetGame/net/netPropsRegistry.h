// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace net
{
class IConnection;
}
namespace danet
{
class BitStream;
}
namespace propsreg
{
void init_net_registry_server();
void serialize_net_registry(danet::BitStream &bs);
void deserialize_net_registry(const danet::BitStream &bs);
void flush_net_registry(net::IConnection &conn);
void term_net_registry();
}; // namespace propsreg
