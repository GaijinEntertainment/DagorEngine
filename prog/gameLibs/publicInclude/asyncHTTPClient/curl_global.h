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
} // namespace curl_global