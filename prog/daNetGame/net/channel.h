// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

enum NetChannel : uint8_t
{
  NC_DEFAULT,
  NC_TIME_SYNC,
  NC_PHYS_SNAPSHOTS = 2, // server -> client
  NC_CONTROLS = 2,       // client -> server
  NC_REPLICATION,        // object creation & comp sync (server -> client)
  NC_AC_AND_VOICE,       // Anti-Cheat/Voice messages
  NC_EFFECT
};
