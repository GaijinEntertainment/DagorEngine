//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>


namespace curl_global
{
void init();
void shutdown();
void was_initialized_externally(bool value);
void set_requests_limit(uint32_t limit);
} // namespace curl_global