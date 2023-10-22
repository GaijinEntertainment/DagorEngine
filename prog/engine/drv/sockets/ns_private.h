// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DAGOR_NETSOCKETS_PRIVATE_H
#define _GAIJIN_DAGOR_NETSOCKETS_PRIVATE_H
#pragma once

namespace NetSockets
{
bool winsock2_init();
void winsock2_term();

char *decode_error(int ErrorCode);
void load_debug_info();
}; // namespace NetSockets

#endif
