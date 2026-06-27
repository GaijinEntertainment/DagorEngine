// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net.h"
#include "netPrivate.h"

#include <daECS/core/entityManager.h>
#include <daECS/net/network.h>
#include <daECS/net/connection.h>
#include <daECS/net/message.h>
#include <daECS/net/msgDispatch.h>
#include <daECS/net/netbase.h>
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>
#include <debug/dag_assert.h>


// Untargeted send_net_msg: routes via NetContext (GET_NET_CTX() is thread-aware).
// No EntityManager argument because the routing is per-NetContext, not per-EM.
int send_net_msg(net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc)
{
  const net::MessageNetDesc &msgDesc = !msg_net_desc ? msg.getMsgClass() : *msg_net_desc;

  if (!has_network())
  {
    G_ASSERT(is_server());
    if (msgDesc.routing == net::ROUTING_CLIENT_TO_SERVER || msgDesc.routing == net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER)
    {
      msg.connection = nullptr;
      if (net::dispatch_net_msg_handler(&msg))
        return 1;
    }
    return 0;
  }

  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return 0;

  net::CNetwork *netw = &ctx->getNet();
  int numSends = 0;
  if (msgDesc.routing == net::ROUTING_SERVER_TO_CLIENT)
  {
    if (msgDesc.rcptFilter == &net::broadcast_rcptf || msgDesc.rcptFilter == nullptr)
    {
      for (net::ConnectionsIterator cit; cit; ++cit)
        numSends += netw->sendto_untargeted(/*time*/ 0, msg, &*cit, msg_net_desc) ? 1 : 0;
    }
    else
    {
      Tab<net::IConnection *> conns(framemem_ptr());
      for (auto cn : msgDesc.rcptFilter(conns, ecs::INVALID_ENTITY_ID, msg))
        if (cn)
          numSends += netw->sendto_untargeted(/*time*/ 0, msg, cn, msg_net_desc) ? 1 : 0;
    }
  }
  else
  {
    numSends = netw->sendto_untargeted(/*time*/ 0, msg, netw->getServerConnection(), msg_net_desc) ? 1 : 0;
  }
  return numSends;
}
