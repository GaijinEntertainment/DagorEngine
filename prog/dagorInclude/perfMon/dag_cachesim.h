//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class ScopedCacheSim
{
public:
  ScopedCacheSim();
  ~ScopedCacheSim();

  static void schedule();
};
