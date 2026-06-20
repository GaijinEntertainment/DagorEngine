// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stddef.h>

namespace net
{
class INetworkObserver;

INetworkObserver *create_listen_server_net_observer(void *buf, size_t bufsz);
} // namespace net

void register_listen_server_msg_handler();
