//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once


namespace curl_global
{
void init();
void shutdown();
void was_initialized_externally(bool value);
} // namespace curl_global