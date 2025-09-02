// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

#ifdef _WIN32
using ENetSocket = uintptr_t;
#else
using ENetSocket = int;
#endif
struct _ENetAddress;

const char *ucr_get_key();
static inline bool ucr_is_packet(const void *data, uint32_t data_len)
{
  return data_len >= 16 && *(const uint32_t *)data == 0xd52d71e9;
}
void ucr_send_hello(ENetSocket s, _ENetAddress *addr);
void ucr_send_hello(const char *addr);
void ucr_handle_packet(ENetSocket s, _ENetAddress *addr, const void *data, uint32_t data_len);
