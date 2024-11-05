// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mathBase.h>

class AcesEffect;

enum TransObjectType
{
  TRANS_INVALID,
  TRANS_EFFECT,
  TRANS_FM,
  TRANS_CABLE,
  TRANS_BALOON,
  TRANS_CABLE_REND,

  TRANS_NUM
};

struct SortedRenderTransItem
{
  SortedRenderTransItem() = default;

  SortedRenderTransItem(void *obj, TransObjectType t, float dist) :
    object(obj), type(t), distance_i(bitwise_cast<uint32_t, float>(max(0.f, 1000.f + dist)))
  {}

  void *object = NULL;
  TransObjectType type = TRANS_INVALID;
  uint32_t distance_i = 0;
};
