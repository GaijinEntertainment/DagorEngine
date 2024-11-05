//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_sockets.h>

namespace sockets
{
template <OsSocketAddressFamily so_family>
using on_resolved_callback_t = void (*)(const SocketAddr<so_family> &addr);

void resolve_socket_addr_async(const char *hostname, uint16_t port, const on_resolved_callback_t<OSAF_IPV4> &on_resolved);
} // namespace sockets