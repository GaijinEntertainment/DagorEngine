// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

//
// This header contains not-public API, suitable to use only within network subsystem/directory
//

namespace net
{
class IConnection;
class CNetwork;
} // namespace net

net::CNetwork *get_net_internal();
void flush_new_connection(net::IConnection &conn);
void init_remote_recreate_entity_from_client();
