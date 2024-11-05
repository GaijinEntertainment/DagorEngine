// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>

class IBrushMask
{
public:
  IBrushMask();

  float getCenteredMask(float x, float y, float radius, float angle = 0);
  bool load(const char *file);

protected:
  struct Mask
  {
    SmallTab<float, TmpmemAlloc> mask;
    int width, height;

    Mask() : width(0), height(0) {}
    float getCenteredMask(float x, float y, float radius, float angle);
  };
  Tab<Mask> maskChain;
};
