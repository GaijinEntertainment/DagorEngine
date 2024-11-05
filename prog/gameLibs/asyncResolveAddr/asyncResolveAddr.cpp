// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <asyncResolveAddr/asyncResolveAddr.h>

#include <osApiWrappers/dag_threads.h>
#include <util/dag_simpleString.h>
#include <util/dag_delayedAction.h>


void sockets::resolve_socket_addr_async(const char *hostname, uint16_t port,
  const sockets::on_resolved_callback_t<OSAF_IPV4> &on_resolved)
{
  SimpleString hostnameStr(hostname);
  execute_in_new_thread([hostnameStr = eastl::move(hostnameStr), port, on_resolved](auto) {
    debug("[network] async resolving address %s:%d", hostnameStr, port);

    sockets::SocketAddr<OSAF_IPV4> addr(hostnameStr, port);

    if (addr.isValid())
    {
      char buf[64] = {0};
      addr.str(buf, sizeof(buf));

      debug("[network] %s:%d has been resolved: %s", hostnameStr, port, buf);
    }
    else
      debug("[network] %s:%d has been failed", hostnameStr, port);

    delayed_call([on_resolved, addr]() { on_resolved(addr); });
  });
}