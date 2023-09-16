//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>

struct GameLadder
{
  TMatrix tm;
  int ladderStepsCount = 0;
  GameLadder(const TMatrix &m, int steps) : tm(m), ladderStepsCount(steps) {}
};
