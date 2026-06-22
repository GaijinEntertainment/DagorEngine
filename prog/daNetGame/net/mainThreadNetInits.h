// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace ecs
{
class EntityManager;
}
namespace net
{
struct ConnectParams;
}

bool net_init_early(ecs::EntityManager &mgr);
bool net_init_late_client(net::ConnectParams &&connect_params, ecs::EntityManager &mgr);
void net_init_late_server(ecs::EntityManager &mgr);
