//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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