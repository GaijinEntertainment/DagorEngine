// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentType.h>

class Rain
{
  bool useDropSplashes : 1;
  float splashRate = 0;
  float growth = 0;
  float parDensity = 15.f;

public:
  Rain();
  Rain(Rain &&) = default;

  ~Rain();

  void update(float dt, float growthRate, float limit);

  bool getUseDropSplashes() { return useDropSplashes; };
};

ECS_DECLARE_RELOCATABLE_TYPE(Rain);
