// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace NetSockets
{
bool winsock2_init();
void winsock2_term();

char *decode_error(int ErrorCode);
void load_debug_info();
}; // namespace NetSockets
