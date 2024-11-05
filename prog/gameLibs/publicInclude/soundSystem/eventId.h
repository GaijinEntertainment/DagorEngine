//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/strHash.h>

namespace sndsys
{
typedef str_hash_t event_id_t;
} // namespace sndsys

#define SND_ID(a)      SND_HASH(a)
#define SND_ID_SLOW(a) SND_HASH_SLOW(a)
