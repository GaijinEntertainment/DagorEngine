//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <soundSystem/strHash.h>

namespace bind_dascript
{
inline sndsys::str_hash_t sound_hash(const char *str) { return str ? SND_HASH_SLOW(str) : SND_HASH(""); }
} // namespace bind_dascript
