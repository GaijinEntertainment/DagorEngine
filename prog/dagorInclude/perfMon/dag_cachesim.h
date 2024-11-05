//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class ScopedCacheSim
{
public:
  ScopedCacheSim();
  ~ScopedCacheSim();

  static void schedule();
};
