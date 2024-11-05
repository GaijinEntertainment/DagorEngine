//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>

struct GameLadder
{
  TMatrix tm;
  int ladderStepsCount = 0;
  GameLadder(const TMatrix &m, int steps) : tm(m), ladderStepsCount(steps) {}
};
