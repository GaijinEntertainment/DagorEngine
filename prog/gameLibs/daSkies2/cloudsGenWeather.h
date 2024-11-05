// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>

#include "clouds2Common.h"

class GenWeather
{
public:
  void init();
  void invalidate() { frameValid = false; }
  CloudsChangeFlags render();
  bool setExternalWeatherTexture(TEXTUREID tid);

private:
  UniqueTexHolder clouds_weather_texture;
  PostFxRenderer gen_weather;

  int size = 128;
  bool frameValid = true;
  TEXTUREID externalWeatherTexture = BAD_TEXTUREID;
};