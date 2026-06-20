// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stddef.h>
#include "net.h"

namespace ecs
{
class EntityManager;
}
namespace net
{
class CNetwork;
class INetDriver;
class INetworkObserver;

INetworkObserver *create_net_control_client(void *buf, size_t bufsz);
} // namespace net

net::CNetwork &net_ctx_init_client(ecs::EntityManager &mgr, net::INetDriver *drv);

void net_create_client_driver(ecs::EntityManager &mgr, const eastl::string &server_url, net::ConnectParams &&connect_params);
