//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_enumBitMask.h>

namespace crypto
{

enum class InitSslFlags
{
  None,
  InitRand = 1,
  InitLocking = 2,
  InitDagorRandSeed = 4,

  InitDefault = (InitRand | InitLocking)
};
DAGOR_ENABLE_ENUM_BITMASK(InitSslFlags);

void init_ssl(InitSslFlags init_flags = InitSslFlags::InitDefault);
void cleanup_ssl();

} // namespace crypto
