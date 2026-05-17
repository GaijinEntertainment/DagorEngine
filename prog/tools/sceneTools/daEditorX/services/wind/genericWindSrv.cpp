// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/wind/ambientWind.h>
#include "windSrv.h"

class GenericWindService : public WindBaseService
{
public:
  GenericWindService() : WindBaseService() {}

  void term() { close_ambient_wind(); }

  void update() { update_ambient_wind(); }

private:
  void applyWind(const WindSettings &settings, WindSettings::Ambient &currentAmbient) const override
  {
    init_ambient_wind(currentAmbient.windFlowTextureName, currentAmbient.windMapBorders, settings.windDir, settings.windStrength,
      currentAmbient.windNoiseStrength, currentAmbient.windNoiseSpeed, currentAmbient.windNoiseScale,
      currentAmbient.windNoisePerpendicular, currentAmbient.windNoiseTextureName);

    if (settings.enabled)
      enable_ambient_wind();
    else
      disable_ambient_wind();
  }
};

static GenericWindService srv;

void *get_generic_wind_service() { return &srv; }
