// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daSkies2/daScattering.h>
#include "clouds2.h"
#include "preparedSkies.h"


struct SkiesData
{
  CloudsRendererData clouds = {};
  int cloudsDetailLevel;
  int skyDetailLevel;
  int tw = 0, th = 0;
  SimpleString base_name;
  UniqueTex lowresSkies;    // todo: do we need at all? can't just apply skies to temporal clouds?
  PreparedSkies *baseSkies; // fixme:SLI support
  uint32_t renderSunMoon;
  bool shouldRenderStars;
  bool flyThrough = false;
  bool autoDetect = true;
  bool cloudsVisible = true;
  Point3 preparedOrigin = {0, 0, 0};

  ~SkiesData() { destroy_prepared_skies(baseSkies); }
  SkiesData(const char *name, const PreparedSkiesParams &params) :
    cloudsDetailLevel(0),
    skyDetailLevel(0),
    base_name(name),
    baseSkies(0),
    renderSunMoon(0xFFFFFFFF),
    shouldRenderStars(params.panoramic != PreparedSkiesParams::Panoramic::ON)
  {
    baseSkies = create_prepared_skies(name, params);
  }
};
