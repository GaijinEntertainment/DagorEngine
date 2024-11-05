// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/wind/ambientWind.h"

float AmbientWind::beaufort_to_meter_per_second(float) { return 0; }

float AmbientWind::meter_per_second_to_beaufort(float) { return 0; }

void AmbientWind::init() {}

void AmbientWind::setWindParameters(const Point4 &, const Point2 &, float, float, float, float, float) {}

void AmbientWind::setWindTextures(const char *, const char *) {}

void AmbientWind::setWindParameters(const DataBlock &) {}
void AmbientWind::setWindTextures(const DataBlock &) {}

void AmbientWind::close() {}

float AmbientWind::getWindStrength() const { return windStrength; }
const Point2 &AmbientWind::getWindDir() const { return windDir; }
float AmbientWind::getWindSpeed() const { return windSpeed; }

void AmbientWind::update() {}

void AmbientWind::enable() {}
void AmbientWind::disable() {}

// This functions are for use in non-ECS projects only
static eastl::unique_ptr<AmbientWind> ambient_wind;

AmbientWind *get_ambient_wind() { return ambient_wind.get(); };
void init_ambient_wind(const char *, const Point4 &, const Point2 &, float, float, float, float, float, const char *) {}
void enable_ambient_wind() {}
void disable_ambient_wind() {}
void close_ambient_wind() {}
void update_ambient_wind() {}